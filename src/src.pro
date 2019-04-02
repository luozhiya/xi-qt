#-------------------------------------------------
#
# Project created by QtCreator 2018-04-27T16:43:34
#
#-------------------------------------------------

QT       += core gui concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = xi-qt
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


RC_ICONS += resources/icons/xi-editor.ico

SOURCES += \
    main.cpp \
    ximainwindow.cpp \
    theme.cpp \
    application.cpp \
    shortcuts.cpp \
    trace.cpp \
    range.cpp \
    content_view.cpp \
    core_connection.cpp \
    edit_view.cpp \
    edit_window.cpp \
    line_cache.cpp \
    style_map.cpp \
    text_line.cpp \
    unfair_lock.cpp \
    file.cpp \
    font.cpp \
    perference.cpp \
    xi.cpp \
    style_span.cpp \
    style.cpp \
    config.cpp

HEADERS += \
    ximainwindow.h \
    theme.h \
    application.h \
    shortcuts.h \
    trace.h \
    range.h \
    content_view.h \
    core_connection.h \
    edit_view.h \
    edit_window.h \
    line_cache.h \
    text_line.h \
    unfair_lock.h \
    style_map.h \
    file.h \
    font.h \
    perference.h \
    xi.h \
    style_span.h \
    style.h \
    config.h

DISTFILES += \
    resources/icons/xi-editor-app.png \
    resources/icons/xi-editor.png \
    todo.txt

RESOURCES += \
    xi.qrc

INCLUDEPATH += $$PWD/../third_party/boost/include/boost-1_67
DEPENDPATH += $$PWD/../third_party/boost/include/boost-1_67

CONFIG += c++1z
