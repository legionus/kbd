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
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/ioctl.h>
#include "xmalloc.h"
#include "nls.h"
#include "version.h"

int
main(int argc, char **argv) {
    int cons = 0;
    char infile[20];
    unsigned char header[4];
    unsigned int rows, cols;
    int fd;
    unsigned int i, j;
    char *inbuf, *outbuf, *p, *q;

    set_progname(argv[0]);

    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE_NAME, LOCALEDIR);
    textdomain(PACKAGE_NAME);

    if (argc == 2 && !strcmp(argv[1], "-V"))
	print_version_and_exit();

    if (argc > 2) {
	fprintf(stderr, _("usage: screendump [n]\n"));
	exit(1);
    }

    cons = (argc == 2) ? atoi(argv[1]) : 0;

    sprintf(infile, "/dev/vcsa%d", cons);
    fd = open(infile, O_RDONLY);
    if (fd < 0 && cons == 0 && errno == ENOENT) {
      sprintf(infile, "/dev/vcsa");
      fd = open(infile, O_RDONLY);
    }
    if (fd < 0 && errno == ENOENT) {
      sprintf(infile, "/dev/vcs/a%d", cons);
      fd = open(infile, O_RDONLY);
    }
    if (fd < 0 && cons == 0 && errno == ENOENT) {
      sprintf(infile, "/dev/vcs/a");
      fd = open(infile, O_RDONLY);
    }
    if (fd < 0 || read(fd, header, 4) != 4)
      goto try_ioctl;
    rows = header[0];
    cols = header[1];
    if (rows * cols == 0)
      goto try_ioctl;
    inbuf = xmalloc(rows*cols*2);
    outbuf = xmalloc(rows*(cols+1));

    if (read(fd, inbuf, rows*cols*2) != (ssize_t) (rows*cols*2)) {
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
	char consnam[20], devfsconsnam[20];
	unsigned char *screenbuf;

	sprintf(consnam, "/dev/tty%d", cons);
	fd = open(consnam, O_RDONLY);
	if (fd < 0 && errno == ENOENT) {
	    sprintf(devfsconsnam, "/dev/vc/%d", cons);
	    fd = open(devfsconsnam, O_RDONLY);
	    if (fd < 0)
		errno = ENOENT;
	}
	if (fd < 0) {
	    perror(consnam);
	    fd = 0;
	}

	if (ioctl(fd, TIOCGWINSZ, &win)) {
	    perror("TIOCGWINSZ");
	    exit(1);
	}

	screenbuf = xmalloc(2 + win.ws_row * win.ws_col);
	screenbuf[0] = 0;
	screenbuf[1] = (unsigned char) cons;

	if (ioctl(fd,TIOCLINUX,screenbuf) &&
	    (!fd || ioctl(0,TIOCLINUX,screenbuf))) {
#if 0
	    perror("TIOCLINUX");
	    fprintf(stderr,_("couldn't read %s, and cannot ioctl dump\n"),
		    infile);
#else
	    /* we tried this just to be sure, but TIOCLINUX
	       function 0 has been disabled since 1.1.92
	       Do not mention `ioctl dump' in error msg */
	    fprintf(stderr,_("couldn't read %s\n"), infile);
#endif
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

	outbuf = xmalloc(rows*(cols+1));
	p = ((char *)screenbuf) + 2;
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
