#ifndef POLY_H
#define POLY_H

#include <QVector>
#include <QPoint>
#include <QColor>

class QPainter;

class poly
{
public:
  friend QDataStream &operator << ( QDataStream &ds, const poly &s );
  friend QDataStream &operator >> ( QDataStream &ds, poly &s );

  enum MutationType {
    SwapZ = 20,
    MoveCorner = 50,
    TweakChannel = 80,
    Relocate = 90,
    Randomize = 100 
  };

  poly( int width, int height );
  poly( const poly &other );
  virtual ~poly();

  void mutate( MutationType mutationType );
  void renderTo( QPainter &painter );

  poly &operator = ( const poly &other );

  static void uniformCrossover( poly *d1, poly *d2, poly *s1, poly *s2 );

private:
  QVector< QPoint > m_points;
  QColor m_color;

  int m_width;
  int m_height;
};

#endif //POLY_H
