# SPDX-License-Identifier: GPL-2.0-or-later

if USE_XKB
XKB_ENABLED = yes
else
XKB_ENABLED = no
endif

STATIC_COMPLETIONS = \
	chvt \
	deallocvt \
	dumpkeys \
	fgconsole \
	kbd_mode \
	kbdinfo \
	kbdrate \
	openvt \
	psfxtable \
	setleds \
	setmetamode \
	setvtrgb \
	showconsolefont \
	showkey

if VLOCK
STATIC_COMPLETIONS += vlock
endif

if KEYCODES_PROGS
STATIC_COMPLETIONS += getkeycodes setkeycodes
endif

GENERATED_COMPLETIONS = \
	loadkeys \
	loadunimap \
	mapscrn \
	setfont

if RESIZECONS_PROGS
GENERATED_COMPLETIONS += resizecons
endif

EXTRA_DIST = \
	loadkeys.in \
	loadunimap.in \
	mapscrn.in \
	resizecons.in \
	setfont.in

if BUILD_BASH_COMPLETION
dist_bashcompletion_DATA = $(STATIC_COMPLETIONS)
nodist_bashcompletion_DATA = $(GENERATED_COMPLETIONS)
else
EXTRA_DIST += $(STATIC_COMPLETIONS)
endif

CLEANFILES = \
	loadkeys \
	loadunimap \
	mapscrn \
	resizecons \
	setfont

edit = $(SED) \
	-e 's|@datadir[@]|$(datadir)|g' \
	-e 's|@USE_XKB[@]|$(XKB_ENABLED)|g'

%: %.in Makefile
	$(AM_V_GEN)$(edit) $< > $@.tmp && mv $@.tmp $@
