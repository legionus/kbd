# Generated files
GEN1=man1/dumpkeys.1 man1/loadkeys.1
GEN8=man8/setfont.8 man8/loadunimap.8 man8/mapscrn.8

install:
	for i in man?; do \
		install -d -m 755 $(MANDIR)/$$i; \
		install -m 644 $$i/*.[0-9] $(MANDIR)/$$i; \
	done
	rm -f $(MANDIR)/man5/keytables.5

clean:
	rm -f $(GEN1) $(GEN8)
