/*
 * kbd_mode: report and set keyboard mode - aeb, 940406
 * 
 * If you make \215A\201 an alias for "kbd_mode -a", and you are
 * in raw mode, then hitting F7 = (two keys) will return you to sanity.
 */
#include <fcntl.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/kd.h>
#include "nls.h"

char *progname;

extern int getfd(void);

void
usage(){
    fprintf(stderr, _("usage: kbd_mode [-a|-u|-k|-s]\n"));
    exit(1);
}

int
main(int argc, char *argv[]){
        int fd, mode;

	progname = argv[0];

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	fd = getfd();

	if (argc == 1) {
	    /* report mode */
	    if (ioctl(fd, KDGKBMODE, &mode)) {
		perror("KDGKBMODE");
	        fprintf(stderr, _("kbd_mode: error reading keyboard mode\n"));
		exit(1);
	    }
	    switch(mode) {
	      case K_RAW:
		printf(_("The keyboard is in raw (scancode) mode\n"));
		break;
	      case K_MEDIUMRAW:
		printf(_("The keyboard is in mediumraw (keycode) mode\n"));
		break;
	      case K_XLATE:
		printf(_("The keyboard is in the default (ASCII) mode\n"));
		break;
	      case K_UNICODE:
		printf(_("The keyboard is in Unicode (UTF-8) mode\n"));
		break;
	      default:
		printf(_("The keyboard is in some unknown mode\n"));
	    }
	    exit(1);
	}
	if (argc != 2)
	  usage();
	if (!strcmp(argv[1], "-a"))
	  mode = K_XLATE;
	else if (!strcmp(argv[1], "-u"))
	  mode = K_UNICODE;
	else if (!strcmp(argv[1], "-s"))
	  mode = K_RAW;
	else if (!strcmp(argv[1], "-k"))
	  mode = K_MEDIUMRAW;
	else
	  usage();
	if (ioctl(fd, KDSKBMODE, mode)) {
		perror("KDSKBMODE");
		fprintf(stderr, _("%s: error setting keyboard mode\n"), progname);
		exit(1);
	}
	exit(0);
}
