lessThan(QT_MAJOR_VERSION, 5) {
  QT += gui
} else {
  QT += widgets
}

HEADERS       = toolwindowmanager.h
SOURCES       = toolwindowmanager.cpp \
                main.cpp

FORMS         = toolwindowmanager.ui

LIBS += -lqtoolwindowmanager -L../build-libqtoolwindowmanager

INCLUDEPATH += ../libqtoolwindowmanager
