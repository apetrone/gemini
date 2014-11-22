#-------------------------------------------------
#
# Project created by QtCreator 2014-09-10T21:45:50
#
#-------------------------------------------------

QT       += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# temporary developer lib path
QMAKE_LFLAGS += -F/Users/apetrone/Qt5.3.1/5.3/clang_64/lib

TARGET = kraken
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    renderwindow.cpp

HEADERS  += mainwindow.h \
    renderwindow.h

RESOURCES += \
    kraken.qrc

DESTDIR = ../../../bin/debug_x86_64/
