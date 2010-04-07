#!/bin/sh -efu

OPT=
[ "${1-}" != '-f' ] || OPT=-f

autoreconf -I m4 -isv $OPT

echo
echo "Now type '${0%/*}/configure' and 'make' to compile."
echo
