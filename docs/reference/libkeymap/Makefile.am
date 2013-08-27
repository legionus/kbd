DOC_MODULE  = libkeymap

EXTRA_DIST  = Doxyfile.in
CLEANFILES  = Doxyfile

install-data-hook:
	sed \
		-e 's,[@]VERSION[@],$(VERSION),g' \
		-e 's,[@]MODULE[@],$(DOC_MODULE),g' \
		-e 's,[@]TOPDIR[@],$(abs_top_srcdir),g' \
		-e 's,[@]SOURCEDIR[@],$(abs_top_srcdir)/src/$(DOC_MODULE),g' \
		-e 's,[@]BUILDDIR[@],$(DESTDIR)$(docdir),g' \
		< $(srcdir)/Doxyfile.in >$(builddir)/Doxyfile
	mkdir -p -- $(DESTDIR)$(docdir)
	doxygen $(builddir)/Doxyfile

uninstall-hook:
	rm -rf -- $(DESTDIR)$(docdir)/html
