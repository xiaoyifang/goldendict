#pragma once

/// @file
/// Handling global hotkeys and some tricks
/// Part of this header are implmented in
/// + `winhotkeywrapper.cc`
/// + `machotkeywrapper.hh`
///

#include <QGuiApplication>
#include <QThread>

#include "config.hh"
#include "ex.hh"
#include "qtsingleapplication.h"
#include "utils.hh"

#ifdef HAVE_X11

  #include <set>

  #include <X11/Xlib.h>
  #include <X11/extensions/record.h>
  #if ( QT_VERSION < QT_VERSION_CHECK( 6, 0, 0 ) )
    #include <QX11Info>
  #endif
  #include <X11/Xlibint.h>

  #undef Bool
  #undef min
  #undef max

#endif

#ifdef Q_OS_WIN
  #include <QAbstractNativeEventFilter>
#endif

#ifdef Q_OS_MAC
  #import <Carbon/Carbon.h>
#endif

//////////////////////////////////////////////////////////////////////////

struct HotkeyStruct
{
  HotkeyStruct() = default;
  HotkeyStruct( quint32 key, quint32 key2, quint32 modifier, int handle, int id );

  quint32 key      = 0;
  quint32 key2     = 0;
  quint32 modifier = 0;
  int handle       = 0;
  int id           = 0;
#ifdef Q_OS_MAC
  EventHotKeyRef hkRef  = 0;
  EventHotKeyRef hkRef2 = 0;
#endif
};

//////////////////////////////////////////////////////////////////////////

class HotkeyWrapper: public QThread // Thread is actually only used on X11
{
  Q_OBJECT

  friend class QHotkeyApplication;

public:

  DEF_EX( exInit, "Hotkey wrapper failed to init", std::exception )

  HotkeyWrapper( QObject * parent );
  virtual ~HotkeyWrapper();

  /// The handle is passed back in hotkeyActivated() to inform which hotkey
  /// was activated.
  bool setGlobalKey( int key, int key2, Qt::KeyboardModifiers modifier, int handle );

  bool setGlobalKey( QKeySequence const &, int );

  /// Unregisters everything
  void unregister();

signals:

  void hotkeyActivated( int );

protected slots:

  void waitKey2();

#ifndef Q_OS_MAC
private slots:

  bool checkState( quint32 vk, quint32 mod );
#endif

private:

  void init();
  quint32 nativeKey( int key );

  QList< HotkeyStruct > hotkeys;

  bool state2;
  HotkeyStruct state2waiter;

#ifdef Q_OS_WIN32
  virtual bool winEvent( MSG * message, qintptr * result );
  HWND hwnd;
#endif

#ifdef Q_OS_MAC

public:
  void activated( int hkId );

private:
  void sendCmdC();

  static EventHandlerUPP hotKeyFunction;
  quint32 keyC;
  EventHandlerRef handlerRef;
#endif

#ifdef HAVE_X11
  static void recordEventCallback( XPointer, XRecordInterceptData * );

  /// Called by recordEventCallback()
  void handleRecordEvent( XRecordInterceptData * );

  void run(); // QThread

  // We do one-time init of those, translating keysyms to keycodes
  KeyCode lShiftCode, rShiftCode, lCtrlCode, rCtrlCode, lAltCode, rAltCode, cCode, insertCode, kpInsertCode, lMetaCode,
    rMetaCode;

  quint32 currentModifiers;

  Display * dataDisplay;
  XRecordRange * recordRange;
  XRecordContext recordContext;
  XRecordClientSpec recordClientSpec;

  /// Holds all the keys currently grabbed.
  /// The first value is keycode, the second is modifiers
  using GrabbedKeys = std::set< std::pair< quint32, quint32 > >;
  GrabbedKeys grabbedKeys;

  GrabbedKeys::iterator keyToUngrab; // Used for second stage grabs

  /// Returns true if the given key is usually used to copy from clipboard,
  /// false otherwise.
  bool isCopyToClipboardKey( quint32 keyCode, quint32 modifiers ) const;
  /// Returns true if the given key is grabbed, false otherwise
  bool isKeyGrabbed( quint32 keyCode, quint32 modifiers ) const;
  /// Grabs the given key, recording the fact in grabbedKeys. If the key's
  /// already grabbed, does nothing.
  /// Returns the key's iterator in grabbedKeys.
  GrabbedKeys::iterator grabKey( quint32 keyCode, quint32 modifiers );
  /// Ungrabs the given key. erasing it from grabbedKeys. The key's provided
  /// as an interator inside the grabbedKeys set.
  void ungrabKey( GrabbedKeys::iterator );

signals:

  /// Emitted from the thread
  void keyRecorded( quint32 vk, quint32 mod );

#endif
};

//////////////////////////////////////////////////////////////////////////

class DataCommitter
{
public:

  virtual void commitData( QSessionManager & ) = 0;
  virtual ~DataCommitter() {}
};

class QHotkeyApplication: public QtSingleApplication
#if defined( Q_OS_WIN )
  ,
                          public QAbstractNativeEventFilter
#endif
{
  Q_OBJECT

  friend class HotkeyWrapper;

  QList< DataCommitter * > dataCommitters;

public:
  QHotkeyApplication( int & argc, char ** argv );
  QHotkeyApplication( QString const & id, int & argc, char ** argv );

  void addDataCommiter( DataCommitter & );
  void removeDataCommiter( DataCommitter & );

private slots:
  /// This calls all data committers.
  void hotkeyAppCommitData( QSessionManager & );

  void hotkeyAppSaveState( QSessionManager & );

protected:
  void registerWrapper( HotkeyWrapper * wrapper );
  void unregisterWrapper( HotkeyWrapper * wrapper );

#ifdef Q_OS_WIN
  virtual bool nativeEventFilter( const QByteArray & eventType, void * message, qintptr * result );
#endif

  QList< HotkeyWrapper * > hotkeyWrappers;
};

//////////////////////////////////////////////////////////////////////////
