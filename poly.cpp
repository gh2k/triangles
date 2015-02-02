#include "poly.h"

#include <QPainter>
#include <QPolygon>
#include <QColor>

#include "randomiser.h"

Poly::Poly( int width, int height )
{
  m_width = width;
  m_height = height;

  m_color = QColor( Randomiser::randomInt( 255 ), Randomiser::randomInt( 255 ), Randomiser::randomInt( 255 ), Randomiser::randomInt( 255 ) );
  m_points.resize( 3 );

  for( int i = 0; i < m_points.size(); ++ i )
  {
    m_points[i] = QPoint( Randomiser::randomInt( width ), Randomiser::randomInt( height ) );
  }
}

Poly::Poly( const Poly &other )
{
  m_width = other.m_width;
  m_height = other.m_height;
  m_points = other.m_points;
  m_color = other.m_color;
}

Poly::~Poly()
{

}

Poly &Poly::operator = ( const Poly &other )
{
  m_width = other.m_width;
  m_height = other.m_height;
  m_points = other.m_points;
  m_color = other.m_color;
  return *this;
}

void Poly::renderTo( QPainter &painter )
{
  painter.setBrush( QBrush( m_color ) );
  painter.drawPolygon( QPolygon( m_points ) );
}

void Poly::mutate( MutationType mt )
{
  if( mt < MoveCorner )
  {
    int corner = Randomiser::randomInt( m_points.size() );
    m_points[corner] = QPoint( Randomiser::randomInt( m_width ), Randomiser::randomInt( m_height ) );
  }
  else if ( mt < TweakChannel )
  {
    int channel = Randomiser::randomInt( 4 );
    switch ( channel )
    {
    case 0:
      m_color.setRed( Randomiser::randomInt( 255 ) );
      break;
    case 1:
      m_color.setGreen( Randomiser::randomInt( 255 ) );
      break;
    case 2:
      m_color.setBlue( Randomiser::randomInt( 255 ) );
      break;
    case 3:
      m_color.setAlpha( Randomiser::randomInt( 255 ) );
      break;
    }
  }
  else if ( mt < Relocate )
  {
    for( int i = 0; i < m_points.size(); ++ i )
      m_points[i] = QPoint( Randomiser::randomInt( m_width ), Randomiser::randomInt( m_height ) );
  }
  else if ( mt < Randomize )
  {
    for( int i = 0; i < m_points.size(); ++ i )
      m_points[i] = QPoint( Randomiser::randomInt( m_width ), Randomiser::randomInt( m_height ) );
    m_color = QColor( Randomiser::randomInt( 255 ), Randomiser::randomInt( 255 ), Randomiser::randomInt( 255 ), Randomiser::randomInt( 255 ) );
  }
}

void Poly::uniformCrossover( Poly *d1, Poly *d2, Poly *s1, Poly *s2 )
{
  for( int i = 0; i < s1->m_points.size() + 4; ++ i )
  {
    Poly *cd1 = d1;
    Poly *cd2 = d2;
    if ( Randomiser::randomInt( 2 ) == 0 )
    {
      cd1 = d2;
      cd2 = d1;
    }
    switch( i )
    {
    case 0:
      cd1->m_color.setRed( s1->m_color.red() );
      cd2->m_color.setRed( s2->m_color.red() );
      break;
    case 1:
      cd1->m_color.setGreen( s1->m_color.green() );
      cd2->m_color.setGreen( s2->m_color.green() );
      break;
    case 2:
      cd1->m_color.setBlue( s1->m_color.blue() );
      cd2->m_color.setBlue( s2->m_color.blue() );
      break;
    case 3:
      cd1->m_color.setAlpha( s1->m_color.alpha() );
      cd2->m_color.setAlpha( s2->m_color.alpha() );
      break;
    default:
      cd1->m_points[ i - 4 ] = s1->m_points[ i - 4 ];
      cd2->m_points[ i - 4 ] = s2->m_points[ i - 4 ];
      break;
    }
  }
}

QDataStream &operator << ( QDataStream &ds, const Poly &p )
{
  ds << p.m_points;
  ds << p.m_color;
  return ds;
}

QDataStream &operator >> ( QDataStream &ds, Poly &p )
{
  ds >> p.m_points;
  ds >> p.m_color;
  return ds;
}