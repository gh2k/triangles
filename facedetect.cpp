#include "facedetect.h"

#include <opencv/cv.h>
#include <opencv/highgui.h>

CvRect detect_and_draw( IplImage* img, CvMemStorage* storage, CvHaarClassifierCascade* cascade )
{
  IplImage *gray, *small_img;
  int i = 0;
  int scale = 1;

  gray = cvCreateImage( cvSize(img->width,img->height), 8, 1 );
  small_img = cvCreateImage( cvSize( cvRound (img->width/scale),
    cvRound (img->height/scale)), 8, 1 );

  cvCvtColor( img, gray, CV_RGB2GRAY );
  cvResize( gray, small_img, CV_INTER_LINEAR );
  cvEqualizeHist( small_img, small_img );
  cvClearMemStorage( storage );

  CvRect* r = NULL;

  if( cascade )
  {
    double t = (double)cvGetTickCount();
    CvSeq* faces = cvHaarDetectObjects( small_img, cascade, storage,
      1.1, 2, 0
      |CV_HAAR_FIND_BIGGEST_OBJECT
      //|CV_HAAR_DO_ROUGH_SEARCH
      //|CV_HAAR_DO_CANNY_PRUNING
      //|CV_HAAR_SCALE_IMAGE
      ,
      cvSize(30, 30) );
    t = (double)cvGetTickCount() - t;

    printf( "detection time = %gms\n", t/((double)cvGetTickFrequency()*1000.) );

    r = (CvRect*)cvGetSeqElem( faces, i );

    cvReleaseImage( &gray );
    cvReleaseImage( &small_img );

    if(r) {
      return cvRect(r->x,r->y,r->width,r->height);
    }
  }

  return cvRect(-1,-1,0,0);
}

QList< QRect > detectFaces( const QImage &_image )
{
  QList< QRect > faces;
  QImage image( _image );

  const char *CASCADE_NAME = "haarcascade_frontalface_alt.xml";

  CvMemStorage *storage = cvCreateMemStorage(0);
  CvHaarClassifierCascade *cascade = (CvHaarClassifierCascade*)cvLoad( CASCADE_NAME, 0, 0, 0 );

  IplImage *opencvimg = cvCreateImageHeader(cvSize(image.width(),image.height()),IPL_DEPTH_8U,4);
  opencvimg->imageData = (char*)image.bits(); // share buffers

  CvRect r = detect_and_draw(opencvimg,storage,cascade);
  if( r.x > 0 && r.y > 0 )
    faces.append(QRect(QPoint(r.x,r.y),QSize(r.width,r.height)));

  return faces;
}
