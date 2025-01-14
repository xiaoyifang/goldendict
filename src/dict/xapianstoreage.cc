#include "xapianstorage.hh"
#include <string>


namespace XapianStorage {

Writer::Writer( QString & filename ):
  _filename( filename )
{
  try {
    std::string dbPath = fileName.toStdString();
    _db                = Xapian::WritableDatabase( dbPath + "_temp", Xapian::DB_CREATE_OR_OPEN );
    std::cout << "Database opened successfully at: " << dbPath << std::endl;
  }
  catch ( const Xapian::Error & e ) {
    std::cerr << "Xapian error: " << e.get_msg() << std::endl;
    throw;
  }
  catch ( const std::exception & e ) {
    std::cerr << "Standard exception: " << e.what() << std::endl;
    throw;
  }
  catch ( ... ) {
    std::cerr << "Unknown exception" << std::endl;
    throw;
  }
}

void Writer::addDocument( uint32_t offset, std::string const & word )
{
  Xapian::TermGenerator indexer;
  //  Xapian::Stem stemmer("english");
  //  indexer.set_stemmer(stemmer);
  //  indexer.set_stemming_strategy(indexer.STEM_SOME_FULL_POS);
  indexer.set_flags( Xapian::TermGenerator::FLAG_CJK_NGRAM );

  Xapian::Document doc;

  indexer.set_document( doc );

  indexer.index_text( word );

  doc.set_data( std::to_string( offset ) );
  // Add the document to the database.
  _db.add_document( doc );
}

uint32_t Writer::commit()
{
  _db.commit();

  auto file = filename.toStdString();
  _db.compact( file );

  _db.close();
  _isClosed = true;

  Utils::Fs::removeDirectory( file + "_temp" );
}

Writer::~Writer()
{
  try {
    if(!_isClosed)
    {
      _db.close();
    }
  }
  catch ( ... ) {
  }
}

Reader::Reader( QString & filename ):
  _filename( filename )
{
  _db = Xapian::Database( filename.toStdString() );
}

QList< uint32_t > Reader::getDocument( std::string const & searchString )
{
  try {
    // Start an enquire session.
    Xapian::Enquire enquire( _db );

    // Combine the rest of the command line arguments with spaces between
    // them, so that simple queries don't have to be quoted at the shell
    // level.
    string query_string( searchString.toStdString() );

    // Parse the query string to produce a Xapian::Query object.
    Xapian::QueryParser qp;
    qp.set_database( _db );
    qp.set_default_op( Xapian::Query::op::OP_AND );
    int flag =
      Xapian::QueryParser::FLAG_DEFAULT | Xapian::QueryParser::FLAG_PURE_NOT | Xapian::QueryParser::FLAG_CJK_NGRAM;
    if ( searchMode == FTS::Wildcards ) {
      flag = flag | Xapian::QueryParser::FLAG_WILDCARD;
      qp.set_max_expansion( 1 );
    }
    Xapian::Query query = qp.parse_query( query_string, flag );
    qDebug() << "Parsed query is: " << query.get_description().c_str();

    // Find the top 100 results for the query.
    enquire.set_query( query );
    Xapian::MSet matches = enquire.get_mset( 0, 100 );

    // Display the results.
    qDebug() << matches.get_matches_estimated() << " results found.\n";
    qDebug() << "Matches " << matches.size() << ":\n\n";
    QList< uint32_t > offsetsForHeadwords;
    for ( Xapian::MSetIterator i = matches.begin(); i != matches.end(); ++i ) {
      qDebug() << i.get_rank() + 1 << ": " << i.get_weight() << " docid=" << *i << " ["
               << i.get_document().get_data().c_str() << "]";
      
      offsetsForHeadwords.append( atoi( i.get_document().get_data().c_str() ) );
    }


    return offsetsForHeadwords;
  }
  catch ( const Xapian::Error & e ) {
    qWarning() << e.get_description().c_str();
  }
  catch ( std::exception & ex ) {
    qWarning( "FTS: Failed full-text search for \"%s\", reason: %s", dict.getName().c_str(), ex.what() );
    // Results not loaded -- we don't set the hasAnyData flag then
  }
}

Reader::~Reader()
{
  try {
    _db.close();
  }
  catch ( ... ) {
  }
}

} // namespace ChunkedStorage
