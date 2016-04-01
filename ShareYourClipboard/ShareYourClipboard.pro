#-------------------------------------------------
#
# Project created by QtCreator 2016-03-29T23:19:13
#
#-------------------------------------------------

QT       += core gui network gui-private

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ShareYourClipboard
TEMPLATE = app

CONFIG+=address_sanitizer
CONFIG+=C++11

linux: LIBS += -lxcb -lxcb-keysyms
mac: LIBS += -framework Carbon
win32: LIBS += -luser32



SOURCES += main.cpp \
    data/cclipboardmanager.cpp \
    ui/ctrayicon.cpp \
    ui/settingswindow.cpp \
    data/cnetworkmanager.cpp \
    data/cfileloader.cpp \
    data/cfilesaver.cpp \
    ui/copyprogressdialog.cpp \
    UGlobalHotkey-master/uexception.cpp \
    UGlobalHotkey-master/uglobalhotkeys.cpp \
    UGlobalHotkey-master/ukeysequence.cpp

HEADERS  += \
    data/cclipboardmanager.h \
    ui/ctrayicon.h \
    ui/settingswindow.h \
    data/cnetworkmanager.h \
    data/cfileloader.h \
    data/cfilesaver.h \
    ui/copyprogressdialog.h \
    data/utils.h \
    UGlobalHotkey-master/hotkeymap.h \
    UGlobalHotkey-master/uexception.h \
    UGlobalHotkey-master/uglobal.h \
    UGlobalHotkey-master/uglobalhotkeys.h \
    UGlobalHotkey-master/ukeysequence.h

FORMS    += \
    ui/settingswindow.ui \
    ui/copyprogressdialog.ui
