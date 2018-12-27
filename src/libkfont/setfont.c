#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>

#include "kfont.h"
#include "contextP.h"

#define MAXIFILES 256

/*
 * 0 - do not test, 1 - test and warn, 2 - test and wipe, 3 - refuse
 */
static int erase_mode = 1;

static int
_kfont_loadfont(struct kfont_ctx *ctx, char *inbuf, size_t width, size_t height, size_t hwunit,
               size_t fontsize, char *filename)
{
	unsigned char *buf;
	unsigned int i;
	size_t buflen;
	size_t bytewidth = (width + 7) / 8;
	size_t charsize = height * bytewidth;
	size_t kcharsize = 32 * bytewidth;
	int bad_video_erase_char = 0;

	if (height < 1 || height > 32) {
		ERR(ctx, _("Bad character height %ld"), height);
		return -EX_DATAERR;
	}
	if (width < 1 || width > 32) {
		ERR(ctx, _("Bad character width %ld"), width);
		return -EX_DATAERR;
	}

	if (!hwunit)
		hwunit = height;

	buflen = (size_t)(kcharsize * ((fontsize < 128) ? 128 : fontsize));

	if ((buf = malloc(buflen)) == NULL) {
		ERR(ctx, "out of memory");
		return -1;
	}

	memset(buf, 0, buflen);

	for (i = 0; i < fontsize; i++)
		memcpy(buf + (i * kcharsize), inbuf + (i * charsize), (size_t) charsize);

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
			ERR(ctx, _("font position 32 is nonblank"));

			switch (erase_mode) {
				case 3:
					return -EX_DATAERR;
				case 2:
					for (i = 0; i < kcharsize; i++)
						buf[32 * kcharsize + i] = 0;
					ERR(ctx, _("wiped it"));
					break;
				case 1:
					ERR(ctx, _("background will look funny"));
			}

			sleep(2);
		}
	}

	if (ctx->verbose) {
		if (height == hwunit && filename)
			INFO(ctx, _("Loading %ld-char %ldx%ld font from file %s"), fontsize, width, height, filename);
		else if (height == hwunit)
			INFO(ctx, _("Loading %ld-char %ldx%ld font"), fontsize, width, height);
		else if (filename)
			INFO(ctx, _("Loading %ld-char %ldx%ld (%ld) font from file %s"), fontsize, width, height, hwunit, filename);
		else
			INFO(ctx, _("Loading %ld-char %ldx%ld (%ld) font"), fontsize, width, height, hwunit);
	}

	if (kfont_load_font(ctx, buf, fontsize, width, hwunit))
		return -EX_OSERR;

	return 0;
}

static int
do_loadtable(struct kfont_ctx *ctx, struct unicode_list *uclistheads, size_t fontsize)
{
	struct unimapdesc ud;
	struct unipair *up;
	unsigned int i, ct = 0, maxct;
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

	if ((up = malloc(maxct * sizeof(struct unipair))) == NULL) {
		ERR(ctx, "out of memory");
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
		ERR(ctx, _("bug in do_loadtable"));
		return -EX_SOFTWARE;
	}

	if (ctx->verbose)
		INFO(ctx, _("Loading Unicode mapping table..."));

	ud.entry_ct = ct;
	ud.entries  = up;

	if (kfont_load_unimap(ctx, NULL, &ud))
		return -EX_OSERR;

	return 0;
}

static ssize_t
position_codepage(struct kfont_ctx *ctx, size_t iunit)
{
	int offset;

	/*
	 * code page: first 40 bytes, then 8x16 font,
	 * then 6 bytes, then 8x14 font,
	 * then 6 bytes, then 8x8 font
	 */

	if (!iunit) {
		ERR(ctx, _("This file contains 3 fonts: 8x8, 8x14 and 8x16. "
		           "Please indicate using an option -8 or -14 or -16 "
		           "which one you want loaded."));
		return -EX_USAGE;
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
			ERR(ctx, _("You asked for font size %ld, "
			           "but only 8, 14, 16 are possible here."),
			        iunit);
			return -EX_USAGE;
	}

	return offset;
}

static int
loadnewfont(struct kfont_ctx *ctx, char *ifil, size_t iunit, size_t hwunit)
{
	struct kbdfile *fp;
	struct kbdfile_ctx *kbdfile_ctx;

	char defname[20];
	size_t height, width, bytewidth;
	int rc, def = 0;
	char *inbuf, *fontbuf;
	size_t inputlth, fontbuflth, fontsize, offset;
	struct unicode_list *uclistheads, **uclist;

	if ((kbdfile_ctx = kbdfile_context_new()) == NULL) {
		ERR(ctx, "out of memory");
		return -EX_OSERR;
	}

	if ((fp = kbdfile_new(kbdfile_ctx)) == NULL) {
		ERR(ctx, "out of memory");
		return -EX_OSERR;
	}

	if (!*ifil) {
		/* try to find some default file */

		def = 1; /* maybe also load default unimap */

		if (iunit > 32)
			iunit = 0;

		if (iunit == 0) {
			if (kbdfile_find(ifil = (char *) "default",     ctx->fontdirs, ctx->fontsuffixes, fp) &&
			    kbdfile_find(ifil = (char *) "default8x16", ctx->fontdirs, ctx->fontsuffixes, fp) &&
			    kbdfile_find(ifil = (char *) "default8x14", ctx->fontdirs, ctx->fontsuffixes, fp) &&
			    kbdfile_find(ifil = (char *) "default8x8",  ctx->fontdirs, ctx->fontsuffixes, fp)) {
				ERR(ctx, _("Cannot find default font"));
				return -EX_NOINPUT;
			}
		} else {
			sprintf(defname, "default8x%ld", iunit);
			if (kbdfile_find(ifil = defname, ctx->fontdirs, ctx->fontsuffixes, fp) &&
			    kbdfile_find(ifil = (char *) "default", ctx->fontdirs, ctx->fontsuffixes, fp)) {
				ERR(ctx, _("Cannot find %s font"), ifil);
				return -EX_NOINPUT;
			}
		}
	} else {
		if (kbdfile_find(ifil, ctx->fontdirs, ctx->fontsuffixes, fp)) {
			ERR(ctx, _("Cannot open font file %s"), ifil);
			return -EX_NOINPUT;
		}
	}

	if (ctx->verbose > 1)
		INFO(ctx, _("Reading font file %s"), ifil);

	inbuf = fontbuf = NULL;
	inputlth = fontbuflth = fontsize = 0;
	width = 8;

	uclist = NULL;
	uclistheads = NULL;

	if (ctx->flags & KFONT_FLAG_SKIP_LOAD_UNICODE_MAP)
		uclist = &uclistheads;

	rc = kfont_read_psffont(ctx, kbdfile_get_file(fp), &inbuf, &inputlth, &fontbuf, &fontbuflth, &width, &fontsize, 0, uclist);

	if (!rc) {
		kbdfile_free(fp);
		kbdfile_context_free(kbdfile_ctx);

		/* we've got a psf font */
		bytewidth = (width + 7) / 8;
		height    = fontbuflth / (bytewidth * fontsize);

		rc = _kfont_loadfont(ctx, fontbuf, width, height, hwunit, fontsize, kbdfile_get_pathname(fp));

		if (rc < 0)
			return rc;

		if (uclistheads && !(ctx->flags & KFONT_FLAG_SKIP_LOAD_UNICODE_MAP)) {
			rc = do_loadtable(ctx, uclistheads, fontsize);
			if (rc < 0)
				return rc;
		}

		if (!uclistheads && !(ctx->flags & KFONT_FLAG_SKIP_LOAD_UNICODE_MAP) && def) {
			rc = kfont_load_unicodemap(ctx, "def.uni");
			if (rc < 0)
				return rc;
		}

		return 0;
	}

	kbdfile_free(fp); // avoid zombies, jw@suse.de (#88501)
	kbdfile_context_free(kbdfile_ctx);

	/* instructions to combine fonts? */
	{
		const char *combineheader = "# combine partial fonts\n";

		size_t chlth = strlen(combineheader);

		char *p, *q;
		if (inputlth >= chlth && !strncmp(inbuf, combineheader, chlth)) {
			char *ifiles[MAXIFILES];
			int ifilct = 0;

			q = inbuf + chlth;

			while (q < inbuf + inputlth) {
				p = q;
				while (q < inbuf + inputlth && *q != '\n')
					q++;
				if (q == inbuf + inputlth) {
					ERR(ctx, _("No final newline in combine file"));
					return -EX_DATAERR;
				}
				*q++ = 0;
				if (ifilct == MAXIFILES) {
					ERR(ctx, _("Too many files to combine"));
					return -EX_DATAERR;
				}
				ifiles[ifilct++] = p;
			}

			/* recursive call */
			return kfont_load_fonts(ctx, ifiles, ifilct, iunit, hwunit);
		}
	}

	/* file with three code pages? */
	if (inputlth == 9780) {
		ssize_t ret = position_codepage(ctx, iunit);
		if (ret < 0) {
			return (int) ret;
		}

		offset   = (size_t) ret;
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
		ERR(ctx, _("Hmm - a font from restorefont? Using the first half"));
		inputlth = 16384; /* ignore rest */
		fontsize = 512;
		offset   = 0;
		width    = 8;
		height   = 32;
		if (!hwunit)
			hwunit = 16;
	} else {
		size_t rem = (inputlth % 256);
		if (rem == 0 || rem == 40) {
			/* 0: bare code page bitmap */
			/* 40: preceded by .cp header */
			/* we might check some header details */
			offset = rem;
		} else {
			ERR(ctx, _("Bad input file size"));
			return -EX_DATAERR;
		}
		fontsize = 256;
		width    = 8;
		height   = inputlth / 256;
	}

	return _kfont_loadfont(ctx, inbuf + offset, width, height, hwunit, fontsize, kbdfile_get_pathname(fp));
}

int
kfont_load_fonts(struct kfont_ctx *ctx, char **ifiles, int ifilct, size_t iunit, size_t hwunit)
{
	char *ifil, *inbuf, *fontbuf, *bigfontbuf;
	size_t inputlth, fontbuflth, fontsize, height, width, bytewidth;
	size_t bigfontbuflth, bigfontsize, bigheight, bigwidth;
	struct unicode_list *uclistheads, **uclist;
	int i, rc;
	struct kbdfile *fp;
	struct kbdfile_ctx *kbdfile_ctx;


	if (ifilct == 1)
		return loadnewfont(ctx, ifiles[0], iunit, hwunit);

	if ((kbdfile_ctx = kbdfile_context_new()) == NULL) {
		ERR(ctx, "out of memory");
		return -EX_OSERR;
	}

	if ((fp = kbdfile_new(kbdfile_ctx)) == NULL) {
		ERR(ctx, "out of memory");
		return -EX_OSERR;
	}

	/* several fonts that must be merged */
	/* We just concatenate the bitmaps - only allow psf fonts */
	bigfontbuf    = NULL;
	bigfontbuflth = 0;
	bigfontsize   = 0;
	uclist        = NULL;
	uclistheads   = NULL;
	bigheight     = 0;
	bigwidth      = 0;

	for (i = 0; i < ifilct; i++) {
		ifil = ifiles[i];
		if (kbdfile_find(ifil, ctx->fontdirs, ctx->fontsuffixes, fp) &&
		    kbdfile_find(ifil, ctx->partfontdirs, ctx->partfontsuffixes, fp)) {
			ERR(ctx, _("Cannot open font file %s"), ifil);
			return -EX_NOINPUT;
		}

		inbuf = fontbuf = NULL;
		inputlth = fontbuflth = 0;
		fontsize              = 0;

		if (ctx->flags & KFONT_FLAG_SKIP_LOAD_UNICODE_MAP)
			uclist = &uclistheads;

		rc = kfont_read_psffont(ctx, kbdfile_get_file(fp), &inbuf, &inputlth, &fontbuf, &fontbuflth, &width, &fontsize, bigfontsize, uclist);

		if (rc < 0) {
			ERR(ctx, _("When loading several fonts, all must be psf fonts - %s isn't"), kbdfile_get_pathname(fp));
			kbdfile_free(fp);
			kbdfile_context_free(kbdfile_ctx);
			return -EX_DATAERR;
		}

		kbdfile_free(fp); // avoid zombies, jw@suse.de (#88501)
		kbdfile_context_free(kbdfile_ctx);

		bytewidth = (width + 7) / 8;
		height    = fontbuflth / (bytewidth * fontsize);

		if (ctx->verbose)
			INFO(ctx, _("Read %ld-char %ldx%ld font from file %s"), fontsize, width, height, kbdfile_get_pathname(fp));

		if (bigheight == 0)
			bigheight = height;
		else if (bigheight != height) {
			ERR(ctx, _("When loading several fonts, all must have the same height"));
			return -EX_DATAERR;
		}

		if (bigwidth == 0)
			bigwidth = width;
		else if (bigwidth != width) {
			ERR(ctx, _("When loading several fonts, all must have the same width"));
			return -EX_DATAERR;
		}

		bigfontsize += fontsize;
		bigfontbuflth += fontbuflth;

		if ((bigfontbuf = realloc(bigfontbuf, bigfontbuflth)) == NULL) {
			ERR(ctx, "out of memory");
			return -EX_OSERR;
		}

		memcpy(bigfontbuf + bigfontbuflth - fontbuflth, fontbuf, fontbuflth);
	}

	rc = _kfont_loadfont(ctx, bigfontbuf, bigwidth, bigheight, hwunit, bigfontsize, NULL);
	free(bigfontbuf);

	if (rc < 0)
		return rc;

	if (uclistheads && !(ctx->flags & KFONT_FLAG_SKIP_LOAD_UNICODE_MAP))
		return do_loadtable(ctx, uclistheads, bigfontsize);

	return 0;
}
