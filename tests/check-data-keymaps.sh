#!/bin/sh -efu

TTY="${TTY:-/dev/tty60}"

workdir="$(readlink -f "$PWD")"
testdir="$(readlink -f "$0")"
testdir="${testdir%/*}"
topdir="${testdir%/*}"

cd "$topdir"

chmod 666 "$TTY"
kbd_mode -f -u -C "$TTY"

find "$PWD"/data/keymaps/i386/ \
		\( -type f -a \! -regex '.*/include/.*' -a -name '*.map' \) \
		-print |
	sort -d -o "$workdir"/keymaps.list

export LOADKEYS_INCLUDE_PATH="$PWD/data/keymaps/**"

ret=0
while read -r keymap; do
	if ! "$BUILDDIR"/src/loadkeys -C "$TTY" "$keymap" >/tmp/out 2>&1; then
		{
			printf '### Failed check: %s\n' "$keymap"
			cat /tmp/out
		} >&2
		ret=1
	fi
done < "$workdir"/keymaps.list

rm -f -- "$workdir"/keymaps.list /tmp/out ||:

exit $ret
