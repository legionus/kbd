#!/bin/sh -efu

OPT=
[ "${1-}" != '-f' ] || OPT=--force

fatal()
{
	printf >&2 '%s\n' "$*"
	exit 1
}

setvars()
{
	local varname="$1"; shift
	eval "prog_$varname=\"\$1\""; shift
	eval "version_matcher_$varname=\"\$1\""; shift
	eval "version_pattern_$varname=\"\$1\""; shift
	eval "args_$varname=\"\$@\""
}

getvars()
{
	eval "prog=\"\$prog_$1\""
	eval "version_matcher=\"\${version_matcher_$1:-gnu_version_matcher}\""
	eval "version_pattern=\"\$version_pattern_$1\""
	eval "args=\"\$args_$1\""
}

get_version()
{
	"$prog" --version </dev/null 2>/dev/null | head -1 | "$version_matcher"
}

gnu_version_matcher()
{
	sed -n -e 's/^.* \([0-9]\+\(\.[0-9]\+\)*\)$/\1/p'
}

vars=
register()
{
	setvars "$@"
	vars="$vars $1"
}

foreach()
{
	local varname
	for varname in $vars; do
		getvars "$varname"
		"$@"
	done
}

check_program()
{
	which "$prog" >/dev/null 2>&1 ||
		fatal "ERROR: You must have $varname installed to build the kbd."

	if [ -n "$version_pattern" ]; then
		local version="$(get_version "$varname")"
		[ -n "${version##$version_pattern}" ] ||
			return 0
		fatal "You must have $varname version >= $version_pattern, but you have $version ."
	fi
}

show_version()
{
	printf '   %10s: version ' "$prog"
	get_version "$varname"
}

execute()
{
	eval "set -- \$args_$varname"
	printf 'RUN: %s\n' "$prog $*"
	"$prog" "$@" || exit 1
}

register autopoint  "${AUTOPOINT:-autopoint}"   '' '' $OPT -f
register libtoolize "${LIBTOOLIZE:-libtoolize}" '' '' $OPT --install --copy --automake
register aclocal    "${ACLOCAL:-aclocal}"       '' '' $OPT -I m4
register autoconf   "${AUTOCONF:-autoconf}"     '' '' $OPT -I m4
register autoheader "${AUTOHEADER:-autoheader}" '' '' $OPT -I m4
register automake   "${AUTOMAKE:-automake}"     '' '' --force-missing --add-missing --copy

printf '\n%s' 'Checking build-system utilities: '
foreach check_program
printf 'OK\n'

printf '\n%s\n' 'Generating build-system with:'
foreach show_version
printf '\n'

rm -rf autom4te.cache
foreach execute

printf '\n%s\n\n' "Now type '${0%/*}/configure' and 'make' to compile."
