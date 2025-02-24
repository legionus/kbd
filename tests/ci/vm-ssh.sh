#!/bin/bash -efu

PROG="${0##*/}"
workdir="/tmp/${CI_ARCH-}"

[ -s "$workdir/qemu-ssh-key" ]

printf '%s: %s\n' "$PROG" "$*"
ssh -q -nT \
	-o "Port 2222" \
	-o "IdentityFile $workdir/qemu-ssh-key" \
	-o "IdentityAgent none" \
	-o "StrictHostKeyChecking no" \
	-o "SetEnv LC_ALL=C" \
	"$@"
