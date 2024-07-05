/* This file is (c) 2012 Tvangeste <i.4m.l33t@yandex.ru>
 * Part of GoldenDict. Licensed under GPLv3 or later, see the LICENSE file */

#include "mainstatusbar.hh"

#include <Qt>
#include <QFrame>
#include <QVBoxLayout>
#include <QEvent>
#include <QApplication>
#include <QPainterPath>

MainStatusBar::MainStatusBar( QWidget * parent ):
  QWidget( parent )
{
  //round corners
  QPainterPath PainterPath;
  PainterPath.addRoundedRect( this->rect(), 2, 0 );
  QRegion mask = QRegion( PainterPath.toFillPolygon().toPolygon() );
  this->setMask( mask );

  textWidget = new QLabel( QString(), this );
  textWidget->setObjectName( "text" );
  textWidget->setFont( QApplication::font( "QStatusBar" ) );
  textWidget->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Fixed );

  picWidget = new QLabel( QString(), this );
  picWidget->setObjectName( "icon" );
  picWidget->setPixmap( QPixmap() );
  picWidget->setScaledContents( true );
  picWidget->setSizePolicy( QSizePolicy::Fixed, QSizePolicy::Ignored );

  timer = new QTimer( this );
  timer->setSingleShot( true );

  // layout
  const auto layout = new QHBoxLayout;
  layout->setSpacing( 0 );
  layout->setSizeConstraint( QLayout::SetFixedSize );
  layout->setAlignment( Qt::AlignLeft | Qt::AlignVCenter );
  layout->setContentsMargins( 2, 2, 2, 2 );
  layout->addWidget( picWidget );
  layout->addWidget( textWidget );
  setLayout( layout );

  parentWidget()->installEventFilter( this );

  connect( timer, &QTimer::timeout, this, &MainStatusBar::clearMessage );

  setAutoFillBackground( true );
}

void MainStatusBar::paintEvent(QPaintEvent *event)
{
    QPainterPath path;
    path.addRoundedRect(rect(), 2, 0);
    QRegion mask = QRegion(path.toFillPolygon().toPolygon());
    setMask(mask);

    QWidget::paintEvent(event);
}

void MainStatusBar::clearMessage()
{
  message.clear();
  textWidget->setText( backgroungMessage );
  picWidget->setPixmap( QPixmap() );
  timer->stop();
}

QString MainStatusBar::currentMessage() const
{
  return message;
}

void MainStatusBar::setBackgroundMessage( const QString & bkg_message )
{
  backgroungMessage = bkg_message;
  if ( message.isEmpty() ) {
    textWidget->setText( backgroungMessage );
  }
}

void MainStatusBar::showMessage( const QString & str, int timeout, const QPixmap & pixmap )
{
  textWidget->setText( message = str );
  picWidget->setPixmap( pixmap );

  if ( !picWidget->pixmap().isNull() ) {
    picWidget->setFixedSize( textWidget->height(), textWidget->height() );
  }
  else {
    picWidget->setFixedSize( 0, 0 );
  }

  if ( timeout > 0 ) {
    timer->start( timeout );
  }

  if ( parentWidget() ) {
    raise();

    move( 0, parentWidget()->height() - height() );
  }
}

void MainStatusBar::mousePressEvent( QMouseEvent * )
{
  clearMessage();
}

bool MainStatusBar::eventFilter( QObject *, QEvent * e )
{
  switch ( e->type() ) {
    case QEvent::Resize:
      raise();

      move( QPoint( 0, parentWidget()->height() - height() ) );
      break;
    default:
      break;
  }

  return false;
}
