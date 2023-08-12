QT += network widgets

HEADERS       = server.h \
    textsaver.h
SOURCES       = server.cpp \
                main.cpp \
    textsaver.cpp

TARGET=kerneldebuggerserver

DISTFILES +=

RESOURCES += \
    resourc.qrc
