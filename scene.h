#ifndef SCENE_H
#define SCENE_H

#include "poly.h"
#include <QVector>
#include <QDataStream>
#include <QColor>
#include <QPair>

class QImage;
class QPicture;

/** A scene defines a collection of triangles used to create a single image */

class scene
{
  friend QDataStream &operator << ( QDataStream &ds, const scene &s );
  friend QDataStream &operator >> ( QDataStream &ds, scene &s );

public:
  /// randomly initialise a scene with a set number of triangles, a set size and background colour
  scene( int polyCount, int width, int height, const QColor &backgroundColor );
  /// copy from another scene
  scene( const scene &other );
  virtual ~scene();

  /// randomly change some variables in the scene
  /// \param mutationStrength is a number between 0 and 99, which
  /// defines the likelihood of the randomisation to be re-run after it ends
  /// (can execute an arbitrary number of times)
  void mutate( int mutationStrength );

  /// cross-breeds this scene with another one
  QPair< scene*, scene* > breed( scene &other, int mutationStrength );

  /// returns the cached fitness for this scene
  double fitness() { return m_fitness; }
  /// sets the cached fitness for this scene
  void setFitness( double _fitness ) { m_fitness = _fitness; }

  /// copy from another scene
  scene &operator = ( const scene &other );

  // rendering methods
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
