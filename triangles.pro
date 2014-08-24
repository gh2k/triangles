#-------------------------------------------------
#
# Project created by QtCreator 2014-08-23T23:50:49
#
#-------------------------------------------------

QT       += core gui svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = triangles
TEMPLATE = app


SOURCES += main.cpp\
        triangles.cpp \
    facedetect.cpp \
    mtrand.cpp \
    poly.cpp \
    randomiser.cpp \
    scene.cpp

HEADERS  += triangles.h \
    facedetect.h \
    mtrand.h \
    poly.h \
    randomiser.h \
    scene.h

FORMS    += triangles.ui

macx:INCLUDEPATH += /usr/local/Cellar/opencv/2.4.9/include/
macx:LIBS += -L/usr/local/Cellar/opencv/2.4.9/lib/ -lopencv_core -lopencv_objdetect -lopencv_imgproc

macx:QMAKE_LFLAGS += -framework OpenCL

OTHER_FILES += \
    README.md \
    LICENSE \
    triangles.cl

RESOURCES += \
    triangles.qrc
