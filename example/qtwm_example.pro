QT += widgets

HEADERS       = toolwindowmanager.h
SOURCES       = toolwindowmanager.cpp \
                main.cpp

FORMS         = toolwindowmanager.ui

LIBS += -lqtoolwindowmanager -L../build-qtoolwindowmanager

INCLUDEPATH += ../qtoolwindowmanager
