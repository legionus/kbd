EXTRA_DIST = consolefonts consoletrans keymaps partialfonts unimaps

KEYMAPSUBDIRS = include sun amiga atari i386/azerty i386/bepo i386/dvorak \
	i386/fgGIod i386/qwerty i386/qwertz i386/include i386/olpc \
	i386/colemak i386/carpalx i386/neo mac/include mac/all \
	pine

COMPRESS = $(COMPRESS_PROG) -f -9
LOCAL_CLEANUPDIRS =

if ENABLE_COMPRESS
SRC_KEYMAPDIR  = $(KEYMAPDIR)_Z
SRC_FONTDIR    = $(FONTDIR)_Z
SRC_PARTIALDIR = $(PARTIALDIR)_Z

LOCAL_CLEANUPDIRS += $(SRC_KEYMAPDIR) $(SRC_FONTDIR) $(SRC_PARTIALDIR)
else
SRC_KEYMAPDIR  = $(KEYMAPDIR)
SRC_FONTDIR    = $(FONTDIR)
SRC_PARTIALDIR = $(PARTIALDIR)
endif

install-data-hook: $(SRC_KEYMAPDIR) $(SRC_FONTDIR) $(SRC_PARTIALDIR)
	install -d -m 755 $(DESTDIR)$(datadir)
	install -d -m 755 $(DESTDIR)$(datadir)/$(FONTDIR)
	install -m 644 $(SRC_FONTDIR)/* $(DESTDIR)$(datadir)/$(FONTDIR)
	install -d -m 755 $(DESTDIR)$(datadir)/$(FONTDIR)/$(PARTIALDIR)
	install -m 644 $(SRC_PARTIALDIR)/* $(DESTDIR)$(datadir)/$(FONTDIR)/$(PARTIALDIR)
	install -d -m 755 $(DESTDIR)$(datadir)/$(TRANSDIR)
	install -m 644 $(srcdir)/$(TRANSDIR)/* $(DESTDIR)$(datadir)/$(TRANSDIR)
	install -d -m 755 $(DESTDIR)$(datadir)/$(UNIMAPDIR)
	install -m 644 $(srcdir)/$(UNIMAPDIR)/* $(DESTDIR)$(datadir)/$(UNIMAPDIR)
	install -d -m 755 $(DESTDIR)$(datadir)/$(KEYMAPDIR)
	install -d -m 755 $(DESTDIR)$(datadir)/$(KEYMAPDIR)/i386
	install -d -m 755 $(DESTDIR)$(datadir)/$(KEYMAPDIR)/mac
	for i in $(KEYMAPSUBDIRS); do \
		install -d -m 755 $(DESTDIR)$(datadir)/$(KEYMAPDIR)/$$i ;\
		install -m 644 $(SRC_KEYMAPDIR)/$$i/* $(DESTDIR)$(datadir)/$(KEYMAPDIR)/$$i ;\
	done
	rm -f $(DESTDIR)$(datadir)/$(KEYMAPDIR)/ppc; $(LN_S) mac $(DESTDIR)$(datadir)/$(KEYMAPDIR)/ppc
	@if [ -f $(DESTDIR)$(datadir)/$(OLDKEYMAPDIR) ]; then \
	  echo "Done. You may want to remove old keymaps with" ; \
	  echo "  rm -rf $(DESTDIR)$(datadir)/$(OLDKEYMAPDIR)" ; \
	  echo "But be careful to preserve your default map if it is" ; \
	  echo "nonstandard, and to adapt any scripts in rc.local or so." ; \
	fi

# compress data files - do not touch the distribution but copy first
$(KEYMAPDIR)_Z:
	cp -r $(srcdir)/$(KEYMAPDIR) $(SRC_KEYMAPDIR)
	chmod -R u+w $(SRC_KEYMAPDIR)
	for f in "$(SRC_KEYMAPDIR)"/*/*.map "$(SRC_KEYMAPDIR)"/*/*/*.map; do \
	  test -L "$$f" && t="`readlink -m $$f`" && rm -f -- "$$f" && \
	  cp -- "$$t" "$$f" ||:; \
	done
	for f in "$(SRC_KEYMAPDIR)"/*/*.map "$(SRC_KEYMAPDIR)"/*/*/*.map; do \
	  test -f "$$f" && $(COMPRESS) "$$f" && rm -f -- "$$f"; \
	done

$(FONTDIR)_Z:
	cp -r "$(srcdir)/$(FONTDIR)" "$(SRC_FONTDIR)"
	chmod -R u+w $(SRC_FONTDIR)
	for f in "$(SRC_FONTDIR)"/*; do \
	  case "$$f" in (*/ERRORS*|*/README*) continue; ;; esac; \
	  test -f "$$f" && $(COMPRESS) "$$f" && rm -f -- "$$f"; \
	done

$(PARTIALDIR)_Z:
	cp -r "$(srcdir)/$(PARTIALDIR)" "$(SRC_PARTIALDIR)"
	chmod -R u+w $(SRC_PARTIALDIR)
	for f in "$(SRC_PARTIALDIR)"/*; do \
	  test -f "$$f" && $(COMPRESS) "$$f" && rm -f -- "$$f"; \
	done

# (not yet screenmaps - some other time)

clean-local:
	for d in $(LOCAL_CLEANUPDIRS); do \
	  test -d "$$d" && rm -rf -- "$$d" ||:; \
	done

uninstall-hook:
	cd $(DESTDIR)$(datadir) && rm -rf $(FONTDIR) $(PARTIALDIR) $(TRANSDIR) $(UNIMAPDIR) $(KEYMAPDIR)
