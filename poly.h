#ifndef POLY_H
#define POLY_H

#include <QVector>
#include <QPoint>
#include <QColor>

class QPainter;

/** Althogh called 'poly', this class actually describes a triangle to be rendered into a scene */

class poly
{
public:
  friend QDataStream &operator << ( QDataStream &ds, const poly &s );
  friend QDataStream &operator >> ( QDataStream &ds, poly &s );

  /// defines ways the triangle can be mutated, with probabilities
  enum MutationType {
    /// swaps the poly with another, in the z-order
    SwapZ = 20,
    /// randomises the position of one corner
    MoveCorner = 50,
    /// changes one of the RGBA values for the triangle
    TweakChannel = 80,
    /// randomises the position of all corners
    Relocate = 90,
    /// reinitialises all values with random data
    Randomize = 100 
  };

  /// initialise a random triagle within a scene bounded by width and height
  poly( int width, int height );
  /// initialise a triangle from another
  poly( const poly &other );
  virtual ~poly();

  /// mutates the triangle, based on the mutation type supplied
  void mutate( MutationType mutationType );
  /// renders this triange to the specified painter, as part of a scene
  void renderTo( QPainter &painter );

  /// assigns the data from another triangle to this one
  poly &operator = ( const poly &other );

  /// merges the data from two triangles, to breed a new one
  /// randomly selects values from either d1 or d2 into s1, and copies
  /// the values from the opposite triangle into s2
  static void uniformCrossover( poly *d1, poly *d2, poly *s1, poly *s2 );

private:
  QVector< QPoint > m_points;
  QColor m_color;

  int m_width;
  int m_height;
};

#endif //POLY_H
