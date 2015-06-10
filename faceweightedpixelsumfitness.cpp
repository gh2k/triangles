#include "faceweightedpixelsumfitness.h"
#include "facedetect.h"

#include <QPainter>

FaceWeightedPixelSumFitness::FaceWeightedPixelSumFitness(const QImage &image, int faceWeight)
  : AbstractFitness( image )
  , m_faceWeight( faceWeight )
{
  doFaceDetection();
}

void FaceWeightedPixelSumFitness::doFaceDetection()
{
  m_faces = detectFaces( target() );

  m_faceMask = QImage( target().size(), QImage::Format_RGB32 );
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

float FaceWeightedPixelSumFitness::getFitness( const QImage &candidate ) const
{
  float f = 0;
  int w = target().width();
  int h = target().height();
  for( int y = 0; y < h; ++ y )
  {
    const uchar *targetLine = target().scanLine( y );
    const uchar *candidateLine = candidate.scanLine( y );
    const uchar *weights = m_pixelWeights[y];

    // calculates the difference between pixels on the two images
    for( int x = 0; x < w; ++ x )
    {
      float dB = (float)(*(targetLine++) - *(candidateLine++));
      float dG = (float)(*(targetLine++) - *(candidateLine++));
      float dR = (float)(*(targetLine++) - *(candidateLine++));
      ++targetLine; ++candidateLine;

      // larger number is better. multiplies pixels detected as part of a face by faceWeight
      // this gives a better weighting to face pixels and will accept candidates that have a better
      // match for those areas
      float pixelFitness = (dR * dR + dG * dG + dB * dB) * (float)(m_faceWeight * weights[x] + 1 );

      f += pixelFitness;
    }
  }

  return f;
}
