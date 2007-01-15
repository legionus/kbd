install: $(KEYMAPDIR)_Z $(FONTDIR)_Z
	install -d -m 755 $(DATADIR)
	install -d -m 755 $(DATADIR)/$(FONTDIR)
	install -m 644 $(FONTDIR)_Z/* $(DATADIR)/$(FONTDIR)
	install -d -m 755 $(DATADIR)/$(FONTDIR)/$(PARTIALDIR)
	install -m 644 $(PARTIALDIR)_Z/* $(DATADIR)/$(FONTDIR)/$(PARTIALDIR)
	install -d -m 755 $(DATADIR)/$(TRANSDIR)
	install -m 644 $(TRANSDIR)/* $(DATADIR)/$(TRANSDIR)
	install -d -m 755 $(DATADIR)/$(UNIMAPDIR)
	install -m 644 $(TRANSDIR)/* $(DATADIR)/$(UNIMAPDIR)
	install -d -m 755 $(DATADIR)/$(KEYMAPDIR)
	install -d -m 755 $(DATADIR)/$(KEYMAPDIR)/i386
	for i in $(KEYMAPSUBDIRS); do \
		install -d -m 755 $(DATADIR)/$(KEYMAPDIR)/$$i ;\
		install -m 644 $(KEYMAPDIR)_Z/$$i/* $(DATADIR)/$(KEYMAPDIR)/$$i ;\
	done
	rm -f $(DATADIR)/$(KEYMAPDIR)/ppc; ln -s mac $(DATADIR)/$(KEYMAPDIR)/ppc
	@if [ -f $(DATADIR)/$(OLDKEYMAPDIR) ]; then \
	  echo "Done. You may want to remove old keymaps with" ; \
	  echo "  rm -rf $(DATADIR)/$(OLDKEYMAPDIR)" ; \
	  echo "But be careful to preserve your default map if it is" ; \
	  echo "nonstandard, and to adapt any scripts in rc.local or so." ; \
	fi


# compress data files - do not touch the distribution but copy first
$(KEYMAPDIR)_Z:
	cp -r $(KEYMAPDIR) $(KEYMAPDIR)_Z
	cd $(KEYMAPDIR)_Z && $(MYGZIP) */*.map */*/*.map

$(FONTDIR)_Z:
	cp -r $(FONTDIR) $(FONTDIR)_Z
	cp -r $(PARTIALDIR) $(PARTIALDIR)_Z
	cd $(FONTDIR)_Z && $(MYGZIP) * && gunzip README*
	cd $(PARTIALDIR)_Z && $(MYGZIP) *
# (not yet screenmaps - some other time)

clean:
	rm -rf $(KEYMAPDIR)_Z $(FONTDIR)_Z $(PARTIALDIR)_Z
