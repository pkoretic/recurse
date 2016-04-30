TARGET = recurse_postgres

QT       += core network sql
QT       -= gui

CONFIG   += console
CONFIG   += c++14
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += postgres.cpp
HEADERS += ../../recurse.hpp \
           ../../request.hpp \
           ../../response.hpp \
           ../../context.hpp

QMAKE_CXXFLAGS += -std=c++14

macx {
    QMAKE_CXXFLAGS += -stdlib=libc++
}

INCLUDEPATH += $$PWD/../../
