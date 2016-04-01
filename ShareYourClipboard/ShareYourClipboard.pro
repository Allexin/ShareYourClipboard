#-------------------------------------------------
#
# Project created by QtCreator 2016-03-29T23:19:13
#
#-------------------------------------------------

QT       += core gui network

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ShareYourClipboard
TEMPLATE = app

CONFIG+=address_sanitizer
CONFIG+=C++11


SOURCES += main.cpp \
    data/cclipboardmanager.cpp \
    ui/ctrayicon.cpp \
    ui/settingswindow.cpp \
    data/cnetworkmanager.cpp \
    data/cfileloader.cpp \
    data/cfilesaver.cpp \
    ui/copyprogressdialog.cpp

HEADERS  += \
    data/cclipboardmanager.h \
    ui/ctrayicon.h \
    ui/settingswindow.h \
    data/cnetworkmanager.h \
    data/cfileloader.h \
    data/cfilesaver.h \
    ui/copyprogressdialog.h \
    data/utils.h

FORMS    += \
    ui/settingswindow.ui \
    ui/copyprogressdialog.ui
