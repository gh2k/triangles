#include "triangles.h"

#include <QFileDialog>
#include <QGraphicsPixmapItem>
#include <QDateTime>
#include <QThreadPool>
#include <QtConcurrent/QtConcurrent>
#include <QFuture>
#include <QPicture>
#include <QPainter>
#include <QSvgGenerator>
#include <QMessageBox>

#include "scene.h"
#include "facedetect.h"
#include "randomiser.h"

triangles::triangles(QWidget *parent, Qt::WindowFlags flags)
    : QDialog(parent, flags)
{
  ui.setupUi(this);

  ui.currentCandidate->setScene( new QGraphicsScene( this ) );
  ui.bestCandidate->setScene( new QGraphicsScene( this ) );
  ui.target->setScene( new QGraphicsScene( this ) );

  m_running = false;
  clear();

  connect( ui.reset, SIGNAL( clicked() ), this, SLOT( clear() ) );
  connect( ui.start, SIGNAL( clicked() ), this, SLOT( run() ) );
  connect( ui.stop, SIGNAL( clicked() ), this, SLOT( stop() ) );
  connect( ui.selectTarget, SIGNAL( clicked() ), this, SLOT( selectTarget() ) );

  QThreadPool::globalInstance()->setMaxThreadCount( 32 );
}

triangles::~triangles()
{

}

void triangles::calculateFitnessForScene( scene *scene, const QImage &target, const QVector< unsigned char * > &pixelWeights, int faceWeight )
{
  QImage candidateImage( target );
  scene->renderTo( candidateImage );
  scene->setFitness( getFitness( candidateImage, target, pixelWeights, faceWeight ) );
}

void triangles::run()
{
  // initialise the candates to m_target, to match format and size
  m_bestCandidate = m_target;
  m_currentCandidate = m_target;

  // initialise everything
  int populationSize = ui.poolSize->value();

  m_currentFitness = -1;
  m_bestFitness = -1;

  double iterationsPerSec = 0;

  QList< scene* > previousAge;
  QList< scene* > nextAge;

  QDir logDir( m_imageFilename + ".triangles" );
  removeDir( m_imageFilename + ".triangles" );
  logDir.mkdir( m_imageFilename + ".triangles" );

  QFile bestScenesFile( logDir.absoluteFilePath( "bestScenes.log" ) );
  bestScenesFile.open( QFile::WriteOnly | QFile::Truncate );
  QDataStream bestScenes( &bestScenesFile );

  m_faceWeight = ui.faceWeight->value();
  m_running = true;

  int age = 0;
  int culture = 0;
  int maxCultures = 0;

  updateDialog( 0, 0, 0, 0, 0, 0, 0, 0 );

  scene bestScene( 0, 0, 0, QColor() );

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
    QList< scene* > pool;

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
        pool.append( new scene( ui.triangleCount->value(), m_target.width(), m_target.height(), QColor( 255, 255, 255, 255 ) ) );
        pool[i]->renderTo( m_currentCandidate );
        pool[i]->setFitness( getFitness( m_currentCandidate, m_target, m_pixelWeights, m_faceWeight ) );
      }
    } else {
      // randomly take scenes from theprevious age to populate this one
      for( int i = 0; i < populationSize; ++ i )
      {
        pool.append( new scene( *previousAge[ randomiser::randomInt( previousAge.count() ) ] ) );
      }
    }

    // calculate the current and best fitness for this age, based on the new culture
    for( int i = 0; i < populationSize; ++ i )
    {
      if ( pool[i]->fitness() < m_currentFitness || m_currentFitness < 0 )
      {
        m_currentFitness = pool[i]->fitness();
        pool[i]->renderTo( m_currentCandidate );
      }
      if ( pool[i]->fitness() < m_bestFitness || m_bestFitness < 0 )
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
      QSet< scene * > gen1( pool.toSet() );
      QList< scene * > gen2;

      QList< QFuture< void > > futures;

      // take scenes at random from the pool, in pairs...
      while( pool.count() )
      {
        scene *p1 = pool.takeAt( randomiser::randomInt( pool.count() ) );
        scene *p2 = pool.takeAt( randomiser::randomInt( pool.count() ) );
        // cross-breed and mutate the pair
        QPair< scene*, scene* > children = p1->breed( *p2, ui.mutationStrength->value() );

        // run the fitness function for the newly-generated children using the thread pool
        futures << QtConcurrent::run( calculateFitnessForScene, children.first, m_target, m_pixelWeights, m_faceWeight );
        futures << QtConcurrent::run( calculateFitnessForScene, children.second, m_target, m_pixelWeights, m_faceWeight );

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
      qSort( gen2.begin(), gen2.end(), sceneHasBetterFitness );

      // if the next generation has a better fitness than the current best fitness, update the candidate data
      if ( gen2.first()->fitness() < m_currentFitness )
      {
        ++ improvements;
        m_currentFitness = gen2.first()->fitness();
        cultureLog << *gen2.first();

        gen2.first()->renderTo( m_currentCandidate );

        if ( m_currentFitness < m_bestFitness )
        {
          m_bestFitness = m_currentFitness;
          bestScene = *gen2.first();
          bestScenes << iterations;
          bestScenes << m_currentFitness;
          bestScenes << *gen2.first();

          gen2.first()->renderTo( m_bestCandidate );
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
          int selection = randomiser::randomInt( ui.tournamentSize->value() );
          scene *s = gen2.takeAt( selection );
          if ( ! gen1.contains( s ) )
          {
            ++ acceptCount;
          }
          pool.append( s );
        }
      }

      // clear the next pool and sort the current data, ready for another iteration
      qDeleteAll( gen2 );
      qSort( pool.begin(), pool.end(), sceneHasBetterFitness );

      ++ iterations;

      iterationsPerSec = static_cast< double > ( iterations ) / static_cast< double > ( timer.elapsed() / 1000 );

      // update the window if we've covered enough iterations
      if ( iterations % ui.updateFrequency->value() == 0 )
      {
        updateDialog( iterations, acceptCount, improvements, age, culture, maxCultures, maxIterations, iterationsPerSec );
      }
    }

    // we've completed all the iterations for the culture...

    // write the best candidate to the age log, and place into the next age
    ageLog << *pool.first();
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
  bestScene.saveToSvg( logDir.absoluteFilePath( "bestPicture.svg" ) );

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
    double fitness;
    bestScenesLoader >> iteration;
    bestScenesLoader >> fitness;
    QString bsFilePath = bsDir.absoluteFilePath( QString( "%1.%2.svg" ).arg( count, 7, 10, QLatin1Char( '0' ) ).arg( iteration ) );
    scene s( ui.triangleCount->value(), m_target.width(), m_target.height(), QColor( 255, 255, 255, 255 ) );
    bestScenesLoader >> s;
    s.saveToSvg( bsFilePath );
    ++ count;
  }

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
      scene s( ui.triangleCount->value(), m_target.width(), m_target.height(), QColor( 255, 255, 255, 255 ) );
      d >> s;
      s.saveToSvg( fp );
      ++ count;
    }
  }
}

void triangles::updateDialog( int iterations, quint64 acceptCount, int improvements, int age, int culture, int maxCultures, int maxIterations, double iterationsPerSec )
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

void triangles::stop()
{
  m_running = false;
}

void triangles::clear()
{
  ui.iteration->setText( "0" );
  ui.triangleCount->setValue( 20 );
  ui.poolSize->setValue( 100 );
  ui.tournamentSize->setValue( 15 );
  ui.mutationStrength->setValue( 50 );
  ui.generationCount->setValue( 10000 );
  ui.faceWeight->setValue( 10 );
  ui.maxAge->setValue( 3 );
  ui.updateFrequency->setValue( 10 );
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

void triangles::selectTarget()
{
  QString imageFile( QFileDialog::getOpenFileName( this, "Select Image" ) );
  if ( imageFile.length() )
  {
    m_target.load( imageFile );
    ui.target->scene()->clear();

    if ( ! m_target.isNull() )
    {
      m_target = m_target.convertToFormat( QImage::Format_RGB32 );
      m_targetRender = m_target.convertToFormat( QImage::Format_RGB32 );
      m_faces = detectFaces( m_target );

      QPainter p( &m_targetRender );
      p.setBrush( Qt::NoBrush );
      p.setPen( QPen( QColor( 255, 0, 0 ), 4.0 ) );
      for( int i = 0; i < m_faces.count(); ++ i )
        p.drawEllipse( m_faces.at( i ) );

      QGraphicsPixmapItem *item = ui.target->scene()->addPixmap( QPixmap::fromImage( m_targetRender ) );
      ui.target->fitInView( item, Qt::KeepAspectRatio );
      m_imageFilename = imageFile;

      m_faceMask = QImage( m_target.size(), QImage::Format_RGB32 );
      m_faceMask.fill( 0 );
      QPainter pm( &m_faceMask );
      pm.setPen( Qt::NoPen );
      pm.setBrush( QColor( 255, 255, 255 ) );
      for( int i = 0; i < m_faces.count(); ++ i )
        pm.drawEllipse( m_faces.at( i ) );

      m_pixelWeights.resize( m_faceMask.height() );
      for( int y = 0; y < m_faceMask.height(); ++ y )
      {
        m_pixelWeights[y] = new unsigned char[m_faceMask.width()];
        for( int x = 0; x < m_faceMask.width(); ++ x )
        {
          m_pixelWeights[y][x] = qRed(m_faceMask.pixel(x, y)) > 0 ? 1 : 0;
        }
      }
    }
  }
}

void triangles::resizeEvent( QResizeEvent *e )
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

double triangles::getFitness( const QImage &candidate, const QImage &target, const QVector< unsigned char * > &pixelWeights, int faceWeight )
{
  double f = 0;
  int w = target.width();
  int h = target.height();
  for( int y = 0; y < h; ++ y )
  {
    const uchar *targetLine = target.scanLine( y );
    const uchar *candidateLine = candidate.scanLine( y );
    const uchar *weights = pixelWeights[y];

    // calculates the difference between pixels on the two images
    for( int x = 0; x < w; ++ x )
    {
      double dB = (double)(*(targetLine++) - *(candidateLine++));
      double dG = (double)(*(targetLine++) - *(candidateLine++));
      double dR = (double)(*(targetLine++) - *(candidateLine++));
      ++targetLine; ++candidateLine;

      // larger number is better. multiplies pixels detected as part of a face by faceWeight
      // this gives a better weighting to face pixels and will accept candidates that have a better
      // match for those areas
      double pixelFitness = (dR * dR + dG * dG + dB * dB) * (double)(faceWeight * weights[x] + 1 );

      f += pixelFitness;
    }
  }

  return f;
}

void triangles::updateCandidateView()
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

bool triangles::removeDir(const QString &dirName)
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
