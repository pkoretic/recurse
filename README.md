# recurse

Recurse is set to be modern micro web framework written in latest C++14 using
Qt library leveraging all the best features of both worlds.  We strongly
emphasize on writing a clean and easy to understand code and avoid using
templates to encourage contributions.

Recurse aims to be small with no middlewares bundled in the core. This should
allow it to be very robust for writing next generation web applications and
APIs.


[![GitHub license](https://img.shields.io/github/license/mashape/apistatus.svg)](https://github.com/xwalk/recurse/blob/master/LICENSE)

# Example

main.cpp
```
#include "recurse.hpp"

int main(int argc, char *argv[])
{
    Recurse app(argc, argv);

    // logger
    app.use([](auto &ctx, auto next) {
        qDebug() << ctx.request.ip;
        next();
    });

    // hello world
    app.use([](auto &ctx) {
        ctx.response.send("Hello world");
    });


    app.listen(3000);
}

```
main.pro
```
TARGET = example

QT       += core network
QT       -= gui

CONFIG   += console
CONFIG   += c++14
CONFIG   -= app_bundle

TEMPLATE = app

SOURCES += main.cpp

QMAKE_CXXFLAGS += -std=c++14

macx {
    QMAKE_CXXFLAGS += -stdlib=libc++
}
```

build, run and use
```
# build

qmake main.pro
# or for faster build: qmake QMAKE_CC=clang QMAKE_CXX=clang++ main.pro
# or for even faster build: qmake QMAKE_CC="ccache clang" QMAKE_CXX="ccache clang++" main.prop

make
# or make -j[number of cores + 1]
# or make release (default)
# or make debug

# run
./example

# use
curl http://127.0.0.1:3000
Hello world
