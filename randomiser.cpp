#include "randomiser.h"
#include "mtrand.h"

#include <QDateTime>
#include <qmath.h>

int randomiser::randomInt( int size )
{
  static MTRand mt( QDateTime::currentMSecsSinceEpoch() );

  return qFloor( mt() * (double) size );
}