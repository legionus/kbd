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

updated=
apt_get_install() {
	[ -n "$updated" ] || {
		message "running 'apt-get -qq update' ..."
		retry_if_failed $sudo apt-get -qq update
		updated=1
	}
	message "installing: $* ..."
	retry_if_failed $sudo \
		apt-get -qq --no-install-suggests --no-install-recommends \
		install -y "$@"
}

message "system info"
{
	message "::: free -h"
	free -h

	message "::: df -h"
	df -h
}

apt_get_install \
	autoconf automake autopoint libtool libtool-bin pkg-config \
	make bison flex gettext kbd strace valgrind libpam0g-dev \
	libz-dev libbz2-dev liblzma-dev libzstd-dev \
	gcc
