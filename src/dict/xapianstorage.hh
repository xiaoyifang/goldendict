/* This file is (c) 2008-2012 Konstantin Isakov <ikm@goldendict.org>
 * Part of GoldenDict. Licensed under GPLv3 or later, see the LICENSE file */

#pragma once

#include "xapian.h"

#include <stdint.h>

/// xapian backend implementation
namespace XapianStorage {

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
