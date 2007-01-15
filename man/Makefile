install:
	for i in man?; do \
		install -d -m 755 $(MANDIR)/$$i; \
		install -m 644 $$i/*.[0-9] $(MANDIR)/$$i; \
	done
	rm -f $(MANDIR)/man5/keytables.5

clean:
	rm -f man1/dumpkeys.1 man1/loadkeys.1 man8/setfont.8
