
CC=gcc
CFLAGS=-O2 -Wall -ansi
LDFLAGS=$(CFLAGS) -s 

SRC=openvt.c
OBJ=openvt.o getfd.o
PROG=openvt
PROGS=$(PROG)
MAN=openvt.1
# BINDIR=${DESTDIR}/usr/bin
# MANDIR=${DESTDIR}/usr/man

all: $(PROGS)

openvt: $(OBJ) 

openvt.o: openvt.h

clean:
	-rm -f $(OBJ)

realclean distclean: clean
	-rm -f $(PROGS)

install: $(PROGS)
	install -d -m 755 $(BINDIR) $(MANDIR)/man1
	install -c $(PROGS) $(BINDIR)
	install -c $(MAN) $(MANDIR)/man1

# don't change 4711 to 0711 - this is the setuid target
setuid:
	chmod 4711 $(BINDIR)/$(PROG)
