TARGET = recurse

QT       += core network
QT       -= gui

CONFIG   += console
CONFIG   += c++14
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += main.cpp

# TODO: avoid this
QMAKE_CXXFLAGS_WARN_ON = -Wall -Wno-unused-parameter

QMAKE_CXXFLAGS += -std=c++14
