#include "abstractscene.h"

#include "randomiser.h"

AbstractScene::AbstractScene()
{
  m_fitness = 0.0;
}

AbstractScene::AbstractScene( const AbstractScene &other )
{
  m_fitness = other.m_fitness;
}

AbstractScene::~AbstractScene()
{
}

void AbstractScene::mutate( int mutationStrength )
{
  m_fitness = 0.0;

  do
  {
    mutateOnce();
  } while ( Randomiser::randomInt( 100 ) < mutationStrength ) ;
}
