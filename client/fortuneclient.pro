QT += network widgets
QT += multimedia

HEADERS       = client.h \
    settings.h \
    alarm.h
SOURCES       = client.cpp \
                main.cpp \
    settings.cpp \
    alarm.cpp

TARGET=kerneldebuggerclient

RESOURCES += \
    client.qrc
