/* showfont.c - aeb, 940207 - updated 2001-02-06 */
/* renamed to showconsolefont.c to avoid clash with the X showfont */

#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>
#include <sysexits.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/kd.h>

#include "libcommon.h"
#include "kfont.h"

/*
 * Showing the font is nontrivial mostly because testing whether
 * we are in utf8 mode cannot be done in an easy and clean way.
 * So, we set up things here in such a way that it does not matter
 * whether we are in utf8 mode.
 */

unsigned short obuf[E_TABSZ], nbuf[E_TABSZ];
struct unimapdesc ounimap, nunimap;
int fd           = 0;
int have_obuf    = 0;
int have_ounimap = 0;

static void __attribute__((noreturn))
leave(struct kfont_context *ctx, int n)
{
	if (have_obuf && loaduniscrnmap(ctx, fd, obuf)) {
		kbd_warning(0, _("failed to restore original translation table\n"));
		n = EXIT_FAILURE;
	}
	if (have_ounimap && loadunimap(ctx, fd, NULL, &ounimap)) {
		kbd_warning(0, _("failed to restore original unimap\n"));
		n = EXIT_FAILURE;
	}
	exit(n);
}

static void
settrivialscreenmap(struct kfont_context *ctx)
{
	unsigned short i;

	if (getuniscrnmap(ctx, fd, obuf))
		exit(1);
	have_obuf = 1;

	for (i = 0; i < E_TABSZ; i++)
		nbuf[i] = i;

	if (loaduniscrnmap(ctx, fd, nbuf)) {
		kbd_error(EXIT_FAILURE, 0, _("cannot change translation table\n"));
	}
}

static void
getoldunicodemap(struct kfont_context *ctx)
{
	struct unimapdesc descr;

	if (getunimap(ctx, fd, &descr))
		leave(ctx, EXIT_FAILURE);
	ounimap = descr;
	have_ounimap = 1;
}

#define BASE 041 /* ' '+1 */

static void
setnewunicodemap(struct kfont_context *ctx, int *list, int cnt)
{
	unsigned short i;

	if (!nunimap.entry_ct) {
		nunimap.entry_ct = 512;

		nunimap.entries  = malloc(nunimap.entry_ct * sizeof(struct unipair));
		if (!nunimap.entries) {
			KFONT_ERR(ctx, "malloc: %m");
			exit(EX_OSERR);
		}
	}
	for (i = 0; i < 512; i++) {
		nunimap.entries[i].fontpos = i;
		nunimap.entries[i].unicode = 0;
	}
	for (i = 0; i < cnt; i++)
		nunimap.entries[list[i]].unicode = (unsigned short) (BASE + i);

	if (loadunimap(ctx, fd, NULL, &nunimap))
		leave(ctx, EXIT_FAILURE);
}

static void __attribute__((noreturn))
usage(void)
{
	fprintf(stderr,
	        _("usage: showconsolefont -V|--version\n"
	          "       showconsolefont [-C tty] [-v] [-i]\n"
	          "(probably after loading a font with `setfont font')\n"
	          "\n"
	          "Options:\n"
	          "  -C tty                device to read the font from. Default: current tty;\n"
	          "  -v                    be more verbose;\n"
	          "  -i                    don't print out the font table, just show;\n"
	          "                        ROWSxCOLSxCOUNT and exit;\n"
	          "  -V, --version         print version number.\n"
	));
	exit(EXIT_FAILURE);
}

int main(int argc, char **argv)
{
	int c;
	unsigned int cols, rows, nr, n, i, j, k;
	int mode;
	const char *space, *sep;
	char *console = NULL;
	int list[64], lth, info = 0;

	set_progname(argv[0]);
	setuplocale();

	struct kfont_context ctx = {
		.progname = get_progname(),
		.verbose = 0,
		.log_fn = kfont_log_stderr,
	};

	if (argc == 2 &&
	    (!strcmp(argv[1], "-V") || !strcmp(argv[1], "--version")))
		print_version_and_exit();

	while ((c = getopt(argc, argv, "ivC:")) != EOF) {
		switch (c) {
			case 'i':
				info = 1;
				break;
			case 'v':
				ctx.verbose++;
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

	if ((fd = getfd(console)) < 0)
		kbd_error(EXIT_FAILURE, 0, _("Couldn't get a file descriptor referring to the console"));

	if (ioctl(fd, KDGKBMODE, &mode)) {
		kbd_warning(errno, "ioctl KDGKBMODE");
		leave(&ctx, EXIT_FAILURE);
	}
	if (mode == K_UNICODE)
		space = "\xef\x80\xa0"; /* U+F020 (direct-to-font space) */
	else
		space = " ";

	if (info) {
		nr = rows = cols = 0;

		n = getfont(&ctx, fd, NULL, &nr, &rows, &cols);
		if (n != 0)
			leave(&ctx, EXIT_FAILURE);

		if (ctx.verbose) {
			printf(_("Character count: %u\n"), nr);
			printf(_("Font width     : %u\n"), rows);
			printf(_("Font height    : %u\n"), cols);
		} else
			printf("%dx%dx%d\n", rows, cols, nr);
		leave(&ctx, EXIT_SUCCESS);
	}

	settrivialscreenmap(&ctx);
	getoldunicodemap(&ctx);

	n = getfontsize(&ctx, fd);
	if (ctx.verbose)
		printf(_("Showing %d-char font\n\n"), n);
	cols = ((n > 256) ? 32 : 16);
	nr   = 64 / cols;
	rows = (n + cols - 1) / cols;
	sep  = ((cols == 16) ? "%1$s%1$s" : "%1$s");

	for (i = 0; i < rows; i++) {
		if (i % nr == 0) {
			lth = 0;
			for (k = i; k < i + nr; k++)
				for (j              = 0; j < cols; j++)
					list[lth++] = k + j * rows;
			setnewunicodemap(&ctx, list, lth);
		}
		printf("%1$s%1$s%1$s%1$s", space);
		for (j = 0; j < cols && i + j * rows < n; j++) {
			putchar(BASE + (i % nr) * cols + j);
			printf(sep, space);
			if (j % 8 == 7)
				printf(sep, space);
		}
		putchar('\n');
		if (i % 8 == 7)
			putchar('\n');
		fflush(stdout);
	}

	leave(&ctx, EXIT_SUCCESS);
	return EXIT_SUCCESS;
}
