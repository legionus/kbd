#!/bin/bash -efu

PROG="${0##*/}"

type sudo >/dev/null 2>&1 && sudo=sudo || sudo=

message() {
	printf '%s: %s\n' "$PROG" "$*"
}

retry_if_failed() {
	for i in `seq 0 99`; do
		"$@" && i= && break || sleep 1
	done
	[ -z "$i" ]
}

message "system info"
{
	message "Run: free -h"
	free -h

	message "Run: df -h"
	df -h
}

message "Run: apt-get update ..."
retry_if_failed $sudo apt-get update

message "Run: apt-get install ..."
retry_if_failed $sudo \
	apt-get -y --no-install-suggests --no-install-recommends install \
		autoconf automake autopoint libtool libtool-bin pkg-config \
		make bison flex gettext kbd strace valgrind libpam0g-dev \
		libz-dev libbz2-dev liblzma-dev libzstd-dev \
		gcc "$@"
