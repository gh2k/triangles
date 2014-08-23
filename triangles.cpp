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
  m_bestCandidate = m_target;
  m_currentCandidate = m_target;
  int populationSize = ui.poolSize->value();

  m_currentFitness = -1;
  m_bestFitness = -1;

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

  updateDialog( 0, 0, 0, 0, 0, 0, 0 ); 

  scene bestScene( 0, 0, 0, QColor() );

  int iterations = 0;
  int maxIterations = 0;
  int acceptCount = 0;
  int improvements = 0;

  logDir.remove( logDir.absoluteFilePath( "age." + QString().setNum( age ) + ".log" ) );

  while( m_running )
  {
    QFile cultureLogFile( logDir.absoluteFilePath( "culture." + QString().setNum( age ) + "." + QString().setNum( culture ) + ".log" ) );
    cultureLogFile.open( QFile::WriteOnly | QFile::Truncate );
    QDataStream cultureLog( &cultureLogFile );

    QFile ageLogFile( logDir.absoluteFilePath( "age." + QString().setNum( age ) + ".log" ) );
    ageLogFile.open( QFile::WriteOnly | QFile::Append );
    QDataStream ageLog( &ageLogFile );

    QList< scene* > pool;

    maxCultures = 0;
    maxIterations = ( ui.generationCount->value() * ( 1 << age ) );
    for( int i = 0; i < ui.maxAge->value() - age; ++ i )
    {
      if ( maxCultures == 0 )
        maxCultures = populationSize;
      else
        maxCultures *= 4;
    }
    if ( age == ui.maxAge->value() )
      maxIterations = 0;

    m_currentFitness = -1;
    if ( previousAge.isEmpty() )
    {
      for( int i = 0; i < populationSize; ++ i )
      {
        pool.append( new scene( ui.triangleCount->value(), m_target.width(), m_target.height(), QColor( 255, 255, 255, 255 ) ) );
        pool[i]->renderTo( m_currentCandidate );
        pool[i]->setFitness( getFitness( m_currentCandidate, m_target, m_pixelWeights, m_faceWeight ) );
      }
    } else {
      for( int i = 0; i < populationSize; ++ i )
      {
        pool.append( new scene( *previousAge[ randomiser::randomInt( previousAge.count() ) ] ) );
      }
    }

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

    updateDialog( iterations, acceptCount, improvements, age, culture, maxCultures, maxIterations );

    iterations = 0;
    acceptCount = 0;
    improvements = 0;

    while ( ( maxIterations == 0 || iterations < maxIterations ) && m_running )
    {
      QSet< scene * > gen1( pool.toSet() );
      QList< scene * > gen2;
      QList< QFuture< void > > futures;
      while( pool.count() )
      {
        scene *p1 = pool.takeAt( randomiser::randomInt( pool.count() ) );
        scene *p2 = pool.takeAt( randomiser::randomInt( pool.count() ) );
        QPair< scene*, scene* > children = p1->breed( *p2, ui.mutationStrength->value() );

        futures << QtConcurrent::run( calculateFitnessForScene, children.first, m_target, m_pixelWeights, m_faceWeight );
        futures << QtConcurrent::run( calculateFitnessForScene, children.second, m_target, m_pixelWeights, m_faceWeight );
        gen2 << p1;
        gen2 << p2;
        gen2 << children.first;
        gen2 << children.second;
      }

      while( futures.count() )
      {
        while( futures.last().isRunning() )
          futures.last().waitForFinished();
        futures.takeLast();
      }

      qSort( gen2.begin(), gen2.end(), sceneHasBetterFitness );

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

      for( int i = 0; i < populationSize; ++ i )
      {
        if ( i == 0 )
        {
          pool.append( gen2.takeFirst() );
        } else {
          int selection = randomiser::randomInt( ui.tournamentSize->value() );
          scene *s = gen2.takeAt( selection );
          if ( ! gen1.contains( s ) )
          {
            ++ acceptCount;
          }
          pool.append( s );
        }
      }

      qDeleteAll( gen2 );
      qSort( pool.begin(), pool.end(), sceneHasBetterFitness );

      ++ iterations;

      if ( iterations % ui.updateFrequency->value() == 0 )
      {
        updateDialog( iterations, acceptCount, improvements, age, culture, maxCultures, maxIterations );
      }
    }

    ageLog << *pool.first();
    nextAge.append( pool.takeFirst() );
    qDeleteAll( pool );
    ++ culture;

    if ( culture == maxCultures )
    {
      qDeleteAll( previousAge );
      previousAge = nextAge;
      nextAge.clear();
      culture = 0;
      ++ age;
      logDir.remove( logDir.absoluteFilePath( "age." + QString().setNum( age ) + ".log" ) );
    }

    updateDialog( iterations, acceptCount, improvements, age, culture, maxCultures, maxIterations );
  }

  updateDialog( iterations, acceptCount, improvements, age, culture, maxCultures, maxIterations );

  qDeleteAll( nextAge );
  qDeleteAll( previousAge );

  bestScene.saveToSvg( logDir.absoluteFilePath( "bestPicture.svg" ) );

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

void triangles::updateDialog( int iterations, int acceptCount, int improvements, int age, int culture, int maxCultures, int maxIterations )
{
  ui.iteration->setText( QString().setNum( iterations ) + "/" + QString().setNum( maxIterations ) );
  ui.acceptCount->setText( QString().setNum( acceptCount ) );
  ui.improvements->setText( QString().setNum( improvements ) );
  ui.age->setText( QString().setNum( age ) );
  ui.culture->setText( QString().setNum( culture ) + "/" + QString().setNum( maxCultures ) );
  ui.bestFitness->setText( QString().setNum( m_bestFitness ) );
  ui.currentFitness->setText( QString().setNum( m_currentFitness ) );
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

    for( int x = 0; x < w; ++ x )
    {
      double dB = (double)(*(targetLine++) - *(candidateLine++));
      double dG = (double)(*(targetLine++) - *(candidateLine++));
      double dR = (double)(*(targetLine++) - *(candidateLine++));
      ++targetLine; ++candidateLine;

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
