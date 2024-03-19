/* This file is (c) 2013 Timon Wong <timon86.wang@gmail.com>
 * Part of GoldenDict. Licensed under GPLv3 or later, see the LICENSE file */
#pragma once
#ifndef NO_TTS_SUPPORT

  #include "dictionary.hh"
  #include "config.hh"
  #include "wstring.hh"

  #include <QCryptographicHash>


namespace VoiceEngines {

using std::vector;
using std::string;
using gd::wstring;

vector< sptr< Dictionary::Class > > makeDictionaries( Config::VoiceEngines const & voiceEngines );

} // namespace VoiceEngines

#endif
