AM_DISTCHECK_CONFIGURE_FLAGS = \
	--enable-libkeymap \
	--enable-optional-progs

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = \
	CREDITS \
	contrib docs rc

SUBDIRS = src data po tests docs

kbd-$(VERSION).tar.xz:
	make distcheck

kbd-$(VERSION).tar.sign:
	xz -d -c kbd-$(VERSION).tar.xz | \
	    gpg --armor --detach-sign \
	        --default-key $(GPG_KEY) \
	        --output kbd-$(VERSION).tar.sign

tar: kbd-$(VERSION).tar.xz kbd-$(VERSION).tar.sign

tar-sync: kbd-$(VERSION).tar.xz kbd-$(VERSION).tar.sign
	kup put \
	    kbd-$(VERSION).tar.xz \
	    kbd-$(VERSION).tar.sign \
	    /pub/linux/utils/kbd/
