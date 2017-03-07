#-------------------------------------------------
#
# Project created by QtCreator 2017-02-20T04:23:05
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets
win32:CONFIG(debug, debug|release): LIBS += -lqwindowsd -lqtfreetyped -lQt5PlatformSupportd
TARGET = DisplayTest
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp

HEADERS  += mainwindow.h

FORMS    += mainwindow.ui
