#!/bin/sh
make -f Makefile.dist clean all
./configure
time gmake clean all
strip src/limez
