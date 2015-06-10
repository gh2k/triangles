#ifndef TRIANGLES_H
#define TRIANGLES_H

#include <QDialog>
#include "ui_triangles.h"

#include <QImage>

#include "abstractscene.h"
#include "abstractfitness.h"
#include <qmath.h>

#include <OpenCLWrapper.h>

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
  void setMethodFrame();

  /// sets the correct input frame on radio button selection
  void setFitnessFrame();

  /// populates the opencl device list when the platform is changed
  void populateDeviceList();

private:

  /// removes a directory, recursively
  static bool removeDir(const QString &dirName);

  /// re-renders the current candidate, to show progress
  void updateCandidateView();

  /// updates all progress variables on the main dialog, and triggers an update of the candidate view
  void updateDialog( int iterations, quint64 acceptCount, int improvements, int age, int culture, int maxCultures, int maxIterations, float iterationsPerSec );
  
  /// calculates the fitness for a scene, and stores the fitness value within the scene
  static void calculateFitnessForScene( const AbstractFitness *fitness, AbstractScene *scene );

  Ui::trianglesClass ui;

  QImage m_bestCandidate;
  QImage m_currentCandidate;
  QImage m_target;
  QImage m_targetRender;

  QString m_imageFilename;

  bool m_running;
  float m_bestFitness;
  float m_currentFitness;

  EmberCLns::OpenCLWrapper m_oclWrapper;

};

#endif // TRIANGLES_H
