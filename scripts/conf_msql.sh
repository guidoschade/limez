#!/bin/sh
CPPFLAGS="-L/usr/local/Hughes/lib -I/usr/local/Hughes/include"
LDFLAGS="-L/usr/local/Hughes/lib"
export CPPFLAGS LDFLAGS

make -f Makefile.dist clean all
./configure --enable-msql-db
time gmake clean all
strip src/limez
