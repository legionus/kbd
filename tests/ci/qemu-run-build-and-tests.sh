#!/bin/bash -efu

: > "$HOME"/artifacts.tar

fail_with_artifacts() {
	files=()

	for f in config.log tests/testsuite.dir tests/testsuite.log; do
		[ ! -e "$f" ] || files+=("$f")
	done

	[ "${#files[@]}" -eq 0 ] ||
		tar --append -f "$HOME"/artifacts.tar "${files[@]}"

	exit 1
}

set -x

ci_arch="$1"
shift

nproc=$(grep -c ^processor /proc/cpuinfo || echo 1)

cd "/tmp/kbd"

tests/configure.sh --datadir="$PWD/tests/data" --enable-memcheck ||
	fail_with_artifacts

make -j$nproc V=1 CFLAGS+="-g -O0"

export CHECK_KEYWORDS="unittest e2e loadability"
export SANDBOX="priviliged"
export TTY="/dev/tty60"

case "$ci_arch" in
	x86_64)
		export HAVE_VGA=1
		;;
esac

sudo -E tests/check.sh ||
	fail_with_artifacts
