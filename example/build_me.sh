#!/bin/bash

if [ `which qmake | wc -l` -lt 1 ]
then
	qmake='/usr/local/Cellar/qt5/5.5.1_2/bin/qmake'
fi

make clean && rm .qmake.stash
$qmake . && make && ./recurse
