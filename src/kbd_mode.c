/*
 * kbd_mode: report and set keyboard mode - aeb, 940406
 * 
 * If you make \215A\201 an alias for "kbd_mode -a", and you are
 * in raw mode, then hitting F7 = (two keys) will return you to sanity.
 */

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <linux/kd.h>
#include "getfd.h"
#include "nls.h"
#include "version.h"

static void __attribute__ ((noreturn))
usage(void){
    fprintf(stderr, _("usage: kbd_mode [-a|-u|-k|-s] [-C device]\n"));
    exit(1);
}

int
main(int argc, char *argv[]){
        int fd, mode, c, n = 0;
        char *console = NULL;

	set_progname(argv[0]);

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_NAME, LOCALEDIR);
	textdomain(PACKAGE_NAME);

	if (argc == 2 && !strcmp(argv[1], "-V"))
	    print_version_and_exit();

	while ((c = getopt(argc, argv, "auskC:")) != EOF) {
		switch (c) {
		case 'a':
                        if (n > 0)
                            usage ();
			mode = K_XLATE;
			n++;
			break;
		case 'u':
                        if (n > 0)
                            usage ();
			mode = K_UNICODE;
			n++;
			break;
		case 's':
                        if (n > 0)
                            usage ();
			mode = K_RAW;
			n++;
			break;
		case 'k':
                        if (n > 0)
                            usage ();
			mode = K_MEDIUMRAW;
			n++;
			break;
		case 'C':
                        if (!optarg || !optarg[0])
			    usage ();
			console = optarg;
			break;
		default:
			usage();
		}
	}

	fd = getfd(console);

	if (n == 0) {
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
	    exit(0);
	}

	if (ioctl(fd, KDSKBMODE, mode)) {
		perror("KDSKBMODE");
		fprintf(stderr, _("%s: error setting keyboard mode\n"), progname);
		exit(1);
	}
	exit(0);
}
