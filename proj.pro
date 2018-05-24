#-------------------------------------------------
#
# Project created by QtCreator 2018-02-04T15:52:37
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = proj
TEMPLATE = app

# The following define makes your compiler emit warnings if you use
# any feature of Qt which has been marked as deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if you use deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0


SOURCES += \
        main.cpp \
        mainwindow.cpp \
        xibridge.cpp \
        editor.cpp \
    ui/editorview.cpp \
    ui/editormodel.cpp \
    ui/cell.cpp \
    ui/line.cpp \
    ui/font.cpp \
    ui/fontfactory.cpp \
    ui/fontcache.cpp \
    configuration.cpp \
    fontconfig.cpp

HEADERS += \
        mainwindow.h \
        xibridge.h \
        editor.h \
    ui/editorview.h \
    ui/editormodel.h \
    ui/cell.h \
    ui/line.h \
    ui/font.h \
    ui/fontfactory.h \
    ui/fontcache.h \
    configuration.h \
    fontconfig.h

FORMS += \
        mainwindow.ui
