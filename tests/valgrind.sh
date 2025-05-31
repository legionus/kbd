#!/bin/sh -efu

tool="$1"
shift

case "$tool" in
	memcheck)
		valgrind_args="--leak-check=full --leak-resolution=high --show-leak-kinds=all --track-origins=yes --trace-children=yes"
		;;
	*)
		echo >&1 "$0: unknown valgrind tool: $tool"
		exit 1
		;;
esac

rc=0
valgrind --tool="$tool" $valgrind_args --error-exitcode=1 -- "$@" \
	1>stdout \
	2>stderr || rc=$?

exit $rc
