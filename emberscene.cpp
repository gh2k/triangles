#include "emberscene.h"

#include <EmberToXml.h>
#include <XmlToEmber.h>

#include <QFile>

QMutex EmberScene::s_renderMutex;
EmberCLns::RendererCL<EMBER_PRECISION> *EmberScene::s_renderer = 0;
EmberNs::SheepTools<EMBER_PRECISION, EMBER_PRECISION> *EmberScene::s_tools = 0;

EmberScene::EmberScene( int width, int height )
  :AbstractScene()
{
  m_width = width;
  m_height = height;

  m_ember.m_OrigFinalRasW = width;
  m_ember.m_OrigFinalRasH = height;

  m_ember.m_TemporalSamples = 1;
  m_ember.m_Quality = 50;

  m_ember.m_OrigPixPerUnit = 20;

  randomise();
}

EmberScene::EmberScene( const EmberScene &other )
  :AbstractScene( other ), m_ember( other.m_ember )
{
  m_width = other.m_width;
  m_height = other.m_height;
}

AbstractScene *EmberScene::clone() const
{
  return new EmberScene( *this );
}

EmberScene::~EmberScene()
{

}

void EmberScene::randomise()
{
  if ( s_tools )
    s_tools->Random( m_ember );
  else
    throw EmberRendererNotInitialisedException();

  m_ember.m_FinalRasW = m_width;
  m_ember.m_FinalRasH = m_height;
  m_ember.m_PixelsPerUnit = 20;
}

QPair< AbstractScene*, AbstractScene* > EmberScene::breed( AbstractScene *other, int mutationStrength )
{
  // create two new scenes from this and one other, merging data from the two and
  // mutating some parameters

  EmberScene *left = new EmberScene( m_width, m_height );
  EmberScene *right = new EmberScene( m_width, m_height );

  EmberScene *emberOther = dynamic_cast< EmberScene* > ( other );

  s_tools->Cross( this->m_ember, emberOther->m_ember, left->m_ember, CROSS_NOT_SPECIFIED );
  s_tools->Cross( emberOther->m_ember, this->m_ember, right->m_ember, CROSS_NOT_SPECIFIED );

  left->mutate( mutationStrength );
  right->mutate( mutationStrength );

  return qMakePair< AbstractScene*, AbstractScene* > ( left, right );
}

void EmberScene::mutateOnce()
{
  // mutate ALL THE THINGS!
  std::vector<EmberNs::eVariationId> vars;
  for ( int i = VAR_ARCH; i <= VAR_ZTRANSLATE; ++ i )
    vars.push_back( static_cast< EmberNs::eVariationId > ( i ) );

  if ( s_tools )
    s_tools->Mutate( m_ember, MUTATE_NOT_SPECIFIED, vars, 0, 0.1 );
  else
    throw EmberRendererNotInitialisedException();
}

void EmberScene::saveToFile( const QString &fn )
{
  QFile f( fn );
  if ( f.open( QFile::WriteOnly | QFile::Truncate ) )
  {
    EmberNs::EmberToXml<EMBER_PRECISION> serializer;
    serializer.Save( fn.toStdString(), m_ember, 0, false, false, false );
//    std::string s;
//    s = serializer.ToString( m_ember, 0, false, false, true );
//    QString os;
//    if ( s.length() )
//      os = QString::fromStdString( s );
//    f.write( os.toUtf8() );
//    f.close();
   }
}

void EmberScene::saveToStream( QDataStream &stream )
{
  // I'm worried that this might be too slow :/

  EmberNs::EmberToXml<EMBER_PRECISION> serializer;
  stream << m_width;
  stream << m_height;
  std::string s( serializer.ToString( m_ember, 0, false, false, true ) );
  QString os = QString::fromStdString( s );
  stream << os;
}

void EmberScene::loadFromStream( QDataStream &stream )
{
  // I'm worried that this might be too slow :/

  QString s;
  std::vector<EmberNs::Ember<EMBER_PRECISION>> embers;

  stream >> m_width;
  stream >> m_height;
  stream >> s;

  QByteArray bs( s.toUtf8() );
  bs.append( static_cast< char > ( 0 ) );

  EmberNs::XmlToEmber<EMBER_PRECISION> serializer;
  serializer.Parse( reinterpret_cast< byte * > ( bs.data() ), "", embers );
  m_ember = embers[0];
}

bool EmberScene::renderTo( QImage &image )
{
  bool failed = true;

  if ( s_renderer )
  {
    s_renderMutex.lock();

    static vector<byte> imgBytes;
    if ( imgBytes.size() == 0 )
      imgBytes.resize( m_width * m_height * 4, 0xff0000ff );

    s_renderer->SetEmber( m_ember );
    if ( s_renderer->Run( imgBytes ) == RENDER_OK )
    {
      // I don't know if this is async or not, but treating it as such
      while ( s_renderer->ProcessState() != ACCUM_DONE )
        usleep(50);

      failed = false;
    }

    s_renderMutex.unlock();

    if ( failed )
    {
      image = QImage( m_width, m_height, QImage::Format_ARGB32 );
      image.fill( Qt::black );
      qDebug( "Ember rendering failed" );
    } else {
      qDebug( "Ember rendering success" );
      image = QImage( imgBytes.data(), m_width, m_height, QImage::Format_ARGB32 );
    }
  } else {
    throw EmberRendererNotInitialisedException();
  }

  return !failed;
}

void EmberScene::destroyRenderer()
{
  // deleting tools deletes the renderer
  if ( s_tools )
    delete s_tools;
  else
    delete s_renderer;

  s_renderer = 0;
  s_tools = 0;
}

bool EmberScene::initialiseRenderer( const QString &palettePath, int platform, int device )
{
  destroyRenderer();

  s_renderer = new EmberCLns::RendererCL<EMBER_PRECISION>( platform, device );
  s_tools = new EmberNs::SheepTools<EMBER_PRECISION, EMBER_PRECISION>( palettePath.toStdString(), s_renderer );

  return true;
}
