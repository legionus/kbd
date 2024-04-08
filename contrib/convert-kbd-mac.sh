#!/bin/sh

# Convert mac keycodes in mac keymaps to make possible to use them as i386
# keymaps.
# Usage:
# ./contrib/convert-kbd-mac.sh
# (Run it only once.)
#
# Authors:
# Olaf Hering <olh@suse.de>, 2003
# Jürgen Weigert <jw@suse.de>, 2006
# Stanislav Brabec <sbrabec@suse.cz>, 2024
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
SED_SCRIPT=contrib/convert-kbd-mac.sed
if test -d ../data/keymaps/mac ; then
	cd ..
fi

if ! test -d data/keymaps/mac ; then
	echo "data/keymaps/mac not found"
	exit 1
fi

for MAP in `find data/keymaps/mac -type f` ; do
	sed -i -f $SED_SCRIPT $MAP
	echo "$MAP converted to i386"
done
