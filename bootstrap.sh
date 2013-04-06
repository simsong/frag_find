#!/bin/sh
#
echo Bootstrap script to create configure script using autoconf
echo
# if test -f Makefile ; then make distclean ; fi
rm -f aclocal.m4
autoheader -f
touch NEWS README AUTHORS ChangeLog
touch stamp-h
aclocal
autoconf -f
automake -a -c
./configure

