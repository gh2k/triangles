#ifndef ABSTRACTSCENE_H
#define ABSTRACTSCENE_H

#include <QImage>

class AbstractScene
{
public:
  AbstractScene();
  AbstractScene( const AbstractScene &other );
  virtual ~AbstractScene();

  /// returns the cached fitness for this scene
  float fitness() const { return m_fitness; }
  /// sets the cached fitness for this scene
  void setFitness( float _fitness ) { m_fitness = _fitness; }

  /// renders the scene to an image
  virtual bool renderTo( QImage &image ) = 0;

  virtual void randomise() = 0;

  /// saves the scene to a non-bitmap file (such as svg, xml)
  virtual void saveToFile( const QString &fn ) = 0;

  /// dumps the scene to a datastream for later processing
  virtual void saveToStream( QDataStream &stream ) = 0;

  /// loads the scene from a datastream for later processing
  virtual void loadFromStream( QDataStream &stream ) = 0;

  /// makes a copy of the scene
  virtual AbstractScene *clone() const = 0;

  /// cross-breeds this scene with another one
  virtual QPair< AbstractScene*, AbstractScene* > breed( AbstractScene *other, int mutationStrength ) = 0;

  /// randomly change some variables in the scene
  /// \param mutationStrength is a number between 0 and 99, which
  /// defines the likelihood of the randomisation to be re-run after it ends
  /// (can execute an arbitrary number of times)
  void mutate( int mutationStrength );

protected:
  virtual void mutateOnce() = 0;

private:
  float m_fitness;
};

#endif // ABSTRACTSCENE_H

