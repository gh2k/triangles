#include "scene.h"

#include <QPainter>
#include <QPicture>
#include <QImage>
#include <QSvgGenerator>
#include "randomiser.h"

scene::scene( int polyCount, int width, int height, const QColor &backgroundColor )
  :m_polys( polyCount )
{
  for( int i = 0; i < polyCount; ++ i )
    m_polys[i] = new poly( width, height );
  m_width = width;
  m_height = height;
  m_backgroundColor = backgroundColor;
  m_fitness = 0;
}

scene::scene( const scene &s )
  :m_polys( s.m_polys.size() )
{
  for( int i = 0; i < m_polys.size(); ++ i )
    m_polys[i] = new poly( *s.m_polys[i] );
  m_width = s.m_width;
  m_height = s.m_height;
  m_backgroundColor = s.m_backgroundColor;
  m_fitness = s.m_fitness;
}

scene::~scene()
{
  for( int i = 0; i < m_polys.size(); ++ i )
    delete m_polys[i];
}

void scene::mutate( int mutationStrength )
{
  m_fitness = 0;
  do
  {
    int p = randomiser::randomInt( m_polys.size() );
    int type = randomiser::randomInt( poly::Randomize );

    if ( type < poly::SwapZ )
    {
      int other = randomiser::randomInt( m_polys.size() );
      poly *o = m_polys[other];
      m_polys[other] = m_polys[p];
      m_polys[p] = o;
    }
    else
    {
      m_polys[p]->mutate( (poly::MutationType) type );
    }
  } while ( randomiser::randomInt( 100 ) < mutationStrength ) ;
}

void scene::renderTo( QImage &image )
{
  QPainter painter( &image );
  painter.fillRect( 0, 0, m_width, m_height, m_backgroundColor );
  renderTo( painter );
}

void scene::renderTo( QPicture &picture )
{
  QPainter painter( &picture );
  renderTo( painter );
}

void scene::renderTo( QPainter &painter )
{
  painter.setPen( Qt::NoPen );

  for( int t = 0; t < m_polys.size(); ++ t )
  {
    m_polys[t]->renderTo( painter );
  }
}

scene &scene::operator=( const scene &s )
{
  for( int i = 0; i < m_polys.size(); ++ i )
    delete m_polys[i];
  m_polys.resize( s.m_polys.size() );
  for( int i = 0; i < m_polys.size(); ++ i )
    m_polys[i] = new poly( *s.m_polys[i] );
  m_width = s.m_width;
  m_height = s.m_height;
  m_backgroundColor = s.m_backgroundColor;
  m_fitness = s.m_fitness;

  return *this;
}

QPair< scene*, scene* > scene::breed( scene &other, int mutationStrength )
{
  scene *left = new scene( 0, m_width, m_height, m_backgroundColor );
  scene *right = new scene( 0, m_width, m_height, m_backgroundColor );

  left->m_polys.resize( m_polys.size() );
  right->m_polys.resize( m_polys.size() );

  for( int i = 0; i < m_polys.size(); ++ i )
  {
    left->m_polys[i] = new poly( m_width, m_height );
    right->m_polys[i] = new poly( m_width, m_height );
    poly::uniformCrossover( left->m_polys[i], right->m_polys[i], m_polys[i], other.m_polys[i] );
  }

  left->mutate( mutationStrength );
  right->mutate( mutationStrength );

  return qMakePair< scene*, scene* > ( left, right );
}

void scene::saveToSvg( const QString &fn )
{
  QSvgGenerator gen;
  gen.setFileName( fn );
  gen.setSize( QSize( m_width, m_height ) );
  gen.setViewBox( QRect( QPoint( 0, 0 ), QSize( m_width, m_height ) ) );
  QPainter svgPainter( &gen );
  renderTo( svgPainter );
}

QDataStream &operator << ( QDataStream &ds, const scene &s )
{
  ds << s.m_backgroundColor;
  ds << s.m_width;
  ds << s.m_height;
  ds << s.m_fitness;
  for( int i = 0; i < s.m_polys.size(); ++ i )
    ds << *s.m_polys[i];

  return ds;
}

QDataStream &operator >> ( QDataStream &ds, scene &s )
{
  ds >> s.m_backgroundColor;
  ds >> s.m_width;
  ds >> s.m_height;
  ds >> s.m_fitness;
  for( int i = 0; i < s.m_polys.size(); ++ i )
    ds >> *s.m_polys[i];

  return ds;
}