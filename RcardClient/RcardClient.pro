#-------------------------------------------------
#
# Project created by QtCreator 2015-07-17T17:41:16
#
#-------------------------------------------------

QT       += core gui serialport

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = RcardClient
TEMPLATE = app


SOURCES += main.cpp\
        mainwindow.cpp \
    frame.cpp \
    memcard.cpp

HEADERS  += mainwindow.h \
    frame.h \
    memcard.h

FORMS    += mainwindow.ui
