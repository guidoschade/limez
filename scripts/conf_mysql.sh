#!/bin/sh
CPPFLAGS="-L/www/apps/mysql/lib -I/www/apps/mysql/include"
LDFLAGS="-L/www/apps/mysql/lib"
export CPPFLAGS LDFLAGS

make -f Makefile.dist clean all
./configure --enable-mysql-db
time gmake clean all
strip src/limez
