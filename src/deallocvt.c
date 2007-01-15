/*
 * disalloc.c - aeb - 940501 - Disallocate virtual terminal(s)
 * Renamed deallocvt.
 */
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
#include <stdio.h>
#include "nls.h"

extern int getfd(void);
char *progname;

void
usage(){
    fprintf(stderr, _("usage: %s [N1 N2 ...]\n"), progname);
    exit(1);
}

int
main(int argc, char *argv[]) {
    int fd, num, i;

    if (argc < 1)		/* unlikely */
      exit(1);
    progname = argv[0];

    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);

    fd = getfd();

    if (argc == 1) {
	/* deallocate all unused consoles */
	if (ioctl(fd,VT_DISALLOCATE,0)) {
	    perror("VT_DISALLOCATE");
	    fprintf(stderr,
		    _("%s: deallocating all unused consoles failed\n"),
		    progname);
	    exit(1);
	}
    } else
    for (i = 1; i < argc; i++) {
	num = atoi(argv[i]);
	if (num == 0)
	    fprintf(stderr, _("%s: 0: illegal VT number\n"), progname);
	else if (num == 1)
	    fprintf(stderr,
		    _("%s: VT 1 is the console and cannot be deallocated\n"),
		    progname);
	else
	if (ioctl(fd,VT_DISALLOCATE,num)) {
	    perror("VT_DISALLOCATE");
	    fprintf(stderr, _("%s: could not deallocate console %d\n"),
		    progname, num);
	    exit(1);
	}
    }
    exit(0);
}
