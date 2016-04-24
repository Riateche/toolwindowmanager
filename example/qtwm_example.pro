lessThan(QT_MAJOR_VERSION, 5) {
  QT += gui
} else {
  QT += widgets
}

HEADERS       = \
    toolwindowmanagerexample.h
SOURCES       = \
                main.cpp \
    toolwindowmanagerexample.cpp

FORMS         = \
    toolwindowmanagerexample.ui

LIBS += -lqtoolwindowmanager -L../build-libqtoolwindowmanager

INCLUDEPATH += ../libqtoolwindowmanager
