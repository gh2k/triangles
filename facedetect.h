#ifndef FACEDETECT_H
#define FACEDETECT_H

#include <QList>
#include <QRect>
#include <QImage>

QList< QRect > detectFaces( const QImage &image );

#endif //FACEDETECT_H