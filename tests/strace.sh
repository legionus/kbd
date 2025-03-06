#!/bin/sh -efu

cwd="$(readlink -ev "${0%/*}")"

rc=0
strace -o syscalls.raw -s 1073741823 -e abbrev=none -e trace=/open.*,/ioctl \
	-- "$@" \
	1>stdout \
	2>stderr || rc=$?

"$cwd/syscall-filter.sed" < syscalls.raw > syscalls ||:

exit $rc
