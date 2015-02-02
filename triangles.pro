#-------------------------------------------------
#
# Project created by QtCreator 2014-08-23T23:50:49
#
#-------------------------------------------------

QT       += core gui svg

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = triangles
TEMPLATE = app

FRACTORIUM_DIR = $$(HOME)/Dev/fractorium/Bin

LIBS += -L$$FRACTORIUM_DIR -lEmber
LIBS += -L$$FRACTORIUM_DIR -lEmberCL

macx {
  LIBS += -framework OpenGL
  LIBS += -framework OpenCL

  # homebrew installs into /usr/local
  LIBS += -L/usr/local/lib

  INCLUDEPATH += /usr/local/include
  INCLUDEPATH += ./fractorium/Deps

  QMAKE_MAC_SDK = macosx10.9
  QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.9

  QMAKE_CXXFLAGS += -stdlib=libc++

  INCLUDEPATH += /usr/local/Cellar/opencv/2.4.9/include/
  LIBS += -L/usr/local/Cellar/opencv/2.4.9/lib/ -lopencv_core -lopencv_objdetect -lopencv_imgproc
}

native {
  QMAKE_CXXFLAGS += -march=native
} else {
  QMAKE_CXXFLAGS += -march=k8
}

CMAKE_CXXFLAGS += -DCL_USE_DEPRECATED_OPENCL_1_1_APIS

INCLUDEPATH += ./fractorium/Source/Ember
INCLUDEPATH += ./fractorium/Source/EmberCL
INCLUDEPATH += ./fractorium/Source/EmberCommon

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

OTHER_FILES += \
    README.md \
    LICENSE

RESOURCES += \
    triangles.qrc
