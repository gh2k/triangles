#ifndef ABSTRACTFITNESS_H
#define ABSTRACTFITNESS_H

#include "abstractscene.h"
class QImage;

typedef bool (*SceneComparisonFunction)(const AbstractScene *a, const AbstractScene *b);

class AbstractFitness {
public:
  AbstractFitness( const QImage &target ) : m_target( target ) {}
  virtual ~AbstractFitness() {}

  /// renders a scene and calcuates the similarity to the target image
  virtual float getFitness( const QImage &image ) const = 0;

  /// compares two scenes, and returns true if scene a has better fitness
  /// (basically, we need to know if higher or lower is better)
  static bool sceneHasBetterFitness( const AbstractScene *a, const AbstractScene *b ) {
    return a->fitness() < b->fitness();
  }
  virtual bool isBetterFitness( float a, float b ) {
    return a < b;
  }

  virtual SceneComparisonFunction sceneHasBetterFitnessMethod() const = 0;

  inline const QImage &target () const { return m_target; }

protected:
  inline QImage &target() { return m_target; }

private:
  QImage m_target;
};

#endif // ABSTRACTFITNESS_H

