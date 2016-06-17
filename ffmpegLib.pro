
TEMPLATE = lib

DEFINES += FFMPEGLIB_LIBRARY

SOURCES += ffmpeglib.cpp

HEADERS += ffmpeglib.h\
        ffmpeglib_global.h

LIBS    += -ljpeg

LIBS += -larmadillo


CONFIG+=link_pkgconfig
PKGCONFIG+=libavformat libavcodec libswscale libswscale


QMAKE_CXXFLAGS += -std=c++11

unix {
    target.path = /usr/lib
    INSTALLS += target
}
