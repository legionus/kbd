/*
 * call: getkeycodes
 *
 * aeb, 941108
 */
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include "getfd.h"
#include "nls.h"
#include "version.h"

static void
usage(void) {
    fprintf(stderr, _("usage: getkeycodes\n"));
    exit(1);
}

int
main(int argc, char **argv) {
    int fd, sc;
    struct kbkeycode a;

    set_progname(argv[0]);

    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);

    if (argc == 2 && !strcmp(argv[1], "-V"))
      print_version_and_exit();

    if (argc != 1)
      usage();
    fd = getfd();
    printf(_("Plain scancodes xx (hex) versus keycodes (dec)\n"));
    printf(_("0 is an error; for 1-88 (0x01-0x58) scancode equals keycode\n"));

    for(sc=88; sc<256; sc++) {
	if (sc == 128)
	  printf(_("\n\nEscaped scancodes e0 xx (hex)\n"));
	if (sc % 8 == 0) {
	    if (sc < 128)
	      printf("\n 0x%02x: ", sc);
	    else
	      printf("\ne0 %02x: ", sc-128);
	}

	if (sc <= 88) {
	    printf(" %3d", sc);
	    continue;
	}

	a.scancode = sc;
	a.keycode = 0;
	if (ioctl(fd,KDGETKEYCODE,&a)) {
	    if (errno == EINVAL)
	      printf("   -");
	    else {
		perror("KDGETKEYCODE");
		fprintf(stderr, _("failed to get keycode for scancode 0x%x\n"),
			sc);
		exit(1);
	    }
	} else
	  printf(" %3d", a.keycode);
    }
    printf("\n");
    return 0;
}
