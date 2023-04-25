#!/bin/sh -efu

TTY="${TTY:-/dev/tty60}"

testdir="$(readlink -f "$0")"
testdir="${testdir%/*}"
topdir="${testdir%/*}"

set -x
cd "$topdir"

chmod 666 "$TTY"
kbd_mode -f -u -C "$TTY"

make check < "$TTY"
