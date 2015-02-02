#include "abstractscene.h"

#include "randomiser.h"

AbstractScene::AbstractScene()
{
  m_fitness = 0.0;
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
