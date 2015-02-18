#ifndef SCENE_H
#define SCENE_H

#include "poly.h"
#include <QVector>
#include <QDataStream>
#include <QColor>
#include <QPair>

#include "abstractscene.h"

class QImage;
class QPicture;

/** A scene defines a collection of triangles used to create a single image */

class TriangleScene : public AbstractScene
{
public:
  /// randomly initialise a scene with a set number of triangles, a set size and background colour
  TriangleScene( int polyCount, int width, int height, const QColor &backgroundColor );
  /// copy from another scene
  TriangleScene( const TriangleScene &other );
  virtual ~TriangleScene();

  /// cross-breeds this scene with another one
  virtual QPair< AbstractScene*, AbstractScene* > breed( AbstractScene *other, int mutationStrength );

  // rendering methods
  virtual bool renderTo( QImage &image );
  void drawTo( QPicture &image );
  void drawTo( QPainter &image );
  virtual void saveToFile( const QString &fn );

  virtual void randomise();

  virtual AbstractScene *clone() const;

  /// dumps the scene to a datastream for later processing
  virtual void saveToStream( QDataStream &stream );

  /// loads the scene from a datastream for later processing
  virtual void loadFromStream( QDataStream &stream );

protected:
  virtual void mutateOnce();

private:

  QVector< Poly* > m_polys;
  QColor m_backgroundColor;
  int m_width;
  int m_height;
};

#endif //SCENE_H
