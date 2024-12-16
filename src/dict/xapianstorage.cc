/* This file is (c) 2008-2012 Konstantin Isakov <ikm@goldendict.org>
 * Part of GoldenDict. Licensed under GPLv3 or later, see the LICENSE file */

#include "xapianstorage.hh"
#include <zlib.h>
#include <string.h>
#include <QDataStream>
#include <QScopeGuard>
#include <QMutexLocker>

namespace XapianStorage {

Writer::Writer( std::string const & filePath ):
  db( filePath, Xapian::DB_CREATE_OR_OPEN )
{
  //  Xapian::Stem stemmer("english");
  //  indexer.set_stemmer(stemmer);
  //  indexer.set_stemming_strategy(indexer.STEM_SOME_FULL_POS);
  indexer.set_flags( Xapian::TermGenerator::FLAG_CJK_NGRAM );
}

void Writer::addWord( std::string const & index_word, uint32_t articleOffset )
{
  Xapian::Document doc;
  indexer.set_document( doc );
  indexer.index_text( index_word );
  doc.set_data( std::to_string( address ) );
  db.add_document( doc );
}
Reader::Reader( std::string const & filePath ):
  db( filePath )
{
  
}

QList< uint32_t > Reader::exactSearch( std::string const & index_word, uint32_t maxResults )
{
  int flag = Xapian::QueryParser::FLAG_PHRASE;

  return search( query_string, flag, maxResults );
}


QList< uint32_t > Reader::search( std::string const & index_word,int flag, uint32_t maxResults )
{
  // Start an enquire session.
  Xapian::Enquire enquire( db );

  // Combine the rest of the command line arguments with spaces between
  // them, so that simple queries don't have to be quoted at the shell
  // level.
  string query_string( index_word.toStdString() );

  // Parse the query string to produce a Xapian::Query object.
  Xapian::QueryParser qp;
  qp.set_database( db );
  qp.set_default_op( Xapian::Query::op::OP_AND );

  Xapian::Query query = qp.parse_query( query_string, flag );
  qDebug() << "Parsed query is: " << query.get_description().c_str();

  // Find the top 100 results for the query.
  enquire.set_query( query );
  Xapian::MSet matches = enquire.get_mset( 0, maxResults );

  // Display the results.
  qDebug() << matches.get_matches_estimated() << " results found.\n";
  qDebug() << "Matches " << matches.size() << ":\n\n";
  QList< uint32_t > offsetsForHeadwords;
  for ( Xapian::MSetIterator i = matches.begin(); i != matches.end(); ++i ) {
    qDebug() << i.get_rank() + 1 << ": " << i.get_weight() << " docid=" << *i << " ["
             << i.get_document().get_data().c_str() << "]";
    if ( i.get_document().get_data() == finish_mark ) {
      continue;
    }
    offsetsForHeadwords.append( atoi( i.get_document().get_data().c_str() ) );
  }

  return offsetsForHeadwords;
}

} // namespace ChunkedStorage
