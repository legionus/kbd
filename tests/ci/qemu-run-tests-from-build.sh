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

ci_arch="${1-}"

export CHECK_KEYWORDS="unittest e2e loadability"
export SANDBOX="priviliged"
export TTY="/dev/tty60"

case "$ci_arch" in
	x86_64)
		export HAVE_VGA=1
		;;
esac

testsuite_args=
for kw in $CHECK_KEYWORDS; do
	testsuite_args="$testsuite_args --keywords=$kw"
done

sudo -- chmod 666 "$TTY"
sudo -- kbd_mode -f -u -C "$TTY"
sudo -E -- /bin/bash /tmp/kbd/tests/testsuite $testsuite_args --memcheck < "$TTY" ||
	fail_with_artifacts
