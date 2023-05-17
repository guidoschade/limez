#!/bin/sh
CPPFLAGS="-L/www/apps/postgres/lib -I/www/apps/postgres/include"
LDFLAGS="-L/www/apps/postgres/lib"
export CPPFLAGS LDFLAGS

make -f Makefile.dist clean all
./configure --enable-postgres-db
time gmake clean all
strip src/limez
