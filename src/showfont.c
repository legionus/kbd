/* showfont.c - aeb, 940207 - updated 2001-02-06 */

#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include "nls.h"
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

static void
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

static void
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

static void
usage(void) {
	fprintf(stderr,
		_("usage: showfont [-v|-V]\n"
		  "(probably after loading a font with `setfont font')\n"));
	exit(1);
}

int
main (int argc, char **argv) {
	int n, cols, rows, nr, i, j, k;
	char *sep;
	int list[64], lth, verbose = 0;

	set_progname(argv[0]);

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	if (argc == 2 &&
	    (!strcmp(argv[1], "-V") || !strcmp(argv[1], "--version")))
		print_version_and_exit();

	if (argc == 2 && !strcmp(argv[1], "-v"))
		verbose = 1;
	else if (argc != 1)
		usage();

#if 0
	fd = getfd();
#endif
	settrivialscreenmap();
	getoldunicodemap();

	n = getfontsize(fd);
	if (verbose)
		printf("Showing %d-char font\n\n", n);
	cols = ((n > 256) ? 32 : 16);
	nr = 64/cols;
	rows = (n+cols-1)/cols;
	sep = ((cols == 16) ? "  " : " ");

	for (i=0; i<rows; i++) {
		if (i % nr == 0) {
			lth = 0;
			for (k=i; k<i+nr; k++)
				for (j=0; j < cols; j++)
					list[lth++] = k+j*rows;
			setnewunicodemap(list, lth);
		}
		printf("    ");
		for(j=0; j < cols && i+j*rows < n; j++) {
			putchar(BASE + (i%nr)*cols+j);
			printf(sep);
			if (j%8 == 7)
				printf(sep);
		}
		putchar('\n');
		if (i%8 == 7)
			putchar('\n');
		fflush(stdout);
	}

	leave(0);
	exit(0);			/* make gcc happy */
}
