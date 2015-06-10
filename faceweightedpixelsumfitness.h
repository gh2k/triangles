#ifndef FACEWEIGHTEDPIXELSUMFITNESS_H
#define FACEWEIGHTEDPIXELSUMFITNESS_H

#include "abstractfitness.h"
#include <QVector>

class FaceWeightedPixelSumFitness : public AbstractFitness {
public:
  FaceWeightedPixelSumFitness( const QImage &image, int faceWeight );
  virtual ~FaceWeightedPixelSumFitness() {}

  /// renders a scene and calcuates the similarity to the target image
  float getFitness( const QImage &image ) const;

  virtual SceneComparisonFunction sceneHasBetterFitnessMethod() const { return AbstractFitness::sceneHasBetterFitness; }

private:
  void doFaceDetection();

  QVector< unsigned char * > m_pixelWeights;
  int m_faceWeight;

  QImage m_faceMask;
  QList< QRect > m_faces;
};

#endif // FACEWEIGHTEDPIXELSUMFITNESS_H

