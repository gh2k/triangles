#include "triangles.h"

#include "x11_undefs.h"

#include <QFileDialog>
#include <QDateTime>
#include <QThreadPool>
#include <QtConcurrent>
#include <QFuture>
#include <QMessageBox>
#include <QGraphicsPixmapItem>

#include "trianglescene.h"
#include "emberscene.h"
#include "faceweightedpixelsumfitness.h"
#include "randomiser.h"

Triangles::Triangles(QWidget *parent, Qt::WindowFlags flags)
    : QDialog(parent, flags)
{
  ui.setupUi(this);

  ui.currentCandidate->setScene( new QGraphicsScene( this ) );
  ui.bestCandidate->setScene( new QGraphicsScene( this ) );
  ui.target->setScene( new QGraphicsScene( this ) );

  ui.inputFrameGroup->setCurrentIndex( 0 );

  m_running = false;
  clear();

  m_oclWrapper.CheckOpenCL();

  connect( ui.reset, SIGNAL( clicked() ), this, SLOT( clear() ) );
  connect( ui.start, SIGNAL( clicked() ), this, SLOT( run() ) );
  connect( ui.stop, SIGNAL( clicked() ), this, SLOT( stop() ) );
  connect( ui.selectTarget, SIGNAL( clicked() ), this, SLOT( selectTarget() ) );
  connect( ui.useFlames, SIGNAL( toggled(bool) ), this, SLOT( setMethodFrame() ) );
  connect( ui.useTriangles, SIGNAL( toggled(bool) ), this, SLOT( setMethodFrame() ) );
  connect( ui.usePsfw, SIGNAL( toggled(bool) ), this, SLOT( setFitnessFrame() ) );
  connect( ui.usePs, SIGNAL( toggled(bool) ), this, SLOT( setFitnessFrame() ) );
  connect( ui.useSsim, SIGNAL( toggled(bool) ), this, SLOT( setFitnessFrame() ) );

  foreach( std::string platform, m_oclWrapper.PlatformNames() )
    ui.openclPlatform->addItem( QString::fromStdString( platform ) );

  populateDeviceList();

  QThreadPool::globalInstance()->setMaxThreadCount( 32 );
}

Triangles::~Triangles()
{

}

void Triangles::calculateFitnessForScene( const AbstractFitness *fitness, AbstractScene *scene )
{
  QImage candidateImage( fitness->target() );
  while ( ! scene->renderTo( candidateImage ) )
    scene->randomise();

  scene->setFitness( fitness->getFitness( candidateImage ) );
}

void Triangles::run()
{
  enum scenetype { TRIANGLES, EMBERS };

  scenetype scenetype = TRIANGLES;
  if ( ui.useFlames->isChecked() )
    scenetype = EMBERS;

  if ( scenetype == EMBERS )
  {
    if ( ! EmberScene::initialiseRenderer( ui.palettesFile->text(), ui.openclPlatform->currentIndex(), ui.openclDevice->currentIndex() ) )
    {
      QMessageBox::critical( this, "Derp!", "Couldn't initialise opencl renderer" );
      return;
    }
  }

  // initialise the candates to m_target, to match format and size
  m_bestCandidate = m_target;
  m_currentCandidate = m_target;

  // initialise everything
  int populationSize = ui.poolSize->value();

  m_currentFitness = -1;
  m_bestFitness = -1;

  float iterationsPerSec = 0;

  QList< AbstractScene* > previousAge;
  QList< AbstractScene* > nextAge;

  QDir logDir( m_imageFilename + ".triangles" );
  removeDir( m_imageFilename + ".triangles" );
  logDir.mkdir( m_imageFilename + ".triangles" );

  QFile bestScenesFile( logDir.absoluteFilePath( "bestScenes.log" ) );
  bestScenesFile.open( QFile::WriteOnly | QFile::Truncate );
  QDataStream bestScenes( &bestScenesFile );

  int faceWeight = ui.faceWeight->value();
  m_running = true;

  AbstractFitness *fitness = new FaceWeightedPixelSumFitness( m_target, faceWeight);

  int age = 0;
  int culture = 0;
  int maxCultures = 0;

  updateDialog( 0, 0, 0, 0, 0, 0, 0, 0 );

  AbstractScene *bestScene = 0;

  if ( scenetype == TRIANGLES )
    bestScene = new TriangleScene( 0, 0, 0, QColor() );
  if ( scenetype == EMBERS )
    bestScene = new EmberScene( 300, 300 );

  int iterations = 0;
  int maxIterations = 0;
  quint64 acceptCount = 0;
  int improvements = 0;

  logDir.remove( logDir.absoluteFilePath( "age." + QString::number( age ) + ".log" ) );

  // loop until the user tells us to stop
  while( m_running )
  {
    // each age simulates a number of cultures over a set number of iterations.
    // at the start of every new age, the best cultures from the previous age are selected and merged into a smaller number
    // of cultures by placing the best scene from each into a new scene pool

    // one culture simulates one attempt to find the best fitness from a given starting point over a certain number of iterations
    // each culture has a scene pool of a certaion size. for each iteration, the scene pool is mutated and scenes are cross-bred
    // with each other. the scenes with the best fitness survive to the next iteration.

    // this loop runs once per culture. age-management variables persist across runs

    // start by setting up the logs...
    QFile cultureLogFile( logDir.absoluteFilePath( "culture." + QString::number( age ) + "." + QString::number( culture ) + ".log" ) );
    cultureLogFile.open( QFile::WriteOnly | QFile::Truncate );
    QDataStream cultureLog( &cultureLogFile );

    QFile ageLogFile( logDir.absoluteFilePath( "age." + QString::number( age ) + ".log" ) );
    ageLogFile.open( QFile::WriteOnly | QFile::Append );
    QDataStream ageLog( &ageLogFile );

    // our pool of scenes for the current culture
    QList< AbstractScene* > pool;

    // the maximum number of cultures for the given age
    maxCultures = 0;
    // run many more iterations for future ages, as we hit diminishing returns
    maxIterations = ( ui.generationCount->value() * ( 1 << age ) );

    // run more cultures in earlier ages, so we can throw away the items with the lowest fitness more swiftly
    for( int i = 0; i < ui.maxAge->value() - age; ++ i )
    {
      if ( maxCultures == 0 )
        maxCultures = populationSize;
      else
        maxCultures *= 4;
    }
    if ( age == ui.maxAge->value() )
      maxIterations = 0;

    // set up the current pool
    m_currentFitness = -1;
    if ( previousAge.isEmpty() )
    {
      // if there's no previous age, we're in the first age so initialise the pool with random values
      for( int i = 0; i < populationSize; ++ i )
      {
        if ( scenetype == TRIANGLES )
          pool.append( new TriangleScene( ui.triangleCount->value(), m_target.width(), m_target.height(), QColor( 255, 255, 255, 255 ) ) );
        if ( scenetype == EMBERS )
          pool.append( new EmberScene( m_target.width(), m_target.height() ) );

        calculateFitnessForScene( fitness, pool[i] );
      }
    } else {
      // randomly take scenes from theprevious age to populate this one
      for( int i = 0; i < populationSize; ++ i )
      {
        pool.append( previousAge[ Randomiser::randomInt( previousAge.count() ) ]->clone() );
      }
    }

    // calculate the current and best fitness for this age, based on the new culture
    for( int i = 0; i < populationSize; ++ i )
    {
      if ( fitness->isBetterFitness( pool[i]->fitness(), m_currentFitness ) || m_currentFitness < 0 )
      {
        m_currentFitness = pool[i]->fitness();

        pool[i]->renderTo( m_currentCandidate );
      }
      if ( fitness->isBetterFitness( pool[i]->fitness(), m_bestFitness ) || m_bestFitness < 0 )
      {
        m_bestFitness = pool[i]->fitness();

        pool[i]->renderTo( m_bestCandidate );
      }
    }

    // update the dialog with our starting variables
    updateDialog( iterations, acceptCount, improvements, age, culture, maxCultures, maxIterations, iterationsPerSec );

    iterations = 0;
    acceptCount = 0;
    improvements = 0;

    QElapsedTimer timer;
    timer.start();

    // run the loop for the current culture (or indefinitely for the last culture)
    while ( ( maxIterations == 0 || iterations < maxIterations ) && m_running )
    {
      // set up containers for the current generation, and the next
      QSet< AbstractScene * > gen1( pool.toSet() );
      QList< AbstractScene * > gen2;

      QList< QFuture< void > > futures;

      // take scenes at random from the pool, in pairs...
      while( pool.count() )
      {
        AbstractScene *p1 = pool.takeAt( Randomiser::randomInt( pool.count() ) );
        AbstractScene *p2 = pool.takeAt( Randomiser::randomInt( pool.count() ) );
        // cross-breed and mutate the pair
        QPair< AbstractScene*, AbstractScene* > children = p1->breed( p2, ui.mutationStrength->value() );

        // run the fitness function for the newly-generated children using the thread pool
        futures << QtConcurrent::run( calculateFitnessForScene, fitness, children.first );
        futures << QtConcurrent::run( calculateFitnessForScene, fitness, children.second );

        // add both parents and both clildren to the next generation's pool
        gen2 << p1;
        gen2 << p2;
        gen2 << children.first;
        gen2 << children.second;
      }

      // wait for the fitness functions from this generation to complete
      while( futures.count() )
      {
        while( futures.last().isRunning() )
          futures.last().waitForFinished();
        futures.takeLast();
      }

      // sort the next generation by fitness
      qSort( gen2.begin(), gen2.end(), fitness->sceneHasBetterFitnessMethod() );

      // if the next generation has a better fitness than the current best fitness, update the candidate data
      if ( fitness->isBetterFitness( gen2.first()->fitness(), m_currentFitness ) )
      {
        ++ improvements;
        m_currentFitness = gen2.first()->fitness();
        if ( scenetype != EMBERS )
          gen2.first()->saveToStream( cultureLog );

        gen2.first()->renderTo( m_currentCandidate );

        if ( fitness->isBetterFitness( m_currentFitness, m_bestFitness ) )
        {
          m_bestFitness = m_currentFitness;
          delete bestScene;
          bestScene = gen2.first()->clone();
          bestScenes << iterations;
          bestScenes << m_currentFitness;
          if ( scenetype != EMBERS )
            bestScene->saveToStream( bestScenes );

          m_bestCandidate = m_currentCandidate;
        }
      }

      // populate the next pool
      for( int i = 0; i < populationSize; ++ i )
      {
        if ( i == 0 )
        {
          // always include the best candidate
          pool.append( gen2.takeFirst() );
        } else {
          // take other candidates at random from the best n results (where n is the tournament size) to keep the gene pool more varied
          int selection = Randomiser::randomInt( ui.tournamentSize->value() );
          AbstractScene *s = gen2.takeAt( selection );
          if ( ! gen1.contains( s ) )
          {
            ++ acceptCount;
          }
          pool.append( s );
        }
      }

      // clear the next pool and sort the current data, ready for another iteration
      qDeleteAll( gen2 );
      qSort( pool.begin(), pool.end(), fitness->sceneHasBetterFitnessMethod() );

      ++ iterations;

      iterationsPerSec = static_cast< float > ( iterations ) / static_cast< float > ( timer.elapsed() / 1000 );

      // update the window if we've covered enough iterations
      if ( iterations % ui.updateFrequency->value() == 0 )
      {
        updateDialog( iterations, acceptCount, improvements, age, culture, maxCultures, maxIterations, iterationsPerSec );
      }
    }

    // we've completed all the iterations for the culture...

    // write the best candidate to the age log, and place into the next age
    if ( scenetype != EMBERS )
      pool.first()->saveToStream( ageLog );
    nextAge.append( pool.takeFirst() );

    // clear the pool and advance to the next culture
    qDeleteAll( pool );
    ++ culture;

    // if we've been through all the cultures, advance to the next age
    if ( culture == maxCultures )
    {
      qDeleteAll( previousAge );
      previousAge = nextAge;
      nextAge.clear();
      culture = 0;
      ++ age;
      logDir.remove( logDir.absoluteFilePath( "age." + QString::number( age ) + ".log" ) );
    }

    updateDialog( iterations, acceptCount, improvements, age, culture, maxCultures, maxIterations, iterationsPerSec );
  }

  // the user has told us to stop...

  // update the screen
  updateDialog( iterations, acceptCount, improvements, age, culture, maxCultures, maxIterations, iterationsPerSec );

  // delete everyhing that's left
  qDeleteAll( nextAge );
  qDeleteAll( previousAge );

  // save the best scene to an svg
  bestScene->saveToFile( logDir.absoluteFilePath( "bestPicture.svg" ) );

  // iterate over the best scenes file, so that we can see the history of all improvements
  bestScenesFile.close();
  bestScenesFile.open( QFile::ReadOnly );
  QDataStream bestScenesLoader( &bestScenesFile );
  logDir.mkdir( "bestScenes" );
  QDir bsDir( logDir.absoluteFilePath( "bestScenes" ) );
  int count = 0;
  while( ! bestScenesLoader.atEnd() )
  {
    int iteration;
    float fitness;
    bestScenesLoader >> iteration;
    bestScenesLoader >> fitness;
    QString bsFilePath = bsDir.absoluteFilePath( QString( "%1.%2.svg" ).arg( count, 7, 10, QLatin1Char( '0' ) ).arg( iteration ) );

    AbstractScene *s = 0;

    if ( scenetype == TRIANGLES )
      s = new TriangleScene( ui.triangleCount->value(), m_target.width(), m_target.height(), QColor( 255, 255, 255, 255 ) );

    if ( scenetype == EMBERS )
      s = new EmberScene( m_target.width(), m_target.height() );

    if ( scenetype != EMBERS )
    {
      s->loadFromStream( bestScenesLoader );
      s->saveToFile( bsFilePath );
    }

    delete s;

    ++ count;
  }

  delete bestScene;

  // write out the best svgs for each individual culture
  QStringList filters;
  filters.append( "age.*.log" );
  //filters.append( "culture.*.log" );
  foreach( QString logEntry, logDir.entryList( filters ) )
  {
    logDir.mkdir( logEntry + ".d" );
    QDir entryDir( logDir.absoluteFilePath( logEntry + ".d" ) );
    QFile lf( logDir.absoluteFilePath( logEntry ) );
    lf.open( QFile::ReadOnly );
    QDataStream d( &lf );
    count = 0;
    while( !d.atEnd() )
    {
      QString fp = entryDir.absoluteFilePath( QString( "%1.svg" ).arg( count, 7, 10, QLatin1Char( '0' ) ) );
      AbstractScene *s = 0;

      if ( scenetype == TRIANGLES )
        s = new TriangleScene( ui.triangleCount->value(), m_target.width(), m_target.height(), QColor( 255, 255, 255, 255 ) );

      if ( scenetype == EMBERS )
        s = new EmberScene( m_target.width(), m_target.height() );

      s->loadFromStream( d );
      s->saveToFile( fp );
      delete s;

      ++ count;
    }
  }

  if ( scenetype == EMBERS )
    EmberScene::destroyRenderer();
}

void Triangles::updateDialog( int iterations, quint64 acceptCount, int improvements, int age, int culture, int maxCultures, int maxIterations, float iterationsPerSec )
{
  ui.iteration->setText( QString::number( iterations ) + "/" + QString::number( maxIterations ) );
  ui.acceptCount->setText( QString::number( acceptCount ) );
  ui.improvements->setText( QString::number( improvements ) );
  ui.age->setText( QString::number( age ) );
  ui.culture->setText( QString::number( culture ) + "/" + QString::number( maxCultures ) );
  ui.bestFitness->setText( QString::number( m_bestFitness ) );
  ui.currentFitness->setText( QString::number( m_currentFitness ) );
  ui.iterationsPerSec->setText( QString::number( iterationsPerSec) );
  updateCandidateView();
  qApp->processEvents();
}

void Triangles::stop()
{
  m_running = false;
}

void Triangles::clear()
{
  ui.iteration->setText( "0" );
  ui.triangleCount->setValue( 20 );
  ui.poolSize->setValue( 10 );
  ui.tournamentSize->setValue( 2 );
  ui.mutationStrength->setValue( 0 );
  ui.generationCount->setValue( 10000 );
  ui.faceWeight->setValue( 10 );
  ui.maxAge->setValue( 1 );
  ui.updateFrequency->setValue( 1 );
  ui.age->setText( "0" );
  ui.culture->setText( "0" );
  ui.currentFitness->setText( "0" );
  ui.bestFitness->setText( "0" );
  ui.acceptCount->setText( "0" );
  ui.improvements->setText( "0" );

  m_bestCandidate = QImage();
  m_currentCandidate = QImage();
  m_target = QImage();
  m_currentFitness = 0;
  m_bestFitness = 0;

  ui.target->scene()->clear();
  ui.bestCandidate->scene()->clear();
  ui.currentCandidate->scene()->clear();
}

void Triangles::selectTarget()
{
  QString imageFile( QFileDialog::getOpenFileName( this, "Select Image" ) );
  if ( imageFile.length() )
  {
    m_target.load( imageFile );
    ui.target->scene()->clear();

    if ( ! m_target.isNull() )
    {
      m_target = m_target.convertToFormat( QImage::Format_RGB32 );

      QGraphicsPixmapItem *item = ui.target->scene()->addPixmap( QPixmap::fromImage( m_target ) );
      ui.target->fitInView( item, Qt::KeepAspectRatio );
      m_imageFilename = imageFile;
    }
  }
}

void Triangles::resizeEvent( QResizeEvent *e )
{
  Q_UNUSED( e );

  QList< QGraphicsItem * > items( ui.target->scene()->items() );
  if ( items.count() == 1 )
    ui.target->fitInView( items.first(), Qt::KeepAspectRatio );

  items = ui.currentCandidate->scene()->items();
  if ( items.count() == 1 )
    ui.currentCandidate->fitInView( items.first(), Qt::KeepAspectRatio );

  items = ui.bestCandidate->scene()->items();
  if ( items.count() == 1 )
    ui.bestCandidate->fitInView( items.first(), Qt::KeepAspectRatio );
}

void Triangles::updateCandidateView()
{
  QGraphicsPixmapItem *item = 0;
  if ( ui.bestCandidate->scene()->items().count() == 0 )
    item = ui.bestCandidate->scene()->addPixmap( QPixmap::fromImage( m_bestCandidate ) );
  else
  {
    item = dynamic_cast< QGraphicsPixmapItem * > ( ui.bestCandidate->scene()->items().first() );
    if ( item )
    {
      item->setPixmap( QPixmap::fromImage( m_bestCandidate ) );
    }
  }
  ui.bestCandidate->fitInView( item, Qt::KeepAspectRatio );

  if ( ui.currentCandidate->scene()->items().count() == 0 )
    item = ui.currentCandidate->scene()->addPixmap( QPixmap::fromImage( m_currentCandidate ) );
  else
  {
    item = dynamic_cast< QGraphicsPixmapItem * > ( ui.currentCandidate->scene()->items().first() );
    if ( item )
    {
      item->setPixmap( QPixmap::fromImage( m_currentCandidate ) );
    }
  }
  ui.currentCandidate->fitInView( item, Qt::KeepAspectRatio );
}

bool Triangles::removeDir(const QString &dirName)
{
  bool result = true;
  QDir dir(dirName);

  if (dir.exists(dirName)) {
    Q_FOREACH(QFileInfo info, dir.entryInfoList(QDir::NoDotAndDotDot | QDir::System | QDir::Hidden  | QDir::AllDirs | QDir::Files, QDir::DirsFirst)) {
      if (info.isDir()) {
        result = removeDir(info.absoluteFilePath());
      }
      else {
        result = QFile::remove(info.absoluteFilePath());
      }

      if (!result) {
        return result;
      }
    }
    result = dir.rmdir(dirName);
  }

  return result;
}

void Triangles::setMethodFrame()
{
  if (ui.useTriangles->isChecked())
    ui.inputFrameGroup->setCurrentIndex(0);
  else
    ui.inputFrameGroup->setCurrentIndex(1);
}

void Triangles::setFitnessFrame()
{
  if (ui.usePsfw->isChecked())
    ui.fitnessFrameGroup->setCurrentIndex(0);
  if (ui.usePs->isChecked())
    ui.fitnessFrameGroup->setCurrentIndex(1);
  if (ui.useSsim->isChecked())
    ui.fitnessFrameGroup->setCurrentIndex(2);
}

void Triangles::populateDeviceList()
{
  ui.openclDevice->clear();

  foreach( std::string device, m_oclWrapper.DeviceNames( ui.openclPlatform->currentIndex() ) )
    ui.openclDevice->addItem( QString::fromStdString( device ) );
}
