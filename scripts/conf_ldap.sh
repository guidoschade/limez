#!/bin/sh
CPPFLAGS="-L/www/apps/openldap/lib -I/www/apps/openldap/include"
LDFLAGS="-L/www/apps/openldap/lib"
export CPPFLAGS LDFLAGS

make -f Makefile.dist clean all
./configure --enable-open-ldap
time gmake clean all
strip src/limez
