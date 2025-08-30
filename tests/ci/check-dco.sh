#!/bin/sh -efu
# SPDX-License-Identifier: GPL-2.0-or-later

rev_list="$1"
shift

cat >/tmp/forbidden.patterns <<'EOF'
@users.noreply.github.com
EOF

git log --max-parents=1 \
	--pretty='%H%x09%s%x09%ce%x09-%(trailers:key=Signed-off-by,valueonly,separator=%x07)' \
	"$rev_list" > /tmp/commits

if [ ! -s /tmp/commits ]; then
	echo '::notice:: DCO: No commits were found for verification.'
	exit 0
fi

retcode=0
n_commits=$(wc -l < /tmp/commits)

echo 'Commits to check:'
echo ''
cut -f1,2 /tmp/commits | nl
echo ''

if grep -e '-$' /tmp/commits | cut -f1,2 | grep -e '^' > /tmp/bad-commits; then
	echo 'Commits that do not have the `Signed-off-by` tag:'
	echo ''
	cat /tmp/bad-commits
	echo ''
	retcode=1
fi

cut -f1,4 /tmp/commits |
while read -r sha signed_off_by; do
	tr -s '[:cntrl:]' '\n' <<-EOF | grep -E -f /tmp/forbidden.patterns > /tmp/bad-address || continue
	${signed_off_by#-}
	EOF
	echo -n "${newline-}"
	echo "* commit $sha"
	nl -s ': Signed-off-by: ' /tmp/bad-address
	newline='
'
done > /tmp/bad-commits

if [ -s /tmp/bad-commits ]; then
	echo 'Commits has invalid values in `Signed-off-by`:'
	echo ''
	cat /tmp/bad-commits
	echo ''
	retcode=1
fi

[ "$retcode" -eq 0 ] &&
	{ level='notice'; status=PASSED; } ||
	{ level='error';  status=FAILED; }

echo "::$level:: The DCO verification of $n_commits commit(s) $status."
echo ''
exit $retcode
