#ifndef TRIANGLES_H
#define TRIANGLES_H

#include <QDialog>
#include "ui_triangles.h"

#include <QImage>

#include "trianglescene.h"

#include "EmberCommonPch.h"

/** Main dialog that runs all of the top-level logic and displays progres */

class Triangles : public QDialog
{
  Q_OBJECT

public:
  Triangles(QWidget *parent = 0, Qt::WindowFlags flags = 0);
  ~Triangles();

protected:

  virtual void resizeEvent( QResizeEvent *e );

private slots:
  /// top-level method that runs the whole thing
  void run();

  /// selects the target image via a file dialog
  void selectTarget();
  /// resets all values to defaults
  void clear();
  /// tells the simulation to stop
  void stop();

  /// sets the correct input frame on radio button selection
  void setFrame();

  /// populates the opencl device list when the platform is changed
  void populateDeviceList();

private:

  /// removes a directory, recursively
  static bool removeDir(const QString &dirName);
  
  /// calculates the fitness for a particular image (how similar it is to the candidate image)
  static double getFitness( const QImage &candidate, const QImage &target, const QVector< unsigned char * > &pixelWeights, int faceWeight );

  /// re-renders the current candidate, to show progress
  void updateCandidateView();

  /// updates all progress variables on the main dialog, and triggers an update of the candidate view
  void updateDialog( int iterations, quint64 acceptCount, int improvements, int age, int culture, int maxCultures, int maxIterations, double iterationsPerSec );

  /// renders a scene and calcuates the similarity to the target image
  static void calculateFitnessForScene( AbstractScene *TriangleScene, const QImage &target, const QVector< unsigned char * > &pixelWeights, int faceWeight );

  /// compares two scenes, and returns true if scene a has better fitnesss
  inline static bool sceneHasBetterFitness( AbstractScene *a, AbstractScene *b ) { return a->fitness() < b->fitness(); }
  
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

  OpenCLWrapper m_oclWrapper;

};

#endif // TRIANGLES_H
