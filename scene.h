#ifndef SCENE_H
#define SCENE_H

#include "poly.h"
#include <QVector>
#include <QDataStream>
#include <QColor>
#include <QPair>

class QImage;
class QPicture;

class scene
{
  friend QDataStream &operator << ( QDataStream &ds, const scene &s );
  friend QDataStream &operator >> ( QDataStream &ds, scene &s );

public:
  scene( int polyCount, int width, int height, const QColor &backgroundColor );
  scene( const scene &other );
  virtual ~scene();

  void mutate( int mutationStrength );
  QPair< scene*, scene* > breed( scene &other, int mutationStrength );

  double fitness() { return m_fitness; }
  void setFitness( double _fitness ) { m_fitness = _fitness; }

  scene &operator = ( const scene &other );
  void renderTo( QImage &image );
  void renderTo( QPicture &image );
  void renderTo( QPainter &image );
  void saveToSvg( const QString &fn );

private:

  QVector< poly* > m_polys;
  QColor m_backgroundColor;
  int m_width;
  int m_height;
  double m_fitness;
};

#endif //SCENE_H