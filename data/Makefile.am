EXTRA_DIST = consolefonts consoletrans keymaps partialfonts unimaps \
	     compress.sh

IGNORE_KEYMAPS = i386/mk_modmap README */README */*/README

COMPRESS = $(COMPRESS_PROG) -f -9
LOCAL_CLEANUPDIRS =

export COMPRESS

if ENABLE_COMPRESS
SRC_KEYMAPDIR  = $(KEYMAPDIR)_Z
SRC_FONTDIR    = $(FONTDIR)_Z
SRC_PARTIALDIR = $(PARTIALDIR)_Z

all: $(SRC_KEYMAPDIR) $(SRC_FONTDIR) $(SRC_PARTIALDIR)

V_PACK   = $(V_PACK_$(V))
V_PACK_  = $(V_PACK_$(AM_DEFAULT_VERBOSITY))
V_PACK_0 = @echo "  PACK    " $@;
V_PACK_1 =

# compress data files - do not touch the distribution but copy first
$(SRC_KEYMAPDIR): $(KEYMAPDIR)
	$(V_PACK)cp -r -- "$<" "$@" && chmod -R u+w -- "$@" && \
	$(srcdir)/compress.sh "$@"/*/*.map "$@"/*/*/*.map

$(SRC_FONTDIR): $(FONTDIR)
	$(V_PACK)cp -r -- "$<" "$@" && chmod -R u+w -- "$@" && \
	$(srcdir)/compress.sh "$@"/*

$(SRC_PARTIALDIR): $(PARTIALDIR)
	$(V_PACK)cp -r -- "$<" "$@" && chmod -R u+w -- "$@" && \
	$(srcdir)/compress.sh "$@"/*

# (not yet screenmaps - some other time)

LOCAL_CLEANUPDIRS += $(SRC_KEYMAPDIR) $(SRC_FONTDIR) $(SRC_PARTIALDIR)
else
SRC_KEYMAPDIR  = $(KEYMAPDIR)
SRC_FONTDIR    = $(FONTDIR)
SRC_PARTIALDIR = $(PARTIALDIR)
endif

install-consolefonts: $(SRC_FONTDIR) $(SRC_PARTIALDIR)
	mkdir -p -m 755 -- $(DESTDIR)$(datadir)/$(FONTDIR)
	cp -dPR -- $(SRC_FONTDIR)/* $(DESTDIR)$(datadir)/$(FONTDIR)/
	mkdir -p -m 755 -- $(DESTDIR)$(datadir)/$(FONTDIR)/$(PARTIALDIR)
	cp -dPR -- $(SRC_PARTIALDIR)/* $(DESTDIR)$(datadir)/$(FONTDIR)/$(PARTIALDIR)/

install-consoletrans:
	mkdir -p -m 755 -- $(DESTDIR)$(datadir)/$(TRANSDIR)
	cp -dPR -- $(srcdir)/$(TRANSDIR)/* $(DESTDIR)$(datadir)/$(TRANSDIR)/

install-unimaps:
	mkdir -p -m 755 -- $(DESTDIR)$(datadir)/$(UNIMAPDIR)
	cp -dPR -- $(srcdir)/$(UNIMAPDIR)/* $(DESTDIR)$(datadir)/$(UNIMAPDIR)/

install-keymaps: $(SRC_KEYMAPDIR)
	mkdir -p -m 755 -- $(DESTDIR)$(datadir)/$(KEYMAPDIR)
	cp -dPR -- $(SRC_KEYMAPDIR)/* $(DESTDIR)$(datadir)/$(KEYMAPDIR)/
	cd "$(DESTDIR)$(datadir)/$(KEYMAPDIR)"; \
	  for f in $(IGNORE_KEYMAPS); do ! test -e "$$f" || rm -f -- "$$f"; done
	rm -f $(DESTDIR)$(datadir)/$(KEYMAPDIR)/ppc
	$(LN_S) mac $(DESTDIR)$(datadir)/$(KEYMAPDIR)/ppc
	@if [ -f $(DESTDIR)$(datadir)/$(OLDKEYMAPDIR) ]; then \
	  echo "Done. You may want to remove old keymaps with" ; \
	  echo "  rm -rf $(DESTDIR)$(datadir)/$(OLDKEYMAPDIR)" ; \
	  echo "But be careful to preserve your default map if it is" ; \
	  echo "nonstandard, and to adapt any scripts in rc.local or so." ; \
	fi

install-data-hook: install-keymaps install-consolefonts install-consoletrans install-unimaps

clean-local:
	for d in $(LOCAL_CLEANUPDIRS); do \
	  test -d "$$d" && rm -rf -- "$$d" ||:; \
	done

uninstall-hook:
	cd $(DESTDIR)$(datadir) && rm -rf $(FONTDIR) $(PARTIALDIR) $(TRANSDIR) $(UNIMAPDIR) $(KEYMAPDIR)
