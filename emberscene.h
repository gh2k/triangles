#ifndef EMBERSCENE_H
#define EMBERSCENE_H

#include "abstractscene.h"

#include <Ember.h>
#include <SheepTools.h>
#include <RendererCL.h>
#include <OpenCLWrapper.h>

#include <QMutex>
#include <QException>

class EmberRendererNotInitialisedException : public QException
{
public:
  EmberRendererNotInitialisedException() : QException() {}
  virtual ~EmberRendererNotInitialisedException() {}
};

class EmberScene : public AbstractScene
{
public:
  EmberScene( int width, int height );
  EmberScene( const EmberScene &other );
  virtual ~EmberScene();

  static bool initialiseRenderer( const QString &palettePath, int platform, int device );
  static void destroyRenderer();

  /// cross-breeds this scene with another one
  virtual QPair< AbstractScene*, AbstractScene* > breed( AbstractScene *other, int mutationStrength );

  // rendering methods
  virtual bool renderTo( QImage &image );
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
  static QMutex s_renderMutex;
  static EmberCLns::RendererCL<double> *s_renderer;
  static EmberNs::SheepTools<double, double> *s_tools;

  int m_width;
  int m_height;

  EmberNs::Ember<double> m_ember;
};

#endif // EMBERSCENE_H

