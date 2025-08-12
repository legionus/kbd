#!/bin/sh -eu
# SPDX-License-Identifier: GPL-2.0-or-later

COMPRESS="${COMPRESS?A command to compress files is required.}"
tempfile="$(mktemp "${TMPDIR-/tmp}/kbd.compress.XXXXXX")"

fatal()
{
	[ -z "$filename" ] || [ ! -f "$tempfile" ] ||
		rm -f -- "$tempfile"
	printf >&2 'compress.sh: ERROR: %s\n' "$*"
	exit 1
}

for filename in "$@"; do
	if [ -f "$filename" ] && [ -L "$filename" ]; then
		printf '%s\n' "$filename"
		readlink -f "$filename"
	fi
done > "$tempfile"

for filename in "$@"; do
	[ -f "$filename" ] && [ ! -L "$filename" ] ||
		continue

	case "$filename" in
		*/ERRORS*|*/README*)
			continue
			;;
	esac

	$COMPRESS "$filename" ||
		fatal "$filename: Compression failed."

	rm -f -- "$filename" ||:
done

eof=
symlink=

while [ -z "$eof" ]; do
	filename=
	read -r filename || eof=1

	if [ -z "$symlink" ]; then
		symlink="$filename"
		continue
	fi

	i=0
	for compressed in "$filename".*; do
		[ -f "$compressed" ] ||
			continue

		[ "$i" -eq 0 ] ||
			fatal "$symlink: Too many candidates were found to fix symlink: $(echo "$filename".*)"

		i=$(( $i + 1 ))

		suffix="${compressed##$filename.}"

		ln -s -r -- "$compressed" "$symlink.$suffix" ||
			fatal "Unable to create symlink: $symlink.$suffix"

		rm -f -- "$symlink" ||:
	done

	symlink=
done < "$tempfile"

rm -f -- "$tempfile"
tempfile=
