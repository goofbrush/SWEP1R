TEMPLATE = app
TARGET = MyProject
QT += core widgets gui

CONFIG += console
HEADERS += WinMem.h Racer.h mainWindow.h
SOURCES += main.cpp
LIBS += -lPsapi

FORMS += mainWindow.ui

RESOURCES += res/