#!/bin/bash

make clean && rm .qmake.stash && qmake . && make && ./recurse
