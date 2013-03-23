/* showfont.c - aeb, 940207 - updated 2001-02-06 */
/* renamed to showconsolefont.c to avoid clash with the X showfont */

#include <stdio.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include "nls.h"
#include "getfd.h"
#include "version.h"
#include "kdmapop.h"
#include "kdfontop.h"

/*
 * Showing the font is nontrivial mostly because testing whether
 * we are in utf8 mode cannot be done in an easy and clean way.
 * So, we set up things here in such a way that it does not matter
 * whether we are in utf8 mode.
 */

unsigned short obuf[E_TABSZ], nbuf[E_TABSZ];
struct unimapdesc ounimap, nunimap;
int fd = 0;
int have_obuf = 0;
int have_ounimap = 0;

static void __attribute__ ((noreturn))
leave(int n) {
	if (have_obuf && loaduniscrnmap(fd,obuf)) {
		fprintf(stderr,
			_("failed to restore original translation table\n"));
		n = 1;
	}
	if (have_ounimap && loadunimap(fd,NULL,&ounimap)) {
		fprintf(stderr,
			_("failed to restore original unimap\n"));
		n = 1;
	}
	exit(n);
}

static void
settrivialscreenmap(void) {
	int i;

	if (getuniscrnmap(fd,obuf))
		exit(1);
	have_obuf = 1;

	for(i=0; i<E_TABSZ; i++)
		nbuf[i] = i;

	if (loaduniscrnmap(fd,nbuf)) {
		fprintf(stderr, _("cannot change translation table\n"));
		exit(1);
	}
}

static void __attribute__ ((noreturn))
out_of_memory(void) {
	fprintf(stderr, _("%s: out of memory?\n"), progname);
	leave(1);
}

static void
getoldunicodemap(void) {
	struct unimapdesc descr;

	if (getunimap(fd, &descr))
		leave(1);
	ounimap = descr;
	have_ounimap = 1;
}

#define BASE	041		/* ' '+1 */

static void
setnewunicodemap(int *list, int cnt) {
	int i;

	if (!nunimap.entry_ct) {
		nunimap.entry_ct = 512;
		nunimap.entries = (struct unipair *)
			malloc(nunimap.entry_ct * sizeof(struct unipair));
		if (nunimap.entries == NULL)
			out_of_memory();
	}
	for (i=0; i<512; i++) {
		nunimap.entries[i].fontpos = i;
		nunimap.entries[i].unicode = 0;
	}
	for (i=0; i<cnt; i++)
		nunimap.entries[list[i]].unicode = BASE+i;

	if (loadunimap(fd, NULL, &nunimap))
		leave(1);
}

static void __attribute__ ((noreturn))
usage(void) {
	fprintf(stderr,
		_("usage: showconsolefont -V|--version\n"
		  "       showconsolefont [-C tty] [-v] [-i]\n"
		  "(probably after loading a font with `setfont font')\n"
		  "\n"
		  "Valid options are:\n"
		  " -C tty   Device to read the font from. Default: current tty.\n"
		  " -v       Be more verbose.\n"
		  " -i       Don't print out the font table, just show\n"
		  "          ROWSxCOLSxCOUNT and exit.\n"));
	exit(1);
}

int
main (int argc, char **argv) {
	int c, n, cols, rows, nr, i, j, k;
	int mode;
	char *space, *sep, *console = NULL;
	int list[64], lth, info = 0, verbose = 0;

	set_progname(argv[0]);

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_NAME, LOCALEDIR);
	textdomain(PACKAGE_NAME);

	if (argc == 2 &&
	    (!strcmp(argv[1], "-V") || !strcmp(argv[1], "--version")))
		print_version_and_exit();

	while ((c = getopt(argc, argv, "ivC:")) != EOF) {
		switch (c) {
		case 'i':
			info = 1;
			break;
		case 'v':
			verbose = 1;
			break;
		case 'C':
			console = optarg;
			break;
		default:
			usage();
		}
	}

	if (optind != argc)
		usage();

	fd = getfd(console);

	if (ioctl(fd, KDGKBMODE, &mode)) {
		perror("KDGKBMODE");
		leave(1);
	}
	if (mode == K_UNICODE)
		space = "\xef\x80\xa0";	/* U+F020 (direct-to-font space) */
	else
		space = " ";

        if (info) {
	    nr = rows = cols = 0;
	    n = getfont(fd, NULL, &nr, &rows, &cols);
	    if (n != 0)
	      leave(1);

	    if (verbose) {
	        printf(_("Character count: %d\n"), nr);
		printf(_("Font width     : %d\n"), rows);
		printf(_("Font height    : %d\n"), cols);
	    }
	    else
		printf("%dx%dx%d\n", rows, cols, nr);
	    leave(0);
	  }

	settrivialscreenmap();
	getoldunicodemap();

	n = getfontsize(fd);
	if (verbose)
		printf(_("Showing %d-char font\n\n"), n);
	cols = ((n > 256) ? 32 : 16);
	nr = 64/cols;
	rows = (n+cols-1)/cols;
	sep = ((cols == 16) ? "%1$s%1$s" : "%1$s");

	for (i=0; i<rows; i++) {
		if (i % nr == 0) {
			lth = 0;
			for (k=i; k<i+nr; k++)
				for (j=0; j < cols; j++)
					list[lth++] = k+j*rows;
			setnewunicodemap(list, lth);
		}
		printf("%1$s%1$s%1$s%1$s", space);
		for(j=0; j < cols && i+j*rows < n; j++) {
			putchar(BASE + (i%nr)*cols+j);
			printf(sep, space);
			if (j%8 == 7)
				printf(sep, space);
		}
		putchar('\n');
		if (i%8 == 7)
			putchar('\n');
		fflush(stdout);
	}

	leave(0);
	exit(0);			/* make gcc happy */
}
