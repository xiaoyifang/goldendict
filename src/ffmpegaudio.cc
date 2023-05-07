#ifdef MAKE_FFMPEG_PLAYER

#include "audiooutput.hh"
#include "ffmpegaudio.hh"

#include <math.h>
#include <errno.h>

extern "C" {
#include <libavcodec/avcodec.h>
#include <libavformat/avformat.h>
#include <libavutil/avutil.h>
#include "libswresample/swresample.h"
}

#include <QString>
#include <QDataStream>

#include <vector>
#if( QT_VERSION >= QT_VERSION_CHECK( 6, 2, 0 ) )
  #include <QMediaDevices>

  #include <QAudioDevice>
#endif
#include "gddebug.hh"
#include "utils.hh"

using std::vector;

namespace Ffmpeg
{

QMutex DecoderThread::deviceMutex_;

static inline QString avErrorString( int errnum )
{
  char buf[64];
  av_strerror( errnum, buf, 64 );
  return QString::fromLatin1( buf );
}

AudioService & AudioService::instance()
{
  static AudioService a;
  return a;
}

AudioService::AudioService()
{
//  ao_initialize();
}

AudioService::~AudioService()
{
  emit cancelPlaying( true );
//  ao_shutdown();
}

void AudioService::playMemory( const char * ptr, int size )
{
  emit cancelPlaying( false );
  QByteArray audioData( ptr, size );
  DecoderThread * thread = new DecoderThread( audioData, this );

  connect( thread, &DecoderThread::error, this, &AudioService::error );
  connect( this, &AudioService::cancelPlaying, thread, &DecoderThread::cancel, Qt::DirectConnection );
  connect( thread, &QThread::finished, thread, &QObject::deleteLater );

  thread->start();
}

void AudioService::stop()
{
  emit cancelPlaying( false );
}

struct DecoderContext
{
  enum
  {
    kBufferSize = 32768
  };

  static QMutex deviceMutex_;
  QAtomicInt & isCancelled_;
  QByteArray audioData_;
  QDataStream audioDataStream_;
  AVFormatContext * formatContext_;
#if LIBAVCODEC_VERSION_MAJOR < 59
  AVCodec * codec_;
#else
  const AVCodec * codec_;
#endif
  AVCodecContext * codecContext_;
  AVIOContext * avioContext_;
  AVStream * audioStream_;
//  ao_device * aoDevice_;
  AudioOutput * audioOutput;
  bool avformatOpened_;

  SwrContext *swr_;

  DecoderContext( QByteArray const & audioData, QAtomicInt & isCancelled );
  ~DecoderContext();

  bool openCodec( QString & errorString );
  void closeCodec();
  bool openOutputDevice( QString & errorString );
  void closeOutputDevice();
  bool play( QString & errorString );
  bool normalizeAudio( AVFrame * frame, vector<uint8_t> & samples );
  void playFrame( AVFrame * frame );
};

DecoderContext::DecoderContext( QByteArray const & audioData, QAtomicInt & isCancelled ):
  isCancelled_( isCancelled ),
  audioData_( audioData ),
  audioDataStream_( audioData_ ),
  formatContext_( NULL ),
  codec_( NULL ),
  codecContext_( NULL ),
  avioContext_( NULL ),
  audioStream_( NULL ),
  audioOutput( new AudioOutput ),
  avformatOpened_( false ),
  swr_( NULL )
{
}

DecoderContext::~DecoderContext()
{
  closeOutputDevice();
  closeCodec();
}

static int readAudioData( void * opaque, unsigned char * buffer, int bufferSize )
{
  QDataStream * pStream = ( QDataStream * )opaque;
  // This function is passed as the read_packet callback into avio_alloc_context().
  // The documentation for this callback parameter states:
  // For stream protocols, must never return 0 but rather a proper AVERROR code.
  if( pStream->atEnd() )
    return AVERROR_EOF;
  const int bytesRead = pStream->readRawData( ( char * )buffer, bufferSize );
  // QDataStream::readRawData() returns 0 at EOF => return AVERROR_EOF in this case.
  // An error is unlikely here, so just print a warning and return AVERROR_EOF too.
  if( bytesRead < 0 )
    gdWarning( "readAudioData: error while reading raw data." );
  return bytesRead > 0 ? bytesRead : AVERROR_EOF;
}

bool DecoderContext::openCodec( QString & errorString )
{
  formatContext_ = avformat_alloc_context();
  if ( !formatContext_ )
  {
    errorString = "avformat_alloc_context() failed.";
    return false;
  }

  unsigned char * avioBuffer = ( unsigned char * )av_malloc( kBufferSize + AV_INPUT_BUFFER_PADDING_SIZE );

  if ( !avioBuffer )
  {
    errorString = "av_malloc() failed.";
    return false;
  }

  // Don't free buffer allocated here (if succeeded), it will be cleaned up automatically.
  avioContext_ = avio_alloc_context( avioBuffer, kBufferSize, 0, &audioDataStream_, readAudioData, NULL, NULL );
  if ( !avioContext_ )
  {
    av_free( avioBuffer );
    errorString = "avio_alloc_context() failed.";
    return false;
  }

  avioContext_->seekable = 0;
  avioContext_->write_flag = 0;

  // If pb not set, avformat_open_input() simply crash.
  formatContext_->pb = avioContext_;
  formatContext_->flags |= AVFMT_FLAG_CUSTOM_IO;

  int ret = 0;
  avformatOpened_ = true;

  ret = avformat_open_input( &formatContext_, NULL, NULL, NULL );
  if ( ret < 0 )
  {
    errorString = QString( "avformat_open_input() failed: %1." ).arg( avErrorString( ret ) );
    return false;
  }

  ret = avformat_find_stream_info( formatContext_, NULL );
  if ( ret < 0 )
  {
    errorString = QString( "avformat_find_stream_info() failed: %1." ).arg( avErrorString( ret ) );
    return false;
  }

  // Find audio stream, use the first audio stream if available
  for ( unsigned i = 0; i < formatContext_->nb_streams; i++ )
  {
    if ( formatContext_->streams[i]->codecpar->codec_type == AVMEDIA_TYPE_AUDIO )
    {
      audioStream_ = formatContext_->streams[i];
      break;
    }
  }
  if ( !audioStream_ )
  {
    errorString = QString( "Could not find audio stream." );
    return false;
  }

  codec_ = avcodec_find_decoder( audioStream_->codecpar->codec_id );
  if ( !codec_ )
  {
    errorString = QString( "Codec [id: %1] not found." ).arg( audioStream_->codecpar->codec_id );
    return false;
  }
  codecContext_ = avcodec_alloc_context3( codec_ );
  if ( !codecContext_ )
  {
    errorString = QString( "avcodec_alloc_context3() failed." );
    return false;
  }
  avcodec_parameters_to_context( codecContext_, audioStream_->codecpar );

  ret = avcodec_open2( codecContext_, codec_, NULL );
  if ( ret < 0 )
  {
    errorString = QString( "avcodec_open2() failed: %1." ).arg( avErrorString( ret ) );
    return false;
  }

  gdDebug( "Codec open: %s: channels: %d, rate: %d, format: %s\n", codec_->long_name,
          codecContext_->channels, codecContext_->sample_rate, av_get_sample_fmt_name( codecContext_->sample_fmt ) );

  auto layout = codecContext_->channel_layout;
  if( !layout )
  {
    layout                        = av_get_default_channel_layout( codecContext_->channels );
    codecContext_->channel_layout = layout;
  }

  swr_ = swr_alloc_set_opts( NULL,
    layout,
    AV_SAMPLE_FMT_S16,
    44100,
    layout,
    codecContext_->sample_fmt,
    codecContext_->sample_rate,
    0,
    NULL );

  if( !swr_ || swr_init( swr_ ) < 0 )
  {
    av_log( NULL, AV_LOG_ERROR, "Cannot create sample rate converter \n" );
    swr_free( &swr_ );
    return false;
  }

  return true;
}

void DecoderContext::closeCodec()
{
  if ( swr_ )
  {
    swr_free( &swr_ );
  }

  if ( !formatContext_ )
  {
    if ( avioContext_ )
    {
      av_free( avioContext_->buffer );
      avioContext_ = NULL;
    }
    return;
  }

  // avformat_open_input() is not called, just free the buffer associated with
  // the AVIOContext, and the AVFormatContext
  if ( !avformatOpened_ )
  {
    if ( formatContext_ )
    {
      avformat_free_context( formatContext_ );
      formatContext_ = NULL;
    }

    if ( avioContext_ )
    {
      av_free( avioContext_->buffer );
      avioContext_ = NULL;
    }
    return;
  }

  avformatOpened_ = false;

  // Closing a codec context without prior avcodec_open2() will result in
  // a crash in ffmpeg
  if ( audioStream_ && codecContext_ && codec_ )
  {
    audioStream_->discard = AVDISCARD_ALL;
    avcodec_close( codecContext_ );
    avcodec_free_context( &codecContext_ );
  }

  avformat_close_input( &formatContext_ );
  av_free( avioContext_->buffer );
}

bool DecoderContext::openOutputDevice( QString & errorString )
{
  // only check device when qt version is greater than 6.2
  #if  (QT_VERSION >= QT_VERSION_CHECK(6,2,0))
  QAudioDevice m_outputDevice = QMediaDevices::defaultAudioOutput();
  if(m_outputDevice.isNull()){
    errorString += QString( "Can not found default audio output device" );
    return false;
  }
  #endif

  audioOutput->setAudioFormat( 44100, codecContext_->channels );
  return true;
}

void DecoderContext::closeOutputDevice()
{

}

bool DecoderContext::play( QString & errorString )
{
  AVFrame * frame = av_frame_alloc();
  if ( !frame )
  {
    errorString = QString( "avcodec_alloc_frame() failed." );
    return false;
  }

  AVPacket * packet = av_packet_alloc();

  while ( !Utils::AtomicInt::loadAcquire( isCancelled_ ) &&
          av_read_frame( formatContext_, packet ) >= 0 )
  {
    if ( packet->stream_index == audioStream_->index )
    {
      int ret = avcodec_send_packet( codecContext_, packet );
      /* read all the output frames (in general there may be any number of them) */
      while( ret >= 0 )
      {
        ret = avcodec_receive_frame( codecContext_, frame);

        if ( Utils::AtomicInt::loadAcquire( isCancelled_ ) || ret < 0 )
          break;

        playFrame( frame );
      }

    }

    av_packet_unref( packet );

  }

  /* flush the decoder */
  packet->data = NULL;
  packet->size = 0;
  int ret = avcodec_send_packet(codecContext_, packet );
  while( ret >= 0 )
  {
    ret = avcodec_receive_frame(codecContext_, frame);
    if ( Utils::AtomicInt::loadAcquire( isCancelled_ ) || ret < 0 )
      break;
    playFrame( frame );
  }
  av_frame_free( &frame );
  av_packet_free(&packet);
  return true;
}

bool DecoderContext::normalizeAudio( AVFrame * frame, vector<uint8_t > & samples )
{
  auto dst_freq     = 44100;
  auto dst_channels = codecContext_->channels;
  int out_count     = (int64_t) frame->nb_samples * dst_freq / frame->sample_rate + 256;
  int out_size      = av_samples_get_buffer_size( NULL, dst_channels, out_count, AV_SAMPLE_FMT_S16, 1 );
  samples.resize( out_size );
  uint8_t * data[ 2 ] = { 0 };
  data[ 0 ] = &samples.front();

  auto out_nb_samples = swr_convert( swr_, data, out_count, (const uint8_t **) frame->extended_data, frame->nb_samples );

  if( out_nb_samples < 0 )
  {
    av_log( NULL, AV_LOG_ERROR, "converte fail \n" );
    return false;
  }
  else
  {
//    qDebug( "out_count:%d, out_nb_samples:%d, frame->nb_samples:%d \n", out_count, out_nb_samples, frame->nb_samples );
  }

  int actual_size      = av_samples_get_buffer_size( NULL, dst_channels, out_nb_samples, AV_SAMPLE_FMT_S16, 1 );
  samples.resize(actual_size);
  return true;
}

void DecoderContext::playFrame( AVFrame * frame )
{
  if ( !frame )
    return;

  vector<uint8_t> samples;
  if ( normalizeAudio( frame, samples ) )
  {
    audioOutput->play(&samples.front(), samples.size());
  }
}

DecoderThread::DecoderThread( QByteArray const & audioData, QObject * parent ) :
  QThread( parent ),
  isCancelled_( 0 ),
  audioData_( audioData )
{
}

DecoderThread::~DecoderThread()
{
  isCancelled_.ref();
}

void DecoderThread::run()
{
  QString errorString;
  DecoderContext d( audioData_, isCancelled_ );

  if ( !d.openCodec( errorString ) )
  {
    emit error( errorString );
    return;
  }

  while ( !deviceMutex_.tryLock( 100 ) )
  {
    if ( Utils::AtomicInt::loadAcquire( isCancelled_ ) )
      return;
  }

  if ( !d.openOutputDevice( errorString ) )
    emit error( errorString );
  else if ( !d.play( errorString ) )
    emit error( errorString );

  d.closeOutputDevice();
  deviceMutex_.unlock();
}

void DecoderThread::cancel( bool waitUntilFinished )
{
  isCancelled_.ref();
  if ( waitUntilFinished )
    this->wait();
}

}

#endif // MAKE_FFMPEG_PLAYER
