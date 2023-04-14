/* This file is (c) 2008-2012 Konstantin Isakov <ikm@goldendict.org>
 * Part of GoldenDict. Licensed under GPLv3 or later, see the LICENSE file */

#ifndef __FSENCODING_HH_INCLUDED__
#define __FSENCODING_HH_INCLUDED__

#include "wstring.hh"
#include <QString>

/// Utilities to convert a wide string or an utf8 string to the local 8bit
/// encoding of the file system, and to do other manipulations on the file
/// names.
namespace FsEncoding {

using std::string;

/// Returns the filesystem separator (/ on Unix and clones, \ on Windows).
char separator();

/// Returns the name part of the given filename.
string basename( string const & );

}

#endif
