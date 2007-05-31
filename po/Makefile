PACKAGE = kbd
VERSION = 1.13

include ../make_include

# po2tbl.sed is not taken from /usr/share/gettext/intl
# Often it is not present.
# A RedHat 5.2 installation only has po2tbl.sed.in.
#
PO2TBL = ./po2tbl.sed
INTL = /usr/share/gettext/intl
# FOREIGN=--foreign-user
FOREIGN=
SHELL = /bin/sh

prefix = $(DESTDIR)/usr
datadir = $(prefix)/share
localedir = $(datadir)/locale
gnulocaledir = $(prefix)/share/locale
gettextsrcdir = $(prefix)/share/gettext/po

# Not giving an explicit path improves the chances of finding these
GENCAT = gencat
MSGFMT = msgfmt
XGETTEXT = xgettext
MSGMERGE = msgmerge
INSTALL = install -c
INSTALL_DATA = ${INSTALL} -m 644

# ..      for <config.h> (if HAVE_CONFIG_H is set)
# $(INTL) for "libgettext.h"
INCLUDES = -I.. -I$(INTL)

COMPILE = $(CC) -c $(DEFS) $(INCLUDES) $(CPPFLAGS) $(CFLAGS) $(XCFLAGS)

# Enter here all .po files
POFILES =  cs.po da.po de.po el.po es.po fr.po gr.po nl.po pl.po \
 ro.po ru.po sv.po tr.po
# the same but with .gmo
GMOFILES = cs.gmo da.gmo de.gmo el.gmo es.gmo fr.gmo gr.gmo nl.gmo \
 pl.gmo ro.gmo ru.gmo sv.gmo tr.gmo

CATALOGS = $(GMOFILES)
CATOBJEXT = .gmo
INSTOBJEXT = .mo

.SUFFIXES:
.SUFFIXES: .c .o .po .gmo .mo .msg .cat

.c.o:
	$(COMPILE) $<

.po.mo:
	$(MSGFMT) -o $@ $<

.po.gmo:
	$(MSGFMT) -o $@ $<

.po.cat:
	sed -f $(PO2TBL) < $< > $*.msg \
	  && rm -f $@ && $(GENCAT) $@ $*.msg

all: all-$(HAVE_XGETTEXT)

all-yes: cat-id-tbl.c $(CATALOGS)
all-no:

kbd.pot:
	$(XGETTEXT) --default-domain=kbd --directory=.. \
	  --add-comments --keyword=_ --keyword=N_ \
	  --files-from=POTFILES.in $(FOREIGN)
	mv kbd.po kbd.pot

cat-id-tbl.c: stamp-cat-id
stamp-cat-id: kbd.pot
	rm -f cat-id-tbl.tmp
	sed -f $(PO2TBL) kbd.pot \
		| sed -e "s/@PACKAGE NAME@/kbd/" > cat-id-tbl.tmp
	if cmp -s cat-id-tbl.tmp cat-id-tbl.c; then \
	  rm cat-id-tbl.tmp; \
	else \
	  echo cat-id-tbl.c changed; \
	  rm -f cat-id-tbl.c; \
	  mv cat-id-tbl.tmp cat-id-tbl.c; \
	fi
	rm -f stamp-cat-id && echo timestamp > stamp-cat-id

install: install-data-$(HAVE_XGETTEXT)
install-data-no: all
install-data-yes: all
	../mkinstalldirs $(datadir);
	@catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
	  case "$$cat" in \
	    *.gmo) destdir=$(gnulocaledir);; \
	    *)     destdir=$(localedir);; \
	  esac; \
	  lang=`echo $$cat | sed 's/$(CATOBJEXT)$$//'`; \
	  dir=$$destdir/$$lang/LC_MESSAGES; \
	  ../mkinstalldirs $$dir; \
	  $(INSTALL_DATA) $$cat $$dir/kbd$(INSTOBJEXT); \
	  echo "installing $$cat as $$dir/kbd$(INSTOBJEXT)"; \
	  if test -r $$cat.m; then \
	    $(INSTALL_DATA) $$cat.m $$dir/kbd$(INSTOBJEXT).m; \
	    echo "installing $$cat.m as $$dir/kbd$(INSTOBJEXT).m"; \
	  fi; \
	done

uninstall:
	catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
	  lang=`echo $$cat | sed 's/$(CATOBJEXT)$$//'`; \
	  rm -f $(localedir)/$$lang/LC_MESSAGES/kbd$(INSTOBJEXT); \
	  rm -f $(localedir)/$$lang/LC_MESSAGES/kbd$(INSTOBJEXT).m; \
	  rm -f $(gnulocaledir)/$$lang/LC_MESSAGES/kbd$(INSTOBJEXT); \
	  rm -f $(gnulocaledir)/$$lang/LC_MESSAGES/kbd$(INSTOBJEXT).m; \
	done
	rm -f $(gettextsrcdir)/po-Makefile.in.in

cat-id-tbl.o: $(INTL)/libgettext.h

clean:
	rm -f core core.* *~ *.o kbd.pot cat-id-tbl.tmp

distclean: clean
	rm -f POTFILES *.gmo *.mo *.msg *.cat *.cat.m

update-po: 
	$(MAKE) kbd.pot
	catalogs='$(CATALOGS)'; \
	for cat in $$catalogs; do \
	  lang=`echo $$cat | sed 's/$(CATOBJEXT)$$//'`; \
	  mv $$lang.po $$lang.old.po; \
	  echo "$$lang:"; \
	  if $(MSGMERGE) $$lang.old.po kbd.pot -o $$lang.po; then \
	    rm -f $$lang.old.po; \
	  else \
	    echo "msgmerge for $$cat failed!"; \
	    rm -f $$lang.po; \
	    mv $$lang.old.po $$lang.po; \
	  fi; \
	done

POTFILES: 
	./update-potfiles

