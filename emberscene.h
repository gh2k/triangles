#ifndef EMBERSCENE_H
#define EMBERSCENE_H

#include "abstractscene.h"

#include <Ember.h>
#include <SheepTools.h>
#include <RendererCL.h>
#include <OpenCLWrapper.h>

#include "x11_undefs.h"

#include <QMutex>
#include <QException>

#define EMBER_PRECISION float

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
  static const unsigned int MAX_XFORMS;

  static QMutex s_renderMutex;
  static EmberCLns::RendererCL<EMBER_PRECISION> *s_renderer;
  static EmberNs::SheepTools<EMBER_PRECISION, EMBER_PRECISION> *s_tools;

  int m_width;
  int m_height;

  EmberNs::Ember<EMBER_PRECISION> m_ember;

  static const std::vector<EmberNs::eVariationId> &vars();
};

#endif // EMBERSCENE_H

