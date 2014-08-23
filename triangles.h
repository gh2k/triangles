#ifndef TRIANGLES_H
#define TRIANGLES_H

#include <QDialog>
#include "ui_triangles.h"

#include <QImage>

#include "scene.h"

class triangles : public QDialog
{
  Q_OBJECT

public:
  triangles(QWidget *parent = 0, Qt::WindowFlags flags = 0);
  ~triangles();

protected:

  virtual void resizeEvent( QResizeEvent *e );

private slots:
  void run();
  void selectTarget();
  void clear();
  void stop();

private:

  static bool removeDir(const QString &dirName);
  
  static double getFitness( const QImage &candidate, const QImage &target, const QVector< unsigned char * > &pixelWeights, int faceWeight );

  void updateCandidateView();
  void updateDialog( int iterations, int acceptCount, int improvements, int age, int culture, int maxCultures, int maxIterations );

  static void calculateFitnessForScene( scene *scene, const QImage &target, const QVector< unsigned char * > &pixelWeights, int faceWeight );

  inline static bool sceneHasBetterFitness( scene *a, scene *b ) { return a->fitness() < b->fitness(); }
  
  Ui::trianglesClass ui;

  QImage m_bestCandidate;
  QImage m_currentCandidate;
  QImage m_target;
  QImage m_targetRender;

  QImage m_faceMask;
  QList< QRect > m_faces;
  int m_faceWeight;

  QVector< unsigned char * > m_pixelWeights;

  QString m_imageFilename;

  bool m_running;
  double m_bestFitness;
  double m_currentFitness;

};

#endif // TRIANGLES_H
