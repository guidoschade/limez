#!/bin/sh
CPPFLAGS="-L/opt/database/ldap/lib -L/opt/database/mysql/lib -I/opt/database/ldap/include -I/opt/database/mysql/include -R/opt/database/mysql/lib -R/opt/database/ldap/lib"
LDFLAGS="-L/opt/database/ldap/lib -L/opt/database/mysql/lib"
export CPPFLAGS LDFLAGS

make -f Makefile.dist clean all
./configure --enable-iplanet-ldap --enable-mysql-db
time gmake clean all
strip src/limez
