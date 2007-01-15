/*
 * psfgettable.c
 *
 * Extract a Unicode character table from a PSF font
 *
 * Copyright (C) 1994 H. Peter Anvin
 *
 * This program may be freely copied under the terms of the GNU
 * General Public License (GPL), version 2, or at your option
 * any later version.
 *
 * fix, aeb, 970316
 */

#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <string.h>
#include "psf.h"
#include "nls.h"

typedef unsigned short unicode;
void usage(char *argv0)
{
  fprintf(stderr, _("Usage: \n"
		    "        %s psffont [outfile]\n"), argv0);
  exit(EX_USAGE);
}

int
main(int argc, char *argv[])
{
  FILE *in, *out;
  char *inname;
  struct psf_header psfhdr;
  int fontlen;
  char buffer[65536];		/* Font data, is scratch only */

  setlocale(LC_ALL, "");
  bindtextdomain(PACKAGE, LOCALEDIR);
  textdomain(PACKAGE);

  if ( argc < 2 || argc > 3 )
    usage(argv[0]);

  if ( !strcmp(argv[1],"-") )
    {
      in = stdin;
      inname = "stdin";
    }
  else
    {
      in = fopen(inname = argv[1], "r");
      if ( !in )
        {
          perror(inname);
          exit(EX_NOINPUT);
        }
    }

  if ( argc < 3 || !strcmp(argv[2],"-") )
    out = stdout;
  else
    {
      out = fopen(argv[2], "w");
      if ( !out )
        {
          perror(argv[2]);
          exit(EX_CANTCREAT);
        }
    }

  if ( fread(&psfhdr, sizeof(struct psf_header), 1, in) < 1 )
    {
      fprintf(stderr, _("%s: Cannot read psf header\n"), inname);
      exit(EX_DATAERR);
    }
  
  if (! PSF_MAGIC_OK(psfhdr) )
    {
      fprintf(stderr, _("%s: Bad magic number\n"), inname);
      exit(EX_DATAERR);
    }
  
  if ( psfhdr.mode > PSF_MAXMODE )
    {
      fprintf(stderr, _("%s: Unknown mode number (%d)\n"),
	      inname, psfhdr.mode);
      exit(EX_DATAERR);
    }
  
  fontlen = ( psfhdr.mode & PSF_MODE512 ) ? 512 : 256;

  if ( ! (psfhdr.mode & PSF_MODEHASTAB ) )
    {
      fprintf(stderr, _("%s: Font already had no character table\n"), inname);
    }

  psfhdr.mode &= ~PSF_MODEHASTAB; /* Clear the bit */
  
  /* Read font data */
  if ( fread(buffer, psfhdr.charsize, fontlen, in) < fontlen )
    {
      perror(inname);
      exit(EX_DATAERR);
    }

  fclose(in);

  /* Write new font file */
  fwrite(&psfhdr, sizeof(struct psf_header), 1, out);
  fwrite(buffer, psfhdr.charsize, fontlen, out);
  fclose(out);

  exit(EX_OK);
}
