#!/bin/bash

make clean && rm .qmake.stash && /usr/local/Cellar/qt5/5.5.1_2/bin/qmake . && make && ./recurse
