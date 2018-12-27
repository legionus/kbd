#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sysexits.h>
#include <errno.h>

#include "kfont.h"
#include "contextP.h"

/* this is the max font size the kernel is willing to handle */
#define MAXFONTSIZE 65536

/* Do we need a psf header? */
/* Yes if ct==512 - otherwise we cannot distinguish
	   a 512-char 8x8 and a 256-char 8x16 font. */
#define ALWAYS_PSF_HEADER 1

static int
do_dumpfont(struct kfont_ctx *ctx, char *ofil, FILE *fpo, int unimap_follows, size_t *count, int *utf8)
{
	unsigned char buf[MAXFONTSIZE];
	unsigned int i;
	size_t ct, width, height, bytewidth, charsize, kcharsize;

	ct = sizeof(buf) / (32 * 32 / 8); /* max size 32x32, 8 bits/byte */

	if (kfont_get_font(ctx, buf, &ct, &width, &height))
		return -EX_OSERR;

	/* save as efficiently as possible */
	bytewidth = (width + 7) / 8;
	height    = kfont_get_charheight(buf, ct, width);
	charsize  = height * bytewidth;
	kcharsize = 32 * bytewidth;

	if (ct != 256 || width != 8 || unimap_follows || ALWAYS_PSF_HEADER) {
		int psftype = 1;
		int flags   = 0;

		if (unimap_follows)
			flags |= WPSFH_HASTAB;

		if (kfont_write_psffontheader(ctx, fpo, width, height, ct, &psftype, flags) < 0)
			return -EX_IOERR;

		if (utf8)
			*utf8 = (psftype == 2);
	}

	if (height == 0) {
		INFO(ctx, _("Found nothing to save"));
	} else {
		for (i = 0; i < ct; i++) {
			if (fwrite(buf + (i * kcharsize), charsize, 1, fpo) != 1) {
				ERR(ctx, _("Cannot write font file"));
				return -EX_IOERR;
			}
		}

		if (ctx->verbose)
			INFO(ctx, _("Saved %ld-char %ldx%ld font file on %s"), ct, width, height, ofil);
	}

	if (count)
		*count = ct;

	return 0;
}

int
kfont_dump_font(struct kfont_ctx *ctx, char *ofil)
{
	int ret;
	FILE *fpo;
	char errbuf[STACKBUF_LEN];

	if ((fpo = fopen(ofil, "w")) == NULL) {
		strerror_r(errno, errbuf, sizeof(errbuf));
		ERR(ctx, "unable to open file: %s: %s", ofil, errbuf);
		return -EX_CANTCREAT;
	}

	ret = do_dumpfont(ctx, ofil, fpo, 0, NULL, NULL);

	fclose(fpo);
	return ret;
}

int
kfont_dump_fullfont(struct kfont_ctx *ctx, char *Ofil)
{
	FILE *fpo;
	size_t ct;
	int ret = 0, utf8 = 0;
	char errbuf[STACKBUF_LEN];

	if ((fpo = fopen(Ofil, "w")) == NULL) {
		strerror_r(errno, errbuf, sizeof(errbuf));
		ERR(ctx, "unable to open file: %s: %s", Ofil, errbuf);
		return -EX_CANTCREAT;
	}

	ct = 0;
	ret = do_dumpfont(ctx, Ofil, fpo, 1, &ct, &utf8);

	if (ret == 0)
		ret = append_unicodemap(ctx, fpo, ct, utf8);

	fclose(fpo);
	return ret;
}
