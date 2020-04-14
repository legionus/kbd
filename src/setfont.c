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

#include <kbdfile.h>

#include "libcommon.h"

#include "paths.h"
#include "psf.h"
#include "psffontop.h"
#include "kfont.h"

static unsigned int position_codepage(unsigned int iunit);
static int saveoldfont(struct kfont_context *ctx, int fd, const char *ofil);
static int saveoldfontplusunicodemap(struct kfont_context *ctx, int fd, const char *Ofil);
static int loadnewfont(struct kfont_context *ctx,
                        int fd, const char *ifil,
                        unsigned int iunit, unsigned int hwunit, int no_m, int no_u);
static int loadnewfonts(struct kfont_context *ctx,
                         int fd, const char *const *ifiles, int ifilct,
                         unsigned int iunit, unsigned int hwunit, int no_m, int no_u);
static void activatemap(int fd);
static void disactivatemap(int fd);

static inline int
findfont(struct kfont_context *ctx, const char *fnam, struct kbdfile *fp)
{
	return kbdfile_find(fnam, ctx->fontdirpath, ctx->fontsuffixes, fp);
}

static inline int
findpartialfont(struct kfont_context *ctx, const char *fnam, struct kbdfile *fp)
{
	return kbdfile_find(fnam, ctx->partfontdirpath, ctx->partfontsuffixes, fp);
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
	                    "    -d         Double size of font horizontally and vertically.\n"
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
	const char *ifiles[MAXIFILES];
	char *mfil, *ufil, *Ofil, *ofil, *omfil, *oufil, *console;
	int ifilct = 0, fd, i, no_m, no_u;
	unsigned int iunit, hwunit;
	int restore = 0;
	int ret;

	set_progname(argv[0]);
	setuplocale();

	struct kfont_context ctx;
	kfont_init(&ctx);

	ifiles[0] = mfil = ufil = Ofil = ofil = omfil = oufil = NULL;
	iunit = hwunit = 0;
	no_m = no_u = 0;
	console     = NULL;

	/*
	 * No getopt() here because of the -om etc options.
	 */
	for (i = 1; i < argc; i++) {
		if (!strcmp(argv[i], "-V")) {
			print_version_and_exit();
		} else if (!strcmp(argv[i], "-v")) {
			ctx.verbose++;
		} else if (!strcmp(argv[i], "-R")) {
			restore = 1;
		} else if (!strcmp(argv[i], "-C")) {
			if (++i == argc || console)
				usage();
			console = argv[i];
		} else if (!strcmp(argv[i], "-O")) {
			if (++i == argc || Ofil)
				usage();
			Ofil = argv[i];
		} else if (!strcmp(argv[i], "-o")) {
			if (++i == argc || ofil)
				usage();
			ofil = argv[i];
		} else if (!strcmp(argv[i], "-om")) {
			if (++i == argc || omfil)
				usage();
			omfil = argv[i];
		} else if (!strcmp(argv[i], "-ou")) {
			if (++i == argc || oufil)
				usage();
			oufil = argv[i];
		} else if (!strcmp(argv[i], "-m")) {
			if (++i == argc || mfil)
				usage();
			if (!strcmp(argv[i], "none"))
				no_m = 1;
			else
				mfil = argv[i];
		} else if (!strcmp(argv[i], "-u")) {
			if (++i == argc || ufil)
				usage();
			if (!strcmp(argv[i], "none"))
				no_u = 1;
			else
				ufil = argv[i];
		} else if (!strcmp(argv[i], "-f")) {
			kfont_set_option(&ctx, kfont_force);
		} else if (!strncmp(argv[i], "-h", 2)) {
			int tmp = atoi(argv[i] + 2);
			if (tmp <= 0 || tmp > 32)
				usage();
			hwunit = (unsigned int)tmp;
		} else if (!strcmp(argv[i], "-d")) {
			kfont_set_option(&ctx, kfont_double_size);
		} else if (argv[i][0] == '-') {
			int tmp = atoi(argv[i] + 1);
			if (tmp <= 0 || tmp > 32)
				usage();
			iunit = (unsigned int)tmp;
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
		if (ctx.verbose)
			printf("setfont: graphics console %s skipped\n", console ? console : "");
		close(fd);
		return 0;
	}

	if (!ifilct && !mfil && !ufil &&
	    !Ofil && !ofil && !omfil && !oufil && !restore)
		/* reset to some default */
		ifiles[ifilct++] = "";

	if (Ofil && (ret = saveoldfontplusunicodemap(&ctx, fd, Ofil)) < 0)
		exit(-ret);

	if (ofil && (ret = saveoldfont(&ctx, fd, ofil)) < 0)
		exit(-ret);

	if (omfil && (ret = saveoldmap(&ctx, fd, omfil)) < 0)
		exit(-ret);

	if (oufil && (ret = saveunicodemap(&ctx, fd, oufil)) < 0)
		exit(-ret);

	if (mfil) {
		if ((ret = loadnewmap(&ctx, fd, mfil)) < 0)
			exit(-ret);
		activatemap(fd);
		no_m = 1;
	}

	if (ufil)
		no_u = 1;

	if (restore)
		restorefont(&ctx, fd);

	if (ifilct)
		loadnewfonts(&ctx, fd, ifiles, ifilct, iunit, hwunit, no_m, no_u);

	if (ufil && (ret = loadunicodemap(&ctx, fd, ufil)) < 0)
		return -ret;

	return 0;
}

/*
 * 0 - do not test, 1 - test and warn, 2 - test and wipe, 3 - refuse
 */
static int erase_mode = 1;

static int
do_loadfont(struct kfont_context *ctx, int fd, const unsigned char *inbuf,
		unsigned int width, unsigned int height, unsigned int hwunit,
		unsigned int fontsize, const char *filename)
{
	unsigned char *buf = NULL;
	unsigned int i, buflen, kcharsize;
	int bad_video_erase_char = 0;
	int ret;

	if (height < 1 || height > 32) {
		KFONT_ERR(ctx, _("Bad character height %d"), height);
		return -EX_DATAERR;
	}

	if (width < 1 || width > 32) {
		KFONT_ERR(ctx, _("Bad character width %d"), width);
		return -EX_DATAERR;
	}

	if (!hwunit)
		hwunit = height;

	if ((ctx->options & (1 << kfont_double_size)) &&
	    (height > 16 || width > 16)) {
		KFONT_ERR(ctx, _("Cannot double %dx%d font (limit is 16x16)"), width, height);
		kfont_unset_option(ctx, kfont_double_size);
	}

	if (ctx->options & (1 << kfont_double_size)) {
		unsigned int bytewidth  = (width + 7) / 8;
		unsigned int kbytewidth = (2 * width + 7) / 8;
		unsigned int charsize   = height * bytewidth;

		kcharsize = 32 * kbytewidth;
		buflen    = kcharsize * ((fontsize < 128) ? 128 : fontsize);

		buf = calloc(1, buflen);
		if (!buf) {
			KFONT_ERR(ctx, "calloc: %m");
			return -EX_OSERR;
		}

		const unsigned char *src = inbuf;
		for (i = 0; i < fontsize; i++) {
			for (unsigned int y = 0; y < height; y++) {
				for (unsigned int x = 0; x < kbytewidth; x++) {
					unsigned char b = src[i * charsize + y * bytewidth + x / 2];
					if (!(x & 1))
						b >>= 4;

					unsigned char b2 = 0;
					for (int j = 0; j < 4; j++)
						if (b & (1 << j))
							b2 |= 3 << (j * 2);
					buf[i * kcharsize + (y * 2) * kbytewidth + x]       = b2;
					buf[i * kcharsize + ((y * 2) + 1) * kbytewidth + x] = b2;
				}
			}
		}

		width *= 2;
		height *= 2;
		hwunit *= 2;
	} else {
		unsigned int bytewidth = (width + 7) / 8;
		unsigned int charsize  = height * bytewidth;

		kcharsize = 32 * bytewidth;
		buflen    = kcharsize * ((fontsize < 128) ? 128 : fontsize);

		buf = calloc(1, buflen);
		if (!buf) {
			KFONT_ERR(ctx, "calloc: %m");
			return -EX_OSERR;
		}

		for (i = 0; i < fontsize; i++)
			memcpy(buf + (i * kcharsize), inbuf + (i * charsize), charsize);
	}

	/*
	 * Due to a kernel bug, font position 32 is used
	 * to erase the screen, regardless of maps loaded.
	 * So, usually this font position should be blank.
	 */
	if (erase_mode) {
		for (i = 0; i < kcharsize; i++) {
			if (buf[32 * kcharsize + i])
				bad_video_erase_char = 1;
		}

		if (bad_video_erase_char) {
			KFONT_ERR(ctx, _("font position 32 is nonblank"));

			switch (erase_mode) {
				case 3:
					ret = -EX_DATAERR;
					goto err;
				case 2:
					for (i = 0; i < kcharsize; i++)
						buf[32 * kcharsize + i] = 0;
					KFONT_ERR(ctx, _("wiped it"));
					break;
				case 1:
					KFONT_ERR(ctx, _("background will look funny"));
					break;
			}
			sleep(2);
		}
	}

	if (ctx->verbose) {
		if (height == hwunit && filename)
			KFONT_INFO(ctx, _("Loading %d-char %dx%d font from file %s"),
			       fontsize, width, height, filename);
		else if (height == hwunit)
			KFONT_INFO(ctx, _("Loading %d-char %dx%d font"),
			       fontsize, width, height);
		else if (filename)
			KFONT_INFO(ctx, _("Loading %d-char %dx%d (%d) font from file %s"),
			       fontsize, width, height, hwunit, filename);
		else
			KFONT_INFO(ctx, _("Loading %d-char %dx%d (%d) font"),
			       fontsize, width, height, hwunit);
	}

	if (putfont(ctx, fd, buf, fontsize, width, hwunit) < 0) {
		ret = -EX_OSERR;
		goto err;
	}

	ret = 0;
err:
	free(buf);
	return ret;
}

static int
do_loadtable(struct kfont_context *ctx, int fd, struct unicode_list *uclistheads,
		unsigned int fontsize)
{
	struct unimapdesc ud;
	struct unipair *up = NULL;
	unsigned int i, ct = 0, maxct;
	struct unicode_list *ul;
	struct unicode_seq *us;
	int ret;

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

	up = malloc(maxct * sizeof(*up));
	if (!up) {
		KFONT_ERR(ctx, "malloc: %m");
		return -EX_OSERR;
	}

	for (i = 0; i < fontsize; i++) {
		ul = uclistheads[i].next;
		if (ctx->verbose > 1)
			printf("char %03x:", i);
		while (ul) {
			us = ul->seq;
			if (us && !us->next) {
				up[ct].unicode = us->uc;
				up[ct].fontpos = i;
				ct++;
				if (ctx->verbose > 1)
					printf(" %04x", us->uc);
			} else if (ctx->verbose > 1) {
				printf(" seq: <");
				while (us) {
					printf(" %04x", us->uc);
					us = us->next;
				}
				printf(" >");
			}
			ul = ul->next;
			if (ctx->verbose > 1)
				printf(",");
		}
		if (ctx->verbose > 1)
			printf("\n");
	}

	if (ct != maxct) {
		KFONT_ERR(ctx, _("bug in do_loadtable"));
		ret = -EX_SOFTWARE;
		goto err;
	}

	if (ctx->verbose)
		KFONT_INFO(ctx, _("Loading Unicode mapping table..."));

	ud.entry_ct = ct;
	ud.entries  = up;

	if (loadunimap(ctx, fd, NULL, &ud) < 0) {
		ret = -EX_OSERR;
		goto err;
	}

	ret = 0;
err:
	free(up);
	return ret;
}

static int
loadnewfonts(struct kfont_context *ctx,
		int fd, const char *const *ifiles, int ifilct,
		unsigned int iunit, unsigned int hwunit, int no_m, int no_u)
{
	const char *ifil;
	unsigned char *inbuf, *fontbuf, *bigfontbuf;
	unsigned int inputlth, fontbuflth, fontsize, height, width, bytewidth;
	unsigned int bigfontbuflth, bigfontsize, bigheight, bigwidth;
	unsigned char *ptr;
	struct unicode_list *uclistheads;
	struct kbdfile *fp = NULL;
	int i;
	int ret = 0;

	if (ifilct == 1)
		return loadnewfont(ctx, fd, ifiles[0], iunit, hwunit, no_m, no_u);

	/* several fonts that must be merged */
	/* We just concatenate the bitmaps - only allow psf fonts */
	bigfontbuf    = NULL;
	bigfontbuflth = 0;
	bigfontsize   = 0;
	uclistheads   = NULL;
	bigheight     = 0;
	bigwidth      = 0;

	for (i = 0; i < ifilct; i++) {
		if (!(fp = kbdfile_new(NULL))) {
			KFONT_ERR(ctx, "Unable to create kbdfile instance: %m");
			ret = -EX_OSERR;
			goto end;
		}

		ifil = ifiles[i];

		if (findfont(ctx, ifil, fp) && findpartialfont(ctx,ifil, fp)) {
			KFONT_ERR(ctx, _("Cannot open font file %s"), ifil);
			ret = -EX_NOINPUT;
			goto end;
		}

		inbuf = fontbuf = NULL;
		inputlth = fontbuflth = 0;
		fontsize = 0;

		if (readpsffont(ctx, kbdfile_get_file(fp), &inbuf, &inputlth,
		                &fontbuf, &fontbuflth, &width, &fontsize, bigfontsize,
		                no_u ? NULL : &uclistheads)) {
			KFONT_ERR(ctx, _("When loading several fonts, all must be psf fonts - %s isn't"),
			    kbdfile_get_pathname(fp));
			ret = -EX_DATAERR;
			goto end;
		}

		bytewidth = (width + 7) / 8;
		height    = fontbuflth / (bytewidth * fontsize);

		if (ctx->verbose)
			KFONT_INFO(ctx, _("Read %d-char %dx%d font from file %s"),
			     fontsize, width, height, kbdfile_get_pathname(fp));

		kbdfile_free(fp); // avoid zombies, jw@suse.de (#88501)
		fp = NULL;

		if (bigheight == 0)
			bigheight = height;
		else if (bigheight != height) {
			KFONT_ERR(ctx, _("When loading several fonts, all must have the same height"));
			ret = -EX_DATAERR;
			goto end;
		}

		if (bigwidth == 0)
			bigwidth = width;
		else if (bigwidth != width) {
			KFONT_ERR(ctx, _("When loading several fonts, all must have the same width"));
			ret = -EX_DATAERR;
			goto end;
		}

		bigfontsize += fontsize;
		bigfontbuflth += fontbuflth;

		ptr = realloc(bigfontbuf, bigfontbuflth);
		if (!ptr) {
			KFONT_ERR(ctx, "realloc: %m");
			ret = -EX_OSERR;
			goto end;
		}
		bigfontbuf = ptr;
		ptr = NULL;

		memcpy(bigfontbuf + bigfontbuflth - fontbuflth, fontbuf, fontbuflth);
	}

	ret = do_loadfont(ctx, fd, bigfontbuf, bigwidth, bigheight, hwunit,
		bigfontsize, NULL);

	if (!ret && uclistheads && !no_u)
		ret = do_loadtable(ctx, fd, uclistheads, bigfontsize);

end:
	free(bigfontbuf);
	free(ptr);

	if (fp)
		kbdfile_free(fp);
	return ret;
}

static int
loadnewfont(struct kfont_context *ctx, int fd, const char *ifil,
		unsigned int iunit, unsigned int hwunit, int no_m, int no_u)
{
	struct kbdfile *fp;

	char defname[20];
	unsigned int height, width, bytewidth;
	int def = 0;
	unsigned char *inbuf, *fontbuf;
	unsigned int inputlth, fontbuflth, fontsize, offset;
	struct unicode_list *uclistheads;
	int ret;

	if (!(fp = kbdfile_new(NULL))) {
		KFONT_ERR(ctx, "Unable to create kbdfile instance: %m");
		return -EX_OSERR;
	}

	if (!*ifil) {
		/* try to find some default file */

		def = 1; /* maybe also load default unimap */

		if (iunit > 32)
			iunit = 0;
		if (iunit == 0) {
			if (findfont(ctx, ifil = "default", fp) &&
			    findfont(ctx, ifil = "default8x16", fp) &&
			    findfont(ctx, ifil = "default8x14", fp) &&
			    findfont(ctx, ifil = "default8x8", fp)) {
				KFONT_ERR(ctx, _("Cannot find default font"));
				ret = -EX_NOINPUT;
				goto end;
			}
		} else {
			sprintf(defname, "default8x%u", iunit);
			if (findfont(ctx, ifil = defname, fp) &&
			    findfont(ctx, ifil = "default", fp)) {
				KFONT_ERR(ctx, _("Cannot find %s font"), ifil);
				ret = -EX_NOINPUT;
				goto end;
			}
		}
	} else {
		if (findfont(ctx, ifil, fp)) {
			KFONT_ERR(ctx, _("Cannot open font file %s"), ifil);
			ret = -EX_NOINPUT;
			goto end;
		}
	}

	if (ctx->verbose > 1)
		KFONT_INFO(ctx, _("Reading font file %s"), ifil);

	inbuf = fontbuf = NULL;
	inputlth = fontbuflth = fontsize = 0;
	width = 8;
	uclistheads = NULL;

	if (!readpsffont(ctx, kbdfile_get_file(fp), &inbuf, &inputlth, &fontbuf,
	                 &fontbuflth, &width, &fontsize, 0,
	                 no_u ? NULL : &uclistheads)) {

		/* we've got a psf font */
		bytewidth = (width + 7) / 8;
		height    = fontbuflth / (bytewidth * fontsize);

		ret = do_loadfont(ctx, fd, fontbuf, width, height, hwunit,
			fontsize, kbdfile_get_pathname(fp));
		if (ret < 0)
			goto end;

		if (uclistheads && !no_u) {
			ret = do_loadtable(ctx, fd, uclistheads, fontsize);
			if (ret < 0)
				goto end;
		}

		if (!uclistheads && !no_u && def) {
			if ((ret = loadunicodemap(ctx, fd, "def.uni")) < 0)
				KFONT_ERR(ctx, "Unable to load unicode map");
		}

		goto end;
	}

	/* instructions to combine fonts? */
	{
		const char *combineheader = "# combine partial fonts\n";
		size_t chlth = strlen(combineheader);

		if (inputlth >= chlth && !memcmp(inbuf, combineheader, chlth)) {
			const char *ifiles[MAXIFILES];
			int ifilct = 0;
			char *p;
			char *q = (char *)inbuf + chlth;
			char *end = (char *)inbuf + inputlth;

			while (q < end) {
				p = q;
				while (q < end && *q != '\n')
					q++;
				if (q == end) {
					KFONT_ERR(ctx, _("No final newline in combine file"));
					ret = -EX_DATAERR;
					goto end;
				}
				*q++ = 0;
				if (ifilct == MAXIFILES) {
					KFONT_ERR(ctx, _("Too many files to combine"));
					ret = -EX_DATAERR;
					goto end;
				}
				ifiles[ifilct++] = p;
			}

			/* recursive call */
			ret = loadnewfonts(ctx, fd, ifiles, ifilct, iunit,
				hwunit, no_m, no_u);

			goto end;
		}
	}

	/* file with three code pages? */
	if (inputlth == 9780) {
		offset   = position_codepage(iunit);
		height   = iunit;
		fontsize = 256;
		width    = 8;
	} else if (inputlth == 32768) {
		/* restorefont -w writes a SVGA font to file
		   restorefont -r restores it
		   These fonts have size 32768, for two 512-char fonts.
		   In fact, when BROKEN_GRAPHICS_PROGRAMS is defined,
		   and it always is, there is no default font that is saved,
		   so probably the second half is always garbage. */
		KFONT_INFO(ctx, _("Hmm - a font from restorefont? "
		            "Using the first half."));

		inputlth = 16384; /* ignore rest */
		fontsize = 512;
		offset   = 0;
		width    = 8;
		height   = 32;
		if (!hwunit)
			hwunit = 16;
	} else {
		unsigned int rem = (inputlth % 256);

		if (rem == 0 || rem == 40) {
			/* 0: bare code page bitmap */
			/* 40: preceded by .cp header */
			/* we might check some header details */
			offset = rem;
		} else {
			KFONT_ERR(ctx, _("Bad input file size"));
			ret = -EX_DATAERR;
			goto end;
		}

		fontsize = 256;
		width    = 8;
		height   = inputlth / 256;
	}

	ret = do_loadfont(ctx, fd, inbuf + offset, width, height, hwunit,
		fontsize, kbdfile_get_pathname(fp));
end:
	kbdfile_free(fp);
	return ret;
}

static unsigned int
position_codepage(unsigned int iunit)
{
	unsigned int offset;

	/* code page: first 40 bytes, then 8x16 font,
	   then 6 bytes, then 8x14 font,
	   then 6 bytes, then 8x8 font */

	if (!iunit) {
		fprintf(stderr,
		        _("This file contains 3 fonts: 8x8, 8x14 and 8x16."
		          " Please indicate\n"
		          "using an option -8 or -14 or -16 "
		          "which one you want loaded.\n"));
		exit(EX_USAGE);
	}
	switch (iunit) {
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
			        iunit);
			exit(EX_USAGE);
	}
	return offset;
}

static int
do_saveoldfont(struct kfont_context *ctx,
		int fd, const char *ofil, FILE *fpo, int unimap_follows,
		unsigned int *count, int *utf8)
{
/* this is the max font size the kernel is willing to handle */
	unsigned char buf[MAXFONTSIZE];

	unsigned int i, ct, width, height, bytewidth, charsize, kcharsize;
	int ret;

	ct = sizeof(buf) / (32 * 32 / 8); /* max size 32x32, 8 bits/byte */

	if (getfont(ctx, fd, buf, &ct, &width, &height) < 0)
		return -EX_OSERR;

	/* save as efficiently as possible */
	bytewidth = (width + 7) / 8;
	height    = font_charheight(buf, ct, width);
	charsize  = height * bytewidth;
	kcharsize = 32 * bytewidth;

/* Do we need a psf header? */
/* Yes if ct==512 - otherwise we cannot distinguish
	   a 512-char 8x8 and a 256-char 8x16 font. */
#define ALWAYS_PSF_HEADER 1

	if (ct != 256 || width != 8 || unimap_follows || ALWAYS_PSF_HEADER) {
		int psftype = 1;
		int flags   = 0;

		if (unimap_follows)
			flags |= WPSFH_HASTAB;

		ret = writepsffontheader(ctx, fpo, width, height, ct, &psftype, flags);
		if (ret < 0)
			return ret;

		if (utf8)
			*utf8 = (psftype == 2);
	}

	if (height == 0) {
		KFONT_INFO(ctx, _("Found nothing to save"));
	} else {
		for (i = 0; i < ct; i++) {
			if (fwrite(buf + (i * kcharsize), charsize, 1, fpo) != 1) {
				KFONT_ERR(ctx, _("Cannot write font file: %m"));
				return -EX_IOERR;
			}
		}
		if (ctx->verbose) {
			KFONT_INFO(ctx,
			     _("Saved %d-char %dx%d font file on %s"),
			     ct, width, height, ofil);
		}
	}

	if (count)
		*count = ct;

	return 0;
}

static int
saveoldfont(struct kfont_context *ctx, int fd, const char *ofil)
{
	int ret;
	FILE *fpo = fopen(ofil, "w");

	if (!fpo) {
		KFONT_ERR(ctx, "Unable to open: %s: %m", ofil);
		return -EX_CANTCREAT;
	}

	ret = do_saveoldfont(ctx, fd, ofil, fpo, 0, NULL, NULL);

	fclose(fpo);
	return ret;
}

static int
saveoldfontplusunicodemap(struct kfont_context *ctx, int fd, const char *Ofil)
{
	int ret = 0;
	FILE *fpo = fopen(Ofil, "w");

	if (!fpo) {
		KFONT_ERR(ctx, "unable to open: %s: %m", Ofil);
		return -EX_CANTCREAT;
	}

	int utf8 = 0;
	unsigned int ct = 0;

	if (!(ret = do_saveoldfont(ctx, fd, Ofil, fpo, 1, &ct, &utf8)))
		ret = appendunicodemap(ctx, fd, fpo, ct, utf8);

	fclose(fpo);
	return ret;
}

/* Only on the current console? On all allocated consoles? */
/* A newly allocated console has NORM_MAP by default -
   probably it should copy the default from the current console?
   But what if we want a new one because the current one is messed up? */
/* For the moment: only the current console, only the G0 set */

static void
send_escseq(int fd, const char *seq, unsigned char n)
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
