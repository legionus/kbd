#!/bin/sh -efu

OPT=
[ "${1-}" != '-f' ] || OPT=-f

autoreconf -isv $OPT

echo
echo "Now type '${0%/*}/configure' and 'make' to compile."
echo
