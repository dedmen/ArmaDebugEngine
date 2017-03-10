QT += core network gui widgets

CONFIG += c++11
win32:CONFIG(debug, debug|release): LIBS += -lqwindowsd -lqtfreetyped -lQt5PlatformSupportd
TARGET = NamedPipeTester
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp \
    mainwindow.cpp

FORMS += \
    mainwindow.ui

HEADERS += \
    mainwindow.h
