#!/bin/sh -efu

env \
	LIBTSWRAP_OUTPUT="$PWD/z.syscalls" \
	LD_PRELOAD="$PWD/libtswrap/.libs/libtswrap.so" \
	"$@" \
	1>z.stdout \
	2>z.stderr \
#
