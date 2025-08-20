CODE_COVERAGE_DIRECTORY        = $(top_builddir)/src $(top_builddir)/tests
CODE_COVERAGE_OUTPUT_FILE      = coverage.info
CODE_COVERAGE_OUTPUT_DIRECTORY = coverage
CODE_COVERAGE_GENHTML_OPTIONS  = --ignore-errors range,range

include $(top_srcdir)/aminclude_static.am

KBD_DISTCHECK_CONFIGURE_FLAGS = \
	--disable-code-coverage \
	--enable-libkeymap \
	--enable-libkfont \
	--enable-optional-progs \
	${DISTCHECK_CONFIGURE_EXTRA_FLAGS}

if CODE_COVERAGE_ENABLED
AM_DISTCHECK_CONFIGURE_FLAGS += $(KBD_DISTCHECK_CONFIGURE_FLAGS)
else
AM_DISTCHECK_CONFIGURE_FLAGS = $(KBD_DISTCHECK_CONFIGURE_FLAGS)
endif

ACLOCAL_AMFLAGS = -I m4

EXTRA_DIST = \
	CREDITS \
	contrib docs

SUBDIRS = src data po docs
if BUILD_TESTS
SUBDIRS += tests
endif

SPARSE = sparse
CHECK  = $(SPARSE) \
	-Wsparse-error -Wsparse-all -Wno-default-bitfield-sign \
	-Wno-declaration-after-statement -Wno-unknown-attribute

check-sparse:
	make CC="cgcc" REAL_CC="$(CC)" CHECK="$(CHECK)" CFLAGS+="-D_LINUX_WAIT_H"

.PHONY: compile_commands.json
compile_commands.json:
	@make -C src $(MAKEFLAGS) REAL_CC="$(CC)" CC=$(CURDIR)/contrib/gen_compile_commands
	@EXPORT_COMPILE_COMMANDS=1 $(CURDIR)/contrib/gen_compile_commands
	@rm -rf -- .lsp.data

clean-local:
	rm -rf -- .lsp.data compile_commands.json

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

clean-local: code-coverage-clean
dist-clean-local: code-coverage-dist-clean
