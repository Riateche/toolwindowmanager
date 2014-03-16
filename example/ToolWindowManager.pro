#-------------------------------------------------
#
# Project created by QtCreator 2014-03-07T22:02:13
#
#-------------------------------------------------

QT       += core gui

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

TARGET = ToolWindowManager
TEMPLATE = app


INCLUDEPATH += ../src

SOURCES += main.cpp\
  MainWindow.cpp \
  ../src/ToolWindowManager.cpp \
    ../src/ToolWindowManagerArea.cpp \
    ../src/ToolWindowManagerWrapper.cpp

HEADERS  += MainWindow.h \
  ../src/ToolWindowManager.h \
    ../src/ToolWindowManagerArea.h \
    ../src/ToolWindowManagerWrapper.h

FORMS    += MainWindow.ui
