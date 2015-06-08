#-------------------------------------------------
#
# Project created by QtCreator 2014-08-23T23:50:49
#
#-------------------------------------------------

QT       += core gui svg concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = triangles
TEMPLATE = app

FRACTORIUM_DIR = $$(HOME)/Dev/fractorium/Bin
debug:FRACTORIUM_DIR = $$(HOME)/Dev/fractorium/Dbg

LIBS += -L$$FRACTORIUM_DIR -lEmber
LIBS += -L$$FRACTORIUM_DIR -lEmberCL

LIBS += -lxml2

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

unix {
  LIBS += -lopencv_core -lopencv_objdetect -lopencv_imgproc -lOpenCL
}

native {
  QMAKE_CXXFLAGS += -march=native
} else {
  QMAKE_CXXFLAGS += -march=k8
}

QMAKE_CXXFLAGS += -DCL_USE_DEPRECATED_OPENCL_1_1_APIS

INCLUDEPATH += ./fractorium/Source/Ember
INCLUDEPATH += ./fractorium/Source/EmberCL
INCLUDEPATH += ./fractorium/Source/EmberCommon

QMAKE_CXXFLAGS_RELEASE += -O2

INCLUDEPATH += /usr/include/libxml2

QMAKE_CXXFLAGS += -std=c++11

# fractorium doesn't care about warnings! ^_^
CONFIG += warn_off
QMAKE_CXXFLAGS += -Wnon-virtual-dtor
QMAKE_CXXFLAGS += -Wshadow
QMAKE_CXXFLAGS += -Winit-self
QMAKE_CXXFLAGS += -Wredundant-decls
QMAKE_CXXFLAGS += -Wcast-align
QMAKE_CXXFLAGS += -Winline
QMAKE_CXXFLAGS += -Wunreachable-code
QMAKE_CXXFLAGS += -Wmissing-include-dirs
QMAKE_CXXFLAGS += -Wswitch-enum
QMAKE_CXXFLAGS += -Wswitch-default
QMAKE_CXXFLAGS += -Wmain
QMAKE_CXXFLAGS += -Wzero-as-null-pointer-constant
QMAKE_CXXFLAGS += -Wfatal-errors
QMAKE_CXXFLAGS += -Wall -fpermissive
QMAKE_CXXFLAGS += -Wold-style-cast
QMAKE_CXXFLAGS += -Wno-unused-parameter
QMAKE_CXXFLAGS += -Wno-unused-function
QMAKE_CXXFLAGS += -Wold-style-cast

SOURCES += main.cpp\
        triangles.cpp \
    facedetect.cpp \
    mtrand.cpp \
    poly.cpp \
    randomiser.cpp \
    trianglescene.cpp \
    abstractscene.cpp \
    emberscene.cpp

HEADERS  += triangles.h \
    facedetect.h \
    mtrand.h \
    poly.h \
    randomiser.h \
    trianglescene.h \
    abstractscene.h \
    emberscene.h \
    x11_undefs.h

FORMS    += triangles.ui

OTHER_FILES += \
    README.md \
    LICENSE

RESOURCES += \
    triangles.qrc
