#-------------------------------------------------
#
# Project created by QtCreator 2013-01-09T23:14:21
#
#-------------------------------------------------

QT += core gui
QT += svg
QT += network
QT += webkit
QT += webkitwidgets
QT += widgets
QT += multimedia

TARGET = BoxVideoPoker
TEMPLATE = app
RC_FILE = Poker.rc


SOURCES += main.cpp\
        mainwindow.cpp \
    pokerhand.cpp \
    logindialog.cpp \
    BoxOAuth2.cpp \
    network.cpp

HEADERS  += mainwindow.h \
    logindialog.h \
    pokerhand.h \
    BoxOAuth2.h \
    network.h

FORMS    += mainwindow.ui \
    logindialog.ui

RESOURCES += \
    resources.qrc
