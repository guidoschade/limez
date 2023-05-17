#!/bin/sh
CPPFLAGS="-L/www/apps/ldapsdk/lib -I/www/apps/openldap/include"
LDFLAGS="-L/www/apps/ldapsdk/lib"
export CPPFLAGS LDFLAGS

make -f Makefile.dist clean all
./configure --enable-iplanet-ldap
time gmake clean all
strip src/limez
