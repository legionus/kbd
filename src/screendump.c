/*
 * screendump.c - aeb 950214
 *
 * [Note: similar functionality can be found in setterm]
 *
 * Call: "screendump N" when the screen of /dev/ttyN has to be dumped
 *
 * On Linux up to 1.1.91 there is an ioctl that will do the dumping.
 * Because of problems with security this has been scrapped.
 * From 1.1.92 on, make devices "virtual console screen" and
 * "virtual console screen with attributes" by (fill in the ellipses):
 *	cd /dev
 *	for i in 0 1 2 3 ...; do
 *		mknod vcs$i c 7 $i
 *		mknod vcsa$i c 7 `expr 128 + $i`
 *	done
 * and give them your favourite owners and permissions.
 */
#include <stdio.h>
#include <linux/termios.h>
#include "nls.h"
extern char *malloc();

int
main(int argc, char **argv) {
    int cons = 0;
    char infile[20];
    unsigned char header[4];
    unsigned int rows, cols;
    int fd, i, j;
    char *inbuf, *outbuf, *p, *q;

    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);

    if (argc > 2) {
	fprintf(stderr, _("usage: screendump [n]\n"));
	exit(1);
    }

    cons = (argc == 2) ? atoi(argv[1]) : 0;

    sprintf(infile, "/dev/vcsa%d", cons);
    fd = open(infile, 0);
    if (fd < 0 || read(fd, header, 4) != 4)
      goto try_ioctl;
    rows = header[0];
    cols = header[1];
    if (rows * cols == 0)
      goto try_ioctl;
    inbuf = malloc(rows*cols*2);
    outbuf = malloc(rows*(cols+1));
    if(!inbuf || !outbuf) {
        fprintf(stderr, _("Out of memory?\n"));
        exit(1);
    }
    if (read(fd, inbuf, rows*cols*2) != rows*cols*2) {
        fprintf(stderr, _("Error reading %s\n"), infile);
        exit(1);
    }
    p = inbuf;
    q = outbuf;
    for(i=0; i<rows; i++) {
        for(j=0; j<cols; j++) {
            *q++ = *p;
            p += 2;
        }
        while(j-- > 0 && q[-1] == ' ')
          q--;
        *q++ = '\n';
    }
    goto done;

try_ioctl:
    {
	struct winsize win;
	char consnam[20];
	unsigned char *screenbuf;

	sprintf(consnam, "/dev/tty%d", cons);
	if((fd = open(consnam, 0)) < 0) {
	    perror(consnam);
	    fd = 0;
	}

	if (ioctl(fd,TIOCGWINSZ,&win)) {
	    perror("TIOCGWINSZ");
	    exit(1);
	}

	screenbuf = malloc(2 + win.ws_row * win.ws_col);
	if (!screenbuf) {
	    fprintf(stderr, _("Out of memory.\n"));
	    exit(1);
	}

	screenbuf[0] = 0;
	screenbuf[1] = (unsigned char) cons;

	if (ioctl(fd,TIOCLINUX,screenbuf) &&
	    (!fd || ioctl(0,TIOCLINUX,screenbuf))) {
	    perror("TIOCLINUX");
	    fprintf(stderr,_("couldn't read %s, and cannot ioctl dump\n"),
		    infile);
	    exit(1);
	}

        rows = screenbuf[0];
        cols = screenbuf[1];
	if (rows != win.ws_row || cols != win.ws_col) {
	    fprintf(stderr,
		    _("Strange ... screen is both %dx%d and %dx%d ??\n"),
		    win.ws_col, win.ws_row, cols, rows);
	    exit(1);
	}

	outbuf = malloc(rows*(cols+1));
	if(!outbuf) {
	    fprintf(stderr, _("Out of memory?\n"));
	    exit(1);
	}
	p = screenbuf + 2;
	q = outbuf;
        for (i=0; i<rows; i++) {
	    for (j=0; j<cols; j++)
	      *q++ = *p++;
            while (j-- > 0 && (q[-1] == ' '))
              q--;
	    *q++ = '\n';
        }
    }
done:
    if (write(1, outbuf, q-outbuf) != q-outbuf) {
        fprintf(stderr, _("Error writing screendump\n"));
        exit(1);
    }
    exit(0);
}
