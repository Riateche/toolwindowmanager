TEMPLATE = lib

QT += widgets

OTHER_FILES += \
    qtoolwindowmanager.pro.user

HEADERS += \
    qabstracttoolwindowmanagerarea.h \
    qtoolwindowmanager.h \
    private/qtoolwindowmanagerarea_p.h \
    private/qtoolwindowmanager_p.h \
    private/qtoolwindowmanagerwrapper_p.h

SOURCES += \
    qabstracttoolwindowmanagerarea.cpp \
    qtoolwindowmanager.cpp \
    qtoolwindowmanagerarea.cpp \
    qtoolwindowmanagerwrapper.cpp

DEFINES += QTOOLWINDOWMANAGER_BUILD_LIB

