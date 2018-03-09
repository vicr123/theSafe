#-------------------------------------------------
#
# Project created by QtCreator 2018-02-25T18:15:59
#
#-------------------------------------------------

QT       += core gui dbus thelib
CONFIG   += c++14

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = thesafe
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
    strategymodel.cpp \
    folderselectmodel.cpp

HEADERS += \
        mainwindow.h \
    strategymodel.h \
    folderselectmodel.h

FORMS += \
        mainwindow.ui

TRANSLATIONS += translations/vi_VN.ts \
    translations/da_DK.ts \
    translations/es_ES.ts \
    translations/lt_LT.ts \
    translations/nl_NL.ts \
    translations/pl_PL.ts \
    translations/pt_BR.ts \
    translations/ru_RU.ts \
    translations/sv_SE.ts \
    translations/en_AU.ts \
    translations/en_US.ts \
    translations/en_GB.ts \
    translations/en_NZ.ts \
    translations/de_DE.ts \
    translations/in_IN.ts \
    translations/au_AU.ts \
    translations/it_IT.ts

unix {
    target.path = /usr/bin/

    translations.files = translations/*
    translations.path = /usr/share/thesafe/translations

    desktop.files = theshell.desktop
    desktop.path = /usr/share/applications/

    INSTALLS += target translations desktop
}
