#!/bin/sh -efu

testdir="$(readlink -f "$0")"
testdir="${testdir%/*}"
topdir="${testdir%/*}"

set -x
cd "$topdir"

./autogen.sh
./configure \
	--datadir="$testdir/data" \
	--enable-optional-progs \
	--enable-libkeymap \
	--enable-libkfont \
	--enable-vlock \
	"$@"
