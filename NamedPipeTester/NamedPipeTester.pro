QT += core network
QT -= gui

CONFIG += c++11
win32:CONFIG(debug, debug|release): LIBS += -lqwindowsd -lqtfreetyped -lQt5PlatformSupportd
TARGET = NamedPipeTester
CONFIG += console
CONFIG -= app_bundle

TEMPLATE = app

SOURCES += main.cpp
