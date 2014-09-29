QT += core network
QT -= gui

CONFIG(release, debug|release) {
    DEFINES += QT_NO_DEBUG_OUTPUT
}

CONFIG += console
CONFIG -= app_bundle

TARGET   = uwb2can
TEMPLATE = app

SOURCES += main.cpp ../can.cpp ../algorithm.cpp \
    uwb2can.cpp \
    server.cpp
HEADERS += ../can.h \
    uwb2can.h \
    protocol.h
