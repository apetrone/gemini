#-------------------------------------------------
#
# Project created by QtCreator 2014-09-10T21:45:50
#
#-------------------------------------------------

QT       += core gui opengl

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

# temporary developer lib path
macx:QMAKE_LFLAGS += -F/Users/apetrone/Qt5.3.1/5.3/clang_64/lib
CONFIG += c++11

TARGET = kraken
TEMPLATE = app


SOURCES += \
    main.cpp\
    mainwindow.cpp \
    renderwindow.cpp \
    log_service.cpp \
    applicationcontext.cpp \
    asset_service.cpp \
    ../common.cpp \
    ../common/extension.cpp \
    ../datamodel/animation.cpp \
    ../datamodel/material.cpp \
    ../datamodel/node.cpp

HEADERS  += mainwindow.h \
    renderwindow.h \
    log_service.h \
    applicationcontext.h \
    asset_service.h \
    ../common/extension.h \
    ../common.h \
    ../datamodel.h \
    ../datamodel/animation.h \
    ../datamodel/material.h \
    ../datamodel/mesh.h \
    ../datamodel/node.h \
    ../datamodel/skeleton.h

RESOURCES += \
    kraken.qrc

DESTDIR = ../../../bin/debug_x86_64/


unix:LIBS += "-L../../../lib/debug_x86_64"
macx:LIBS += "../../../lib/debug_x86_64/platform.dylib"
macx:LIBS += "../../../lib/debug_x86_64/core.dylib"
macx:LIBS += "../../../lib/debug_x86_64/renderer.dylib"
macx:LIBS += "../../../lib/debug_x86_64/runtime.dylib"
macx:LIBS += "-framework Cocoa"

win32:LIBS += "../../../lib/debug_x86_64"
win32:LIBS += "platform.lib"
win32:LIBS += "core.lib"
win32:LIBS += "renderer.lib"

INCLUDEPATH += "../"
INCLUDEPATH += "../../platform"
INCLUDEPATH += "../../platform/utl"
INCLUDEPATH += "../../core"
INCLUDEPATH += "../../renderer"
INCLUDEPATH += "../../sdk/"
INCLUDEPATH += "../../../dependencies/glm"
INCLUDEPATH += "../../../dependencies/slim"
INCLUDEPATH += "../../../src"
