#ifndef FACEDETECT_H
#define FACEDETECT_H

#include <QList>
#include <QRect>
#include <QImage>

/// detects any face-areas in the image, as rectangles
/// so that these can be assigned a higher weight in the
/// fitness function.
/// Currently this only actually detects the largest face
QList< QRect > detectFaces( const QImage &image );

#endif //FACEDETECT_H
