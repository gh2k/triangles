#include "trianglescene.h"

#include <QPainter>
#include <QPicture>
#include <QImage>
#include <QSvgGenerator>
#include "randomiser.h"

TriangleScene::TriangleScene( int polyCount, int width, int height, const QColor &backgroundColor )
  :AbstractScene(), m_polys( polyCount )
{
  for( int i = 0; i < polyCount; ++ i )
    m_polys[i] = new Poly( width, height );
  m_width = width;
  m_height = height;
  m_backgroundColor = backgroundColor;
}

TriangleScene::TriangleScene( const TriangleScene &s )
  :m_polys( s.m_polys.size() )
{
  for( int i = 0; i < m_polys.size(); ++ i )
    m_polys[i] = new Poly( *s.m_polys[i] );
  m_width = s.m_width;
  m_height = s.m_height;
  m_backgroundColor = s.m_backgroundColor;
  setFitness( s.fitness() );
}

TriangleScene::~TriangleScene()
{
  for( int i = 0; i < m_polys.size(); ++ i )
    delete m_polys[i];
}

void TriangleScene::mutateOnce()
{
  // loop until mutationStrenth says we should stop

  // randomly select a triangle to modify
  int p = Randomiser::randomInt( m_polys.size() );
  // randomly select the type of modification
  int type = Randomiser::randomInt( Poly::Randomize );

  // if we're swapping z order, move to a different position in the list
  if ( type < Poly::SwapZ )
  {
    int other = Randomiser::randomInt( m_polys.size() );
    Poly *o = m_polys[other];
    m_polys[other] = m_polys[p];
    m_polys[p] = o;
  }
  else
  {
    // otherwise mutate the triangle
    m_polys[p]->mutate( (Poly::MutationType) type );
  }
}

void TriangleScene::renderTo( QImage &image )
{
  QPainter painter( &image );
  painter.fillRect( 0, 0, m_width, m_height, m_backgroundColor );
  drawTo( painter );
}

void TriangleScene::drawTo( QPicture &picture )
{
  QPainter painter( &picture );
  drawTo( painter );
}

void TriangleScene::drawTo( QPainter &painter )
{
  painter.setPen( Qt::NoPen );

  for( int t = 0; t < m_polys.size(); ++ t )
  {
    m_polys[t]->renderTo( painter );
  }
}

AbstractScene *TriangleScene::clone() const
{
  TriangleScene *ts = new TriangleScene( *this );

  return ts;
}

QPair< AbstractScene*, AbstractScene* > TriangleScene::breed( AbstractScene *other, int mutationStrength )
{
  // create two new scenes from this and one other, merging data from the two and
  // mutating some parameters

  TriangleScene *left = new TriangleScene( 0, m_width, m_height, m_backgroundColor );
  TriangleScene *right = new TriangleScene( 0, m_width, m_height, m_backgroundColor );

  left->m_polys.resize( m_polys.size() );
  right->m_polys.resize( m_polys.size() );

  for( int i = 0; i < m_polys.size(); ++ i )
  {
    left->m_polys[i] = new Poly( m_width, m_height );
    right->m_polys[i] = new Poly( m_width, m_height );
    Poly::uniformCrossover( left->m_polys[i], right->m_polys[i], m_polys[i], dynamic_cast< TriangleScene* > ( other )->m_polys[i] );
  }

  left->mutate( mutationStrength );
  right->mutate( mutationStrength );

  return qMakePair< AbstractScene*, AbstractScene* > ( left, right );
}

void TriangleScene::saveToFile( const QString &fn )
{
  QSvgGenerator gen;
  gen.setFileName( fn );
  gen.setSize( QSize( m_width, m_height ) );
  gen.setViewBox( QRect( QPoint( 0, 0 ), QSize( m_width, m_height ) ) );
  QPainter svgPainter( &gen );
  drawTo( svgPainter );
}

void TriangleScene::saveToStream ( QDataStream &ds )
{
  ds << m_backgroundColor;
  ds << m_width;
  ds << m_height;
  ds << fitness();
  for( int i = 0; i < m_polys.size(); ++ i )
    ds << *m_polys[i];
}

void TriangleScene::loadFromStream ( QDataStream &ds )
{
  ds >> m_backgroundColor;
  ds >> m_width;
  ds >> m_height;
  double f;
  ds >> f;
  setFitness( f );
  for( int i = 0; i < m_polys.size(); ++ i )
    ds >> *m_polys[i];
}
