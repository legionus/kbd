/*
 * setfont.c - Eugene Crosser & Andries Brouwer
 *
 * Version 1.05
 *
 * Loads the console font, and possibly the corresponding screen map(s).
 * We accept two kind of screen maps, one [-m] giving the correspondence
 * between some arbitrary 8-bit character set currently in use and the
 * font positions, and the second [-u] giving the correspondence between
 * font positions and Unicode values.
 */
#include "config.h"

#include <stdio.h>
#include <stdint.h>
#include <memory.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <endian.h>
#include <sysexits.h>
#include "paths.h"
#include "getfd.h"
#include "findfile.h"
#include "loadunimap.h"
#include "psf.h"
#include "psffontop.h"
#include "kdfontop.h"
#include "kdmapop.h"
#include "xmalloc.h"
#include "nls.h"
#include "version.h"
#include "kbd_error.h"

static int position_codepage(int def_height);
static void saveoldfont(int fd, char *outfile_font);
static void saveoldfontplusunicodemap(int fd, char *outfile_font_and_unicode_map);
static void loadnewfont(int fd, char *ifil,
                        int def_height, int font_height, int no_screen_map, int no_unicode_map);
static void loadnewfonts(int fd, char **ifiles, int ifilct,
                         int def_height, int font_height, int no_screen_map, int no_unicode_map);
extern void saveoldmap(int fd, char *outfile_screen_map);
extern void loadnewmap(int fd, char *screen_map);
extern void activatemap(int fd);
extern void disactivatemap(int fd);

int verbose = 0;
int force   = 0;
int debug   = 0;

/* search for the font in these directories (with trailing /) */
const char *const fontdirpath[]  = { "", DATADIR "/" FONTDIR "/", 0 };
const char *const fontsuffixes[] = { "", ".psfu", ".psf", ".cp", ".fnt", 0 };
/* hide partial fonts a bit - loading a single one is a bad idea */
const char *const partfontdirpath[]  = { "", DATADIR "/" FONTDIR "/" PARTIALDIR "/", 0 };
const char *const partfontsuffixes[] = { "", 0 };

static inline int
findfont(char *fnam, lkfile_t *fp)
{
	return lk_findfile(fnam, fontdirpath, fontsuffixes, fp);
}

static inline int
findpartialfont(char *fnam, lkfile_t *fp)
{
	return lk_findfile(fnam, partfontdirpath, partfontsuffixes, fp);
}

static void __attribute__((noreturn))
usage(void)
{
	fprintf(stderr, _(
	                    "Usage: setfont [write-options] [-<N>] [newfont..] [-m consolemap] [-u unicodemap]\n"
	                    "  write-options (take place before file loading):\n"
	                    "    -o  <filename>  Write current font to <filename>\n"
	                    "    -O  <filename>  Write current font and unicode map to <filename>\n"
	                    "    -om <filename>  Write current consolemap to <filename>\n"
	                    "    -ou <filename>  Write current unicodemap to <filename>\n"
	                    "If no newfont and no -[o|O|om|ou|m|u] option is given,\n"
	                    "a default font is loaded:\n"
	                    "    setfont         Load font \"default[.gz]\"\n"
	                    "    setfont -<N>    Load font \"default8x<N>[.gz]\"\n"
	                    "The -<N> option selects a font from a codepage that contains three fonts:\n"
	                    "    setfont -{8|14|16} codepage.cp[.gz]   Load 8x<N> font from codepage.cp\n"
	                    "Explicitly (with -m or -u) or implicitly (in the fontfile) given mappings\n"
	                    "will be loaded and, in the case of consolemaps, activated.\n"
	                    "    -h<N>      (no space) Override font height.\n"
	                    "    -m <fn>    Load console screen map.\n"
	                    "    -u <fn>    Load font unicode map.\n"
	                    "    -m none    Suppress loading and activation of a screen map.\n"
	                    "    -u none    Suppress loading of a unicode map.\n"
	                    "    -v         Be verbose.\n"
	                    "    -C <cons>  Indicate console device to be used.\n"
	                    "    -V         Print version and exit.\n"
	                    "Files are loaded from the current directory or %s/*/.\n"),
	        DATADIR);
	exit(EX_USAGE);
}

#define MAXIFILES 256

int main(int argc, char *argv[])
{
	char *ifiles[MAXIFILES];
	char *screen_map, *unicode_map, *outfile_font_and_unicode_map, *outfile_font, *outfile_screen_map, *outfile_unicode_map, *console;
	int ifilct  = 0, fd, i, def_height, font_height, no_screen_map, no_unicode_map;
	int restore = 0;

	set_progname(argv[0]);

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_NAME, LOCALEDIR);
	textdomain(PACKAGE_NAME);

	ifiles[0] = screen_map = unicode_map = outfile_font_and_unicode_map = outfile_font = outfile_screen_map = outfile_unicode_map = NULL;
	def_height = font_height = 0;
	no_screen_map = no_unicode_map = 0;
	console     = NULL;

	/*
	 * No getopt() here because of the -om etc options.
	 */
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-V")) {
			print_version_and_exit();
		} else if (!strcmp(argv[i], "-v")) {
			verbose++;
		} else if (!strcmp(argv[i], "-R")) {
			restore = 1;
		} else if (!strcmp(argv[i], "-C")) {
			if (++i == argc || console)
				usage();
			console = argv[i];
		} else if (!strcmp(argv[i], "-O")) {
			if (++i == argc || outfile_font_and_unicode_map)
				usage();
			outfile_font_and_unicode_map = argv[i];
		} else if (!strcmp(argv[i], "-o")) {
			if (++i == argc || outfile_font)
				usage();
			outfile_font = argv[i];
		} else if (!strcmp(argv[i], "-om")) {
			if (++i == argc || outfile_screen_map)
				usage();
			outfile_screen_map = argv[i];
		} else if (!strcmp(argv[i], "-ou")) {
			if (++i == argc || outfile_unicode_map)
				usage();
			outfile_unicode_map = argv[i];
		} else if (!strcmp(argv[i], "-m")) {
			if (++i == argc || screen_map)
				usage();
			if (!strcmp(argv[i], "none"))
				no_screen_map = 1;
			else
				screen_map = argv[i];
		} else if (!strcmp(argv[i], "-u")) {
			if (++i == argc || unicode_map)
				usage();
			if (!strcmp(argv[i], "none"))
				no_unicode_map = 1;
			else
				unicode_map = argv[i];
		} else if (!strcmp(argv[i], "-f")) {
			force = 1;
		} else if (!strncmp(argv[i], "-h", 2)) {
			font_height = atoi(argv[i] + 2);
			if (font_height <= 0 || font_height > MAX_FONT_HEIGHT)
				usage();
		} else if (argv[i][0] == '-') {
			def_height = atoi(argv[i] + 1);
			if (def_height <= 0 || def_height > MAX_FONT_HEIGHT)
				usage();
		} else {
			if (ifilct == MAXIFILES) {
				fprintf(stderr, _("setfont: too many input files\n"));
				exit(EX_USAGE);
			}
			ifiles[ifilct++] = argv[i];
		}
	}

	if (ifilct && restore) {
		fprintf(stderr, _("setfont: cannot both restore from character ROM"
		                  " and from file. Font unchanged.\n"));
		exit(EX_USAGE);
	}

	if ((fd = getfd(console)) < 0)
		kbd_error(EXIT_FAILURE, 0, _("Couldn't get a file descriptor referring to the console"));

	int kd_mode = -1;
	if (!ioctl(fd, KDGETMODE, &kd_mode) && (kd_mode == KD_GRAPHICS)) {
		/*
		 * PIO_FONT will fail on a console which is in foreground and in KD_GRAPHICS mode.
		 * 2005-03-03, jw@suse.de.
		 */
		if (verbose)
			printf("setfont: graphics console %s skipped\n", console ? console : "");
		close(fd);
		return 0;
	}

	if (!ifilct && !screen_map && !unicode_map &&
	    !outfile_font_and_unicode_map && !outfile_font && !outfile_screen_map && !outfile_unicode_map && !restore)
		/* reset to some default */
		ifiles[ifilct++] = "";

	if (outfile_font_and_unicode_map)
		saveoldfontplusunicodemap(fd, outfile_font_and_unicode_map);

	if (outfile_font)
		saveoldfont(fd, outfile_font);

	if (outfile_screen_map)
		saveoldmap(fd, outfile_screen_map);

	if (outfile_unicode_map)
		saveunicodemap(fd, outfile_unicode_map);

	if (screen_map) {
		loadnewmap(fd, screen_map);
		activatemap(fd);
		no_screen_map = 1;
	}

	if (unicode_map)
		no_unicode_map = 1;

	if (restore)
		restorefont(fd);

	if (ifilct)
		loadnewfonts(fd, ifiles, ifilct, def_height, font_height, no_screen_map, no_unicode_map);

	if (unicode_map)
		loadunicodemap(fd, unicode_map);

	return 0;
}

/*
 * 0 - do not test, 1 - test and warn, 2 - test and wipe, 3 - refuse
 */
static int erase_mode = 1;

static void
do_loadfont(int fd, char *inbuf, unsigned int width, unsigned int height,
            unsigned int font_height, unsigned int fontsize, char *filename)
{
	unsigned char *buf;
	unsigned int i, buflen;
	unsigned int bytewidth   = get_font_bytewidth(width);
	unsigned int charsize    = height * bytewidth;
	unsigned int kcharsize   = MAX_FONT_HEIGHT * bytewidth;
	int bad_video_erase_char = 0;

	if (height < 1 || height > MAX_FONT_HEIGHT) {
		fprintf(stderr, _("Bad character height %d\n"), height);
		exit(EX_DATAERR);
	}

	if (width < 1 || width > MAX_FONT_WIDTH) {
		fprintf(stderr, _("Bad character width %d\n"), width);
		exit(EX_DATAERR);
	}

	if (!font_height)
		font_height = height;

	buflen = kcharsize * ((fontsize < 128) ? 128 : fontsize);
	buf    = xmalloc(buflen);
	memset(buf, 0, buflen);

	for (i = 0; i < fontsize; i++)
		memcpy(buf + (i * kcharsize), inbuf + (i * charsize), charsize);

	/*
	 * Due to a kernel bug, font position 32 is used
	 * to erase the screen, regardless of maps loaded.
	 * So, usually this font position should be blank.
	 */
	if (erase_mode) {
		for (i = 0; i < kcharsize; i++)
			if (buf[32 * kcharsize + i])
				bad_video_erase_char = 1;
		if (bad_video_erase_char) {
			fprintf(stderr,
			        _("%s: font position 32 is nonblank\n"),
			        progname);
			switch (erase_mode) {
				case 3:
					xfree(buf);
					exit(EX_DATAERR);
				case 2:
					for (i = 0; i < kcharsize; i++)
						buf[32 * kcharsize + i] = 0;
					fprintf(stderr, _("%s: wiped it\n"), progname);
					break;
				case 1:
					fprintf(stderr,
					        _("%s: background will look funny\n"),
					        progname);
			}
			fflush(stderr);
			sleep(2);
		}
	}

	if (verbose) {
		if (height == font_height && filename)
			printf(_("Loading %d-char %dx%d font from file %s\n"),
			       fontsize, width, height, filename);
		else if (height == font_height)
			printf(_("Loading %d-char %dx%d font\n"),
			       fontsize, width, height);
		else if (filename)
			printf(_("Loading %d-char %dx%d (%d) font from file %s\n"),
			       fontsize, width, height, font_height, filename);
		else
			printf(_("Loading %d-char %dx%d (%d) font\n"),
			       fontsize, width, height, font_height);
	}

	if (putfont(fd, buf, fontsize, width, font_height))
		xfree(buf);
		exit(EX_OSERR);

	xfree(buf);
}

static void
do_loadtable(int fd, struct unicode_list *uclistheads, int fontsize)
{
	struct unimapdesc ud;
	struct unipair *up;
	int i, ct = 0, maxct;
	struct unicode_list *ul;
	struct unicode_seq *us;

	maxct = 0;
	for (i = 0; i < fontsize; i++) {
		ul = uclistheads[i].next;
		while (ul) {
			us = ul->seq;
			if (us && !us->next)
				maxct++;
			ul = ul->next;
		}
	}
	up = xmalloc(maxct * sizeof(struct unipair));
	for (i = 0; i < fontsize; i++) {
		ul = uclistheads[i].next;
		if (debug)
			printf("char %03x:", i);
		while (ul) {
			us = ul->seq;
			if (us && !us->next) {
				up[ct].unicode = us->uc;
				up[ct].fontpos = i;
				ct++;
				if (debug)
					printf(" %04x", us->uc);
			} else if (debug) {
				printf(" seq: <");
				while (us) {
					printf(" %04x", us->uc);
					us = us->next;
				}
				printf(" >");
			}
			ul = ul->next;
			if (debug)
				printf(",");
		}
		if (debug)
			printf("\n");
	}
	if (ct != maxct) {
		fprintf(stderr, _("%s: bug in do_loadtable\n"), progname);
		exit(EX_SOFTWARE);
	}

	if (verbose)
		printf(_("Loading Unicode mapping table...\n"));

	ud.entry_ct = ct;
	ud.entries  = up;
	if (loadunimap(fd, NULL, &ud))
		exit(EX_OSERR);
}

static void
loadnewfonts(int fd, char **ifiles, int ifilct,
             int def_height, int font_height, int no_screen_map, int no_unicode_map)
{
	char *inbuf, *fontbuf, *bigfontbuf;
	unsigned int inputlen, fontbuflen, fontsize, height, width, bytewidth;
	unsigned int bigfontbuflen, bigfontsize, bigheight, bigwidth;
	struct unicode_list *uclistheads;
	int i;
	lkfile_t fp;

	if (ifilct == 1) {
		loadnewfont(fd, ifiles[0], def_height, font_height, no_screen_map, no_unicode_map);
		return;
	}

	/* several fonts that must be merged */
	/* We just concatenate the bitmaps - only allow psf fonts */
	bigfontbuf    = NULL;
	bigfontbuflen = 0;
	bigfontsize   = 0;
	uclistheads   = NULL;
	bigheight     = 0;
	bigwidth      = 0;

	for (i = 0; i < ifilct; i++) {
		if (findfont(ifiles[i], &fp) && findpartialfont(ifiles[i], &fp)) {
			fprintf(stderr, _("Cannot open font file %s\n"), ifiles[i]);
			exit(EX_NOINPUT);
		}

		inbuf = fontbuf = NULL;
		inputlen = fontbuflen = 0;
		fontsize              = 0;

		if (readpsffont(fp.fd, &inbuf, &inputlen, &fontbuf, &fontbuflen,
		                &width, &fontsize, bigfontsize,
		                no_unicode_map ? NULL : &uclistheads)) {
			fprintf(stderr, _("When loading several fonts, all "
			                  "must be psf fonts - %s isn't\n"),
			        fp.pathname);
			xfree(inbuf);
			lk_fpclose(&fp);
			exit(EX_DATAERR);
		}
		lk_fpclose(&fp); // avoid zombies, jw@suse.de (#88501)

		bytewidth = get_font_bytewidth(width);
		height    = fontbuflen / (bytewidth * fontsize);
		if (verbose)
			printf(_("Read %d-char %dx%d font from file %s\n"),
			       fontsize, width, height, fp.pathname);

		if (bigheight == 0)
			bigheight = height;
		else if (bigheight != height) {
			fprintf(stderr, _("When loading several fonts, all "
			                  "must have the same height\n"));
			exit(EX_DATAERR);
		}
		if (bigwidth == 0)
			bigwidth = width;
		else if (bigwidth != width) {
			fprintf(stderr, _("When loading several fonts, all "
			                  "must have the same width\n"));
			exit(EX_DATAERR);
		}

		bigfontsize += fontsize;
		bigfontbuflen += fontbuflen;
		bigfontbuf = xrealloc(bigfontbuf, bigfontbuflen);
		memcpy(bigfontbuf + bigfontbuflen - fontbuflen,
		       fontbuf, fontbuflen);
	}
	do_loadfont(fd, bigfontbuf, bigwidth, bigheight, font_height,
	            bigfontsize, NULL);
	free(bigfontbuf);

	if (uclistheads && !no_unicode_map)
		do_loadtable(fd, uclistheads, bigfontsize);
}

static void
loadnewfont(int fd, char *ifil, int def_height, int font_height, int no_screen_map, int no_unicode_map)
{
	lkfile_t fp;
	char defname[20];
	unsigned int height, width, bytewidth, def = 0;
	char *inbuf, *fontbuf;
	unsigned int inputlen, fontbuflen, fontsize, offset;
	struct unicode_list *uclistheads;

	if (!*ifil) {
		/* try to find some default file */

		def = 1; /* maybe also load default unimap */

		if (def_height < 0 || def_height > MAX_FONT_HEIGHT)
			def_height = 0;
		if (def_height == 0) {
			if (findfont(ifil = "default", &fp) &&
			    findfont(ifil = "default8x16", &fp) &&
			    findfont(ifil = "default8x14", &fp) &&
			    findfont(ifil = "default8x8", &fp)) {
				fprintf(stderr, _("Cannot find default font\n"));
				exit(EX_NOINPUT);
			}
		} else {
			sprintf(defname, "default8x%d", def_height);
			if (findfont(ifil = defname, &fp) &&
			    findfont(ifil = "default", &fp)) {
				fprintf(stderr, _("Cannot find %s font\n"), ifil);
				exit(EX_NOINPUT);
			}
		}
	} else {
		if (findfont(ifil, &fp)) {
			fprintf(stderr, _("Cannot open font file %s\n"), ifil);
			exit(EX_NOINPUT);
		}
	}

	if (verbose > 1)
		printf(_("Reading font file %s\n"), ifil);

	inbuf = fontbuf = NULL;
	inputlen = fontbuflen = fontsize = 0;
	width                            = 8;
	uclistheads                      = NULL;
	if (readpsffont(fp.fd, &inbuf, &inputlen, &fontbuf, &fontbuflen,
	                &width, &fontsize, 0,
	                no_unicode_map ? NULL : &uclistheads) == 0) {
		lk_fpclose(&fp);

		/* we've got a psf font */
		bytewidth = get_font_bytewidth(width);
		height    = fontbuflen / (bytewidth * fontsize);

		do_loadfont(fd, fontbuf, width, height, font_height,
		            fontsize, fp.pathname);
		if (uclistheads && !no_unicode_map)
			do_loadtable(fd, uclistheads, fontsize);
#if 1
		if (!uclistheads && !no_unicode_map && def)
			loadunicodemap(fd, "def.uni");
#endif
		return;
	}
	lk_fpclose(&fp); // avoid zombies, jw@suse.de (#88501)

	/* instructions to combine fonts? */
	{
		char *combineheader = "# combine partial fonts\n";
		unsigned int chlth  = strlen(combineheader);
		char *p, *q;
		if (inputlen >= chlth && !strncmp(inbuf, combineheader, chlth)) {
			char *ifiles[MAXIFILES];
			int ifilct = 0;
			q          = inbuf + chlth;
			while (q < inbuf + inputlen) {
				p = q;
				while (q < inbuf + inputlen && *q != '\n')
					q++;
				if (q == inbuf + inputlen) {
					fprintf(stderr,
					        _("No final newline in combine file\n"));
					exit(EX_DATAERR);
				}
				*q++ = 0;
				if (ifilct == MAXIFILES) {
					fprintf(stderr,
					        _("Too many files to combine\n"));
					exit(EX_DATAERR);
				}
				ifiles[ifilct++] = p;
			}
			/* recursive call */
			loadnewfonts(fd, ifiles, ifilct, def_height, font_height, no_screen_map, no_unicode_map);
			return;
		}
	}

	/* file with three code pages? */
	if (inputlen == 9780) {
		offset   = position_codepage(def_height);
		height   = def_height;
		fontsize = 256;
		width    = 8;
	} else if (inputlen == 32768) {
		/* restorefont -w writes a SVGA font to file
		   restorefont -r restores it
		   These fonts have size 32768, for two 512-char fonts.
		   In fact, when BROKEN_GRAPHICS_PROGRAMS is defined,
		   and it always is, there is no default font that is saved,
		   so probably the second half is always garbage. */
		fprintf(stderr, _("Hmm - a font from restorefont? "
		                  "Using the first half.\n"));
		inputlen = 16384; /* ignore rest */
		fontsize = 512;
		offset   = 0;
		width    = 8;
		height   = 32;
		if (!font_height)
			font_height = 16;
	} else {
		int rem = (inputlen % 256);
		if (rem == 0 || rem == 40) {
			/* 0: bare code page bitmap */
			/* 40: preceded by .cp header */
			/* we might check some header details */
			offset = rem;
		} else {
			fprintf(stderr, _("Bad input file size\n"));
			exit(EX_DATAERR);
		}
		fontsize = 256;
		width    = 8;
		height   = inputlen / 256;
	}
	do_loadfont(fd, inbuf + offset, width, height, font_height, fontsize,
	            fp.pathname);
}

static int
position_codepage(int def_height)
{
	int offset;

	/* code page: first 40 bytes, then 8x16 font,
	   then 6 bytes, then 8x14 font,
	   then 6 bytes, then 8x8 font */

	if (!def_height) {
		fprintf(stderr,
		        _("This file contains 3 fonts: 8x8, 8x14 and 8x16."
		          " Please indicate\n"
		          "using an option -8 or -14 or -16 "
		          "which one you want loaded.\n"));
		exit(EX_USAGE);
	}
	switch (def_height) {
		case 8:
			offset = 7732;
			break;
		case 14:
			offset = 4142;
			break;
		case 16:
			offset = 40;
			break;
		default:
			fprintf(stderr, _("You asked for font size %d, "
			                  "but only 8, 14, 16 are possible here.\n"),
			        def_height);
			exit(EX_USAGE);
	}
	return offset;
}

static void
do_saveoldfont(int fd, char *outfile_font, FILE *fpo, int unimap_follows,
               int *count, int *utf8)
{

/* this is the max font size the kernel is willing to handle */
#define MAXFONTSIZE 65536
	unsigned char buf[MAXFONTSIZE];

	int i, ct, width, height, bytewidth, charsize, kcharsize;

	ct = sizeof(buf) / (MAX_FONT_HEIGHT * MAX_FONT_WIDTH / 8); /* max size 32x32, 8 bits/byte */
	if (getfont(fd, buf, &ct, &width, &height))
		exit(EX_OSERR);

	/* save as efficiently as possible */
	bytewidth = get_font_bytewidth(width);
	height    = font_charheight(buf, ct, width);
	charsize  = height * bytewidth;
	kcharsize = MAX_FONT_HEIGHT * bytewidth;

/* Do we need a psf header? */
/* Yes if ct==512 - otherwise we cannot distinguish
	   a 512-char 8x8 and a 256-char 8x16 font. */
#define ALWAYS_PSF_HEADER 1

	if (ct != 256 || width != 8 || unimap_follows || ALWAYS_PSF_HEADER) {
		int psftype = 1;
		int flags   = 0;

		if (unimap_follows)
			flags |= WPSFH_HASTAB;
		writepsffontheader(fpo, width, height, ct, &psftype, flags);
		if (utf8)
			*utf8 = (psftype == 2);
	}

	if (height == 0) {
		fprintf(stderr, _("Found nothing to save\n"));
	} else {
		for (i = 0; i < ct; i++) {
			if (fwrite(buf + (i * kcharsize), charsize, 1, fpo) != 1) {
				fprintf(stderr, _("Cannot write font file"));
				exit(EX_IOERR);
			}
		}
		if (verbose) {
			printf(_("Saved %d-char %dx%d font file on %s\n"),
			       ct, width, height, outfile_font);
		}
	}

	if (count)
		*count = ct;
}

static void
saveoldfont(int fd, char *outfile_font)
{
	FILE *fpo;

	if ((fpo = fopen(outfile_font, "w")) == NULL) {
		perror(outfile_font);
		exit(EX_CANTCREAT);
	}
	do_saveoldfont(fd, outfile_font, fpo, 0, NULL, NULL);
	fclose(fpo);
}

static void
saveoldfontplusunicodemap(int fd, char *outfile_font_and_unicode_map)
{
	FILE *fpo;
	int ct;
	int utf8 = 0;

	if ((fpo = fopen(outfile_font_and_unicode_map, "w")) == NULL) {
		perror(outfile_font_and_unicode_map);
		exit(EX_CANTCREAT);
	}
	ct = 0;
	do_saveoldfont(fd, outfile_font_and_unicode_map, fpo, 1, &ct, &utf8);
	appendunicodemap(fd, fpo, ct, utf8);
	fclose(fpo);
}

/* Only on the current console? On all allocated consoles? */
/* A newly allocated console has NORM_MAP by default -
   probably it should copy the default from the current console?
   But what if we want a new one because the current one is messed up? */
/* For the moment: only the current console, only the G0 set */

static void
send_escseq(int fd, char *seq, int n)
{
	if (write(fd, seq, n) != n) /* maybe fd is read-only */
		printf("%s", seq);
}

void activatemap(int fd)
{
	send_escseq(fd, "\033(K", 3);
}

void disactivatemap(int fd)
{
	send_escseq(fd, "\033(B", 3);
}
