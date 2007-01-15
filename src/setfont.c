/*
 * setfont.c - Eugene Crosser & Andries Brouwer
 *
 * Version 0.98
 *
 * Loads the console font, and possibly the corresponding screen map(s).
 * We accept two kind of screen maps, one [-m] giving the correspondence
 * between some arbitrary 8-bit character set currently in use and the
 * font positions, and the second [-u] giving the correspondence between
 * font positions and Unicode values.
 */
#define VERSION "0.98"

#include <stdio.h>
#include <memory.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/kd.h>
#include <endian.h>
#include "paths.h"
#include "psf.h"
#include "nls.h"

char *progname;

static int position_codepage(int iunit);
static void saveoldfont(int fd, char *ofil);
static void saveoldfontplusunicodemap(int fd, char *Ofil);
static void loadnewfont(int fd, char *ifil,
			int iunit, int hwunit, int no_m, int no_u);
static void restorefont(int fd);
extern void saveoldmap(int fd, char *omfil);
extern void loadnewmap(int fd, char *mfil);
extern void saveunicodemap(int fd, char *oufil);
extern void loadunicodemap(int fd, char *ufil);
extern void appendunicodemap(int fd, FILE *fp, int ct);
extern void activatemap(void);
extern int getfd(void);
extern int getfont(int fd, char *buf, int *count);
extern int putfont(int fd, char *buf, int count, int height);
extern int font_charheight(char *buf, int count);
extern char *malloc();

int verbose = 0;
int force = 0;

/* search for the font in these directories (with trailing /) */
char *fontdirpath[] = { "", DATADIR "/" FONTDIR "/", 0 };
char *fontsuffixes[] = { "", ".psfu", ".psf", ".cp", ".fnt", 0 };

static inline FILE*
findfont(char *fnam) {
    return findfile(fnam, fontdirpath, fontsuffixes);
}

void
usage(void)
{
        fprintf(stderr, _(
"Usage: setfont [write-options] [-<N>] [newfont] [-m consolemap] [-u unicodemap]\n"
"  write-options (take place before file loading):\n"
"    -o  <filename>	Write current font to <filename>\n"
"    -om <filename>	Write current consolemap to <filename>\n"
"    -ou <filename>	Write current unicodemap to <filename>\n"
"If no newfont and no -[o|om|ou|m|u] option is given, a default font is loaded:\n"
"    setfont             Load font \"default[.gz]\"\n"
"    setfont -<N>        Load font \"default8x<N>[.gz]\"\n"
"The -<N> option selects a font from a codepage that contains three fonts:\n"
"    setfont -{8|14|16} codepage.cp[.gz]   Load 8x<N> font from codepage.cp\n"
"Explicitly (with -m or -u) or implicitly (in the fontfile) given mappings will\n"
"be loaded and, in the case of consolemaps, activated.\n"
"    -h<N>       (no space) Override font height.\n"
"    -m none	Suppress loading and activation of a mapping table.\n"
"    -u none	Suppress loading of a unicode map.\n"
"    -v		Be verbose.\n"
"    -V		Print version and exit.\n"
"Files are loaded from the current directory or /usr/lib/kbd/*/.\n"
));
	exit(1);
}

int
main(int argc, char *argv[])
{
	char *ifil, *mfil, *ufil, *Ofil, *ofil, *omfil, *oufil;
	int fd, i, iunit, hwunit, no_m, no_u;
	int restore = 0;

	progname = argv[0];

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	fd = getfd();

	ifil = mfil = ufil = Ofil = ofil = omfil = oufil = 0;
	iunit = hwunit = 0;
	no_m = no_u = 0;

	for (i = 1; i < argc; i++) {
	    if (!strcmp(argv[i], "-V")) {
	        printf(_("setfont version %s\n"), VERSION);
		exit(0);
	    } else if (!strcmp(argv[i], "-v")) {
	        verbose = 1;
	    } else if (!strcmp(argv[i], "-R")) {
		restore = 1;
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
		force = 1;
	    } else if (!strncmp(argv[i], "-h", 2)) {
		hwunit = atoi(argv[i]+2);
		if (hwunit <= 0 || hwunit > 32)
		  usage();
	    } else if (argv[i][0] == '-') {
		iunit = atoi(argv[i]+1);
		if(iunit <= 0 || iunit > 32)
		  usage();
	    } else {
		if (ifil)
		  usage();
		ifil = argv[i];
	    }
	}

	if (ifil && restore) {
	    fprintf(stderr, _("setfont: cannot both restore from character ROM"
			      " and from file. Font unchanged.\n"));
	    exit(1);
	}

	if (!ifil && !mfil && !ufil &&
	    !Ofil && !ofil && !omfil && !oufil && !restore)
	  /* reset to some default */
	  ifil = "";

	if (Ofil)
	  saveoldfontplusunicodemap(fd, Ofil);

	if (ofil)
	  saveoldfont(fd, ofil);

	if (omfil)
	  saveoldmap(fd, omfil);

	if (oufil)
	  saveunicodemap(fd, oufil);

	if (mfil) {
	    loadnewmap(fd, mfil);
	    activatemap();
	    no_m = 1;
	}

	if (ufil)
	  no_u = 1;

	if (restore)
	  restorefont(fd);

	if (ifil)
	  loadnewfont(fd, ifil, iunit, hwunit, no_m, no_u);

	if (ufil)
	  loadunicodemap(fd, ufil);

	return 0;
}

static void
restorefont(int fd) {
	/* On most kernels this won't work since it is not supported
	   when BROKEN_GRAPHICS_PROGRAMS is defined, and that is defined
	   by default.  Moreover, this is not defined for vgacon. */
	if (ioctl(fd, PIO_FONTRESET, 0)) {
		perror("PIO_FONTRESET");
	}
}

static void
do_loadfont(int fd, char *inbuf, int unit, int hwunit, int fontsize) {
	char buf[16384];
	int i;

	memset(buf,0,sizeof(buf));

	if (unit < 1 || unit > 32) {
	    fprintf(stderr, _("Bad character size %d\n"), unit);
	    exit(1);
	}

	if (!hwunit)
	    hwunit = unit;

	for (i = 0; i < fontsize; i++)
	    memcpy(buf+(32*i), inbuf+(unit*i), unit);

	if (verbose)
	  printf(_("Loading %d-char 8x%d font from file %s\n"),
		 fontsize, unit, pathname);

	if (putfont(fd, buf, fontsize, hwunit))
	  exit(1);
}

static void
do_loadtable(int fd, unsigned char *inbuf, int tailsz, int fontsize) {
	struct unimapinit advice;
	struct unimapdesc ud;
	struct unipair *up;
	int ct = 0, maxct;
	int glyph;
	u_short unicode;

	maxct = tailsz;		/* more than enough */
	up = (struct unipair *) malloc(maxct * sizeof(struct unipair));
	if (!up) {
	    fprintf(stderr, _("Out of memory?\n"));
	    exit(1);
	}
	for (glyph = 0; glyph < fontsize; glyph++) {
	    while (tailsz >= 2) {
		unicode = (((u_short) inbuf[1]) << 8) + inbuf[0];
		tailsz -= 2;
		inbuf += 2;
		if (unicode == PSF_SEPARATOR)
		    break;
		up[ct].unicode = unicode;
		up[ct].fontpos = glyph;
		ct++;
	    }
	}

	/* Note: after PIO_UNIMAPCLR and before PIO_UNIMAP
	   this printf did not work on many kernels */
	if (verbose)
	  printf(_("Loading Unicode mapping table...\n"));

	advice.advised_hashsize = 0;
	advice.advised_hashstep = 0;
	advice.advised_hashlevel = 0;
	if(ioctl(fd, PIO_UNIMAPCLR, &advice)) {
#ifdef ENOIOCTLCMD
	    if (errno == ENOIOCTLCMD) {
		fprintf(stderr,
			_("It seems this kernel is older than 1.1.92\n"
			  "No Unicode mapping table loaded.\n"));
	    } else
#endif
	      perror("PIO_UNIMAPCLR");
	    exit(1);
	}
	ud.entry_ct = ct;
	ud.entries = up;
	if(ioctl(fd, PIO_UNIMAP, &ud)) {
#if 0
	    if (errno == ENOMEM) {
		/* change advice parameters */
	    }
#endif
	    perror("PIO_UNIMAP");
	    exit(1);
	}
}

static void
loadnewfont(int fd, char *ifil, int iunit, int hwunit, int no_m, int no_u) {
	FILE *fpi;
	char defname[20];
	int unit;
	char inbuf[32768];	/* primitive */
	int inputlth, offset;

	if (!*ifil) {
	    /* try to find some default file */

	    if (iunit < 0 || iunit > 32)
	      iunit = 0;
	    if (iunit == 0) {
		if ((fpi = findfont(ifil = "default")) == NULL &&
		    (fpi = findfont(ifil = "default8x16")) == NULL &&
		    (fpi = findfont(ifil = "default8x14")) == NULL &&
		    (fpi = findfont(ifil = "default8x8")) == NULL) {
		    fprintf(stderr, _("Cannot find default font\n"));
		    exit(1);
		}
	    } else {
		sprintf(defname, "default8x%d", iunit);
		if ((fpi = findfont(ifil = defname)) == NULL &&
		    (fpi = findfont(ifil = "default")) == NULL) {
		    fprintf(stderr, _("Cannot find %s font\n"), ifil);
		    exit(1);
		}
	    }
	} else {
	    if ((fpi = findfont(ifil)) == NULL) {
		fprintf(stderr, _("Cannot open font file %s\n"), ifil);
		exit(1);
	    }
	}

	/*
	 * We used to look at the length of the input file
	 * with stat(); now that we accept compressed files,
	 * just read the entire file.
	 */
	inputlth = fread(inbuf, 1, sizeof(inbuf), fpi);
	if (ferror(fpi)) {
		fprintf(stderr, _("Error reading input font"));
		exit(1);
	}
	/* use malloc/realloc in case of giant files;
	   maybe these do not occur: 16kB for the font,
	   and 16kB for the map leaves 32 unicode values
	   for each font position */
	if (!feof(fpi)) {
		fprintf(stderr, _(
"Setfont is so naive as to believe that font files\n"
"have a size of at most 32kB.  Unfortunately it seems\n"
"that you encountered an exception.  If this really is\n"
"a font file, (i) recompile setfont, (ii) tell aeb@cwi.nl .\n"
));
		exit(1);
	}
	fpclose(fpi);

	/* test for psf first */
	{
	    struct psf_header psfhdr;
	    int fontsize;
	    int hastable;
	    int head0, head;

	    if (inputlth < sizeof(struct psf_header))
		goto no_psf;

	    psfhdr = * (struct psf_header *) &inbuf[0];

	    if (!PSF_MAGIC_OK(psfhdr))
		goto no_psf;

	    if (psfhdr.mode > PSF_MAXMODE) {
		fprintf(stderr, _("Unsupported psf file mode\n"));
		exit(1);
	    }
	    fontsize = ((psfhdr.mode & PSF_MODE512) ? 512 : 256);
	    hastable = (psfhdr.mode & PSF_MODEHASTAB);
	    unit = psfhdr.charsize;
	    head0 = sizeof(struct psf_header);
	    head = head0 + fontsize*unit;
	    if (head > inputlth || (!hastable && head != inputlth)) {
		fprintf(stderr, _("Input file: bad length\n"));
		exit(1);
	    }
	    do_loadfont(fd, inbuf + head0, unit, hwunit, fontsize);
	    if (hastable && !no_u)
	      do_loadtable(fd, inbuf + head, inputlth-head, fontsize);
	    return;
	}
      no_psf:

	/* file with three code pages? */
	if (inputlth == 9780) {
	    offset = position_codepage(iunit);
	    unit = iunit;
	} else {
	    /* bare font */
	    if (inputlth & 0377) {
		fprintf(stderr, _("Bad input file size\n"));
		exit(1);
	    }
	    offset = 0;
	    unit = inputlth/256;
	}
	do_loadfont(fd, inbuf+offset, unit, hwunit, 256);
}

static int
position_codepage(int iunit) {
        int offset;

	/* code page: first 40 bytes, then 8x16 font,
	   then 6 bytes, then 8x14 font,
	   then 6 bytes, then 8x8 font */

	if (!iunit) {
	    fprintf(stderr,
		    _("This file contains 3 fonts: 8x8, 8x14 and 8x16."
		      " Please indicate\n"
		      "using an option -8 or -14 or -16 "
		      "which one you want loaded.\n"));
	    exit(1);
	}
	switch (iunit) {
	  case 8:
	    offset = 7732; break;
	  case 14:
	    offset = 4142; break;
	  case 16:
	    offset = 40; break;
	  default:
	    fprintf(stderr, _("You asked for font size %d, "
			      "but only 8, 14, 16 are possible here.\n"),
		    iunit);
	    exit(1);
	}
	return offset;
}

static void
do_saveoldfont(int fd, char *ofil, FILE *fpo, int unimap_follows, int *count) {
    int i, unit, ct;
    char buf[16384];

    ct = sizeof(buf)/32;
    if (getfont(fd, buf, &ct))
	exit(1);

    /* save as efficiently as possible */
    unit = font_charheight(buf, ct);

    /* Do we need a psf header? */
    /* Yes if ct=512 - otherwise we cannot distinguish
       a 512-char 8x8 and a 256-char 8x16 font. */

    if (ct != 256 || unimap_follows) {
        struct psf_header psfhdr;

	psfhdr.magic1 = PSF_MAGIC1;
	psfhdr.magic2 = PSF_MAGIC2;
	psfhdr.mode = 0;
	if (ct != 256)
	    psfhdr.mode |= PSF_MODE512;
	if (unimap_follows)
	    psfhdr.mode |= PSF_MODEHASTAB;
	psfhdr.charsize = unit;
	if (fwrite(&psfhdr, sizeof(struct psf_header), 1, fpo) != 1) {
	    fprintf(stderr, _("Cannot write font file header"));
	    exit(1);
	}
    }

    if (unit == 0)
      fprintf(stderr, _("Found nothing to save\n"));
    else {
      for (i = 0; i < ct; i++)
	if (fwrite(buf+(32*i), unit, 1, fpo) != 1) {
	   fprintf(stderr, _("Cannot write font file"));
	   exit(1);
	}
      if (verbose)
	printf(_("Saved %d-char 8x%d font file on %s\n"), ct, unit, ofil);
    }

    if (count)
      *count = ct;
}

static void
saveoldfont(int fd, char *ofil) {
    FILE *fpo;

    if((fpo = fopen(ofil, "w")) == NULL) {
	perror(ofil);
	exit(1);
    }
    do_saveoldfont(fd, ofil, fpo, 0, NULL);
    fclose(fpo);
}

static void
saveoldfontplusunicodemap(int fd, char *Ofil){
    FILE *fpo;
    int ct;

    if((fpo = fopen(Ofil, "w")) == NULL) {
	perror(Ofil);
	exit(1);
    }
    ct = 0;
    do_saveoldfont(fd, Ofil, fpo, 1, &ct);
    appendunicodemap(fd, fpo, ct);
    fclose(fpo);
}

/* Only on the current console? On all allocated consoles? */
/* A newly allocated console has NORM_MAP by default -
   probably it should copy the default from the current console?
   But what if we want a new one because the current one is messed up? */
/* For the moment: only the current console, only the G0 set */
void
activatemap(void) {
    printf("\033(K");
}

void
disactivatemap(void) {
    printf("\033(B");
}
