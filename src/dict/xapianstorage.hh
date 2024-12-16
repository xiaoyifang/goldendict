/* This file is (c) 2008-2012 Konstantin Isakov <ikm@goldendict.org>
 * Part of GoldenDict. Licensed under GPLv3 or later, see the LICENSE file */

#pragma once

#include "xapian.h"
#include "ex.hh"
#include "dictfile.hh"

#include <vector>
#include <stdint.h>

/// xapian backend implementation
namespace XapianStorage {

using std::vector;

DEF_EX( Ex, "Chunked storage exception", std::exception )
DEF_EX( exFailedToCompressChunk, "Failed to compress a chunk", Ex )
DEF_EX( exAddressOutOfRange, "The given chunked address is out of range", Ex )
DEF_EX( exFailedToDecompressChunk, "Failed to decompress a chunk", Ex )
DEF_EX( mapFailed, "Failed to map/unmap the file", Ex )

/// This class writes data blocks in chunks.
class Writer
{
  Xapian::WritableDatabase db;
  Xapian::TermGenerator indexer;


public:
    explict Writer( std::string const & filePath );

    void addWord( std::string const & index_word, uint32_t articleOffset );

    ~Writer(){
      db.commit();
      db.close();
    }
};

/// This class reads data blocks previously written by Writer.
class Reader
{
  Xapian::Database db;

public:

  Reader( std::string const & filePath );

  QList< uint32_t > exactSearch( std::string const & index_word, uint32_t maxResults=1 );
  QList< uint32_t > fuzzySearch( std::string const & index_word, uint32_t maxResults=10 );
  QList< uint32_t > prefixSearch( std::string const & index_word, uint32_t maxResults=10 );

  QList< uint32_t > suffixSearch( std::string const & index_word, uint32_t maxResults=10 );

private:
  QList< uint32_t > search( std::string const & index_word, int flag, uint32_t maxResults = 10 );
};

} // namespace XapianStorage
