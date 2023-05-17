#!/bin/sh
CPPFLAGS="-L/usr/local/msql3/lib -I/usr/local/msql3/include"
LDFLAGS="-L/usr/local/msql3/lib"
export CPPFLAGS LDFLAGS

make -f Makefile.dist clean all
./configure --enable-msql-db
time gmake clean all
strip src/limez
