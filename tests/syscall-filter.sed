#!/bin/sed -rnf

/^ioctl/ {
	s/^ioctl\([0-9]+,/ioctl(<fd>,/;
	s/\<0x([0-9]|[A-Fa-f]){12,}\>/<ptrval>/g;

	# Hack for musl
	/, TIOCGWINSZ, /d;

	/^ioctl\(<fd>, (KD|KIO|TIO|GIO_|PIO_|VT_)[A-Z]+, /p;
}
