#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <unistd.h>
#include <sysexits.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include "kdmapop.h"

#include "libcommon.h"

#ifndef USE_LIBC
/* There is such function in libc5 but it doesn't work for me [libc 5.4.13] */
#include "wctomb.c"
#define wctomb our_wctomb
#endif

static int
ud_compar(const void *u1, const void *u2)
{
	unsigned short fp1 = ((struct unipair *)u1)->fontpos;
	unsigned short fp2 = ((struct unipair *)u2)->fontpos;
	return (int)fp1 - (int)fp2;
}

static void __attribute__((noreturn))
usage(int rc)
{
	fprintf(stderr, _("Usage:\n\t%s [-s] [-C console]\n"), get_progname());
	exit(rc);
}

int main(int argc, char **argv)
{
	int sortflag = 0;
	char mb[]    = { 0, 0, 0, 0, 0, 0, 0, 0 };
	int mb_length;
	int fd, c, i;
	char *console = NULL;
	struct unimapdesc ud;

	set_progname(argv[0]);
	setuplocale();

	if (argc == 2 &&
	    (!strcmp(argv[1], "-V") || !strcmp(argv[1], "--version")))
		print_version_and_exit();

	while ((c = getopt(argc, argv, "sC:")) != EOF) {
		switch (c) {
			case 's':
				sortflag = 1;
				break;
			case 'C':
				console = optarg;
				break;
			default:
				usage(EX_USAGE);
		}
	}

	if (optind < argc)
		usage(EX_USAGE);

	if ((fd = getfd(console)) < 0)
		kbd_error(EXIT_FAILURE, 0, _("Couldn't get a file descriptor referring to the console"));

	if (getunimap(fd, &ud))
		return EXIT_FAILURE;

	if (sortflag) {
		printf("# sorted kernel unimap - count=%d\n", ud.entry_ct);
		/* sort and merge entries */
		qsort(ud.entries, ud.entry_ct, sizeof(ud.entries[0]),
		      ud_compar);
		for (i = 0; i < ud.entry_ct; i++) {
			int fp = ud.entries[i].fontpos;
			printf("0x%03x\tU+%04x", fp, ud.entries[i].unicode);
			while (i + 1 < ud.entry_ct &&
			       ud.entries[i + 1].fontpos == fp)
				printf(" U+%04x", ud.entries[++i].unicode);
			printf("\n");
		}
	} else {
		printf("# kernel unimap - count=%d\n", ud.entry_ct);
		for (i = 0; i < ud.entry_ct; i++) {
			mb_length = wctomb(mb, ud.entries[i].unicode);
			mb[(mb_length > 6) ? 0 : mb_length] = 0;
			if (mb_length == 1 && !isprint(mb[0])) {
				mb[2] = 0;
				mb[1] = mb[0] + 0100;
				mb[0] = '^';
			}
			printf("0x%03x\tU+%04x\t# %s \n",
			       ud.entries[i].fontpos,
			       ud.entries[i].unicode, mb);
		}
	}

	return EXIT_SUCCESS;
}
