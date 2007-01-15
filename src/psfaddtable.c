/*
 * psfaddtable.c
 *
 * Add a Unicode character table to a PSF font
 *
 * Copyright (C) 1994 H. Peter Anvin
 *
 * This program may be freely copied under the terms of the GNU
 * General Public License (GPL), version 2, or at your option
 * any later version.
 *
 * Added input ranges, aeb.
 */

#include <stdio.h>
#include <stdlib.h>
#include <sysexits.h>
#include <string.h>
#include <ctype.h>
#include "psf.h"
#include "nls.h"

typedef unsigned short unicode;

struct unicode_list
{
  unicode uc;			/* Unicode listed */
  struct unicode_list *next;
};

void usage(char *argv0)
{
  fprintf(stderr, _("Usage: \n"
	          "        %s psffont chartable [outfile]\n"), argv0);
  exit(EX_USAGE);
}

int getunicode(char **p0)
{
  char *p = *p0;

  while (*p == ' ' || *p == '\t')
    p++;
  if (*p != 'U' || p[1] != '+' ||
      !isxdigit(p[2]) || !isxdigit(p[3]) || !isxdigit(p[4]) ||
      !isxdigit(p[5]) || isxdigit(p[6]))
    return -1;
  *p0 = p+6;
  return strtol(p+2,0,16);
}

struct unicode_list **hookptr[512];

void addpair(int fp, int un)
{
  struct unicode_list *newmbr;

  if ( un != PSF_SEPARATOR && un <= 0xFFFF )
    {
	/* Add to linked list IN ORDER READ */
		      
	newmbr = malloc(sizeof(struct unicode_list));
	newmbr->uc = (unicode) un;
	newmbr->next = NULL;
	*(hookptr[fp]) = newmbr;
	hookptr[fp] = &(newmbr->next);
    }
  /* otherwise: ignore */
}

int main(int argc, char *argv[])
{
  FILE *in, *ctbl, *out;
  char *inname, *tblname;
  char buffer[65536];
  unsigned char unicodepair[2];
  struct unicode_list *unilist[512];
  struct unicode_list *newmbr, tmpmbr;
  struct psf_header psfhdr;
  int fontlen;
  int i;
  int fp0, fp1, un0, un1;
  char *p, *p1;

  setlocale(LC_ALL, "");
  bindtextdomain(PACKAGE, LOCALEDIR);
  textdomain(PACKAGE);

  if ( argc < 3 || argc > 4 )
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

  if ( !strcmp(argv[2],"-") && in != stdin )
    {
      ctbl = stdin;
      tblname = "stdin";
    }
  else
    {
      ctbl = fopen(tblname = argv[2], "r");
      if ( !ctbl )
	{
	  perror(tblname);
	  exit(EX_NOINPUT);
	}
    }

  if ( argc < 4 || !strcmp(argv[3],"-") )
    out = stdout;
  else
    {
      out = fopen(argv[3], "w");
      if ( !out )
	{
	  perror(argv[3]);
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
      fprintf(stderr, _("%s: Unknown mode number (%d)\n"), inname, psfhdr.mode);
      exit(EX_DATAERR);
    }

  fontlen = ( psfhdr.mode & PSF_MODE512 ) ? 512 : 256;

  /* Copy font data */
  if ( fread(buffer, psfhdr.charsize, fontlen, in) < fontlen )
    {
      perror(inname);
      exit(EX_DATAERR);
    }
  fclose(in);			/* Done with input */

  /* Set has-table bit in mode field, and copy to output */
  
  psfhdr.mode |= PSF_MODEHASTAB;
  fwrite(&psfhdr, sizeof(struct psf_header), 1, out);
  fwrite(buffer, psfhdr.charsize, fontlen, out);

  /* Now we come to the tricky part.  Parse the input table. */

  for ( i = 0 ; i < 512 ; i++ )	/* Initialize unicode list */
    unilist[i] = NULL;

  for ( i = 0 ; i < 512 ; i++ )	/* Initialize hook pointer list */
    hookptr[i] = &(unilist[i]);

  while ( fgets(buffer, sizeof(buffer), ctbl) != NULL )
    {
      if ( (p = strchr(buffer, '\n')) != NULL )
	*p = '\0';
      else
	fprintf(stderr, _("%s: Warning: line too long\n"), tblname);

      p = buffer;

/*
 * Syntax accepted:
 *	<fontpos>	<unicode> <unicode> ...
 *	<range>		idem
 *	<range>		<unicode range>
 *
 * where <range> ::= <fontpos>-<fontpos>
 * and <unicode> ::= U+<h><h><h><h>
 * and <h> ::= <hexadecimal digit>
 */

      while (*p == ' ' || *p == '\t')
	p++;
      if (!*p || *p == '#')
	continue;	/* skip comment or blank line */

      fp0 = strtol(p, &p1, 0);
      if (p1 == p)
	{
	  fprintf(stderr, _("Bad input line: %s\n"), buffer);
	  exit(EX_DATAERR);
        }
      p = p1;

      while (*p == ' ' || *p == '\t')
	p++;
      if (*p == '-')
	{
	  p++;
	  fp1 = strtol(p, &p1, 0);
	  if (p1 == p)
	    {
	      fprintf(stderr, _("Bad input line: %s\n"), buffer);
	      exit(EX_DATAERR);
	    }
	  p = p1;
        }
      else
	fp1 = 0;

      if ( fp0 < 0 || fp0 >= fontlen )
	{
	    fprintf(stderr,
		    _("%s: Glyph number (0x%x) larger than font length\n"),
		    tblname, fp0);
	    exit(EX_DATAERR);
	}
      if ( fp1 && (fp1 < fp0 || fp1 >= fontlen) )
	{
	    fprintf(stderr,
		    _("%s: Bad end of range (0x%x)\n"),
		    tblname, fp1);
	    exit(EX_DATAERR);
	}

      if (fp1)
	{
	  /* we have a range; expect the word "idem" or a Unicode range of the
	     same length */
	  while (*p == ' ' || *p == '\t')
	    p++;
	  if (!strncmp(p, "idem", 4))
	    {
	      for (i=fp0; i<=fp1; i++)
		addpair(i,i);
	      p += 4;
	    }
	  else
	    {
	      un0 = getunicode(&p);
	      while (*p == ' ' || *p == '\t')
		p++;
	      if (*p != '-')
		{
		  fprintf(stderr,
			  _("%s: Corresponding to a range of font positions, "
			    "there should be a Unicode range\n"),
			  tblname);
		  exit(EX_DATAERR);
	        }
	      p++;
	      un1 = getunicode(&p);
	      if (un0 < 0 || un1 < 0)
		{
		  fprintf(stderr,
			  _("%s: Bad Unicode range corresponding to font "
			    "position range 0x%x-0x%x\n"),
			  tblname, fp0, fp1);
		  exit(EX_DATAERR);
	        }
	      if (un1 - un0 != fp1 - fp0)
		{
		  fprintf(stderr,
			  _("%s: Unicode range U+%x-U+%x not of the same "
			    "length as font position range 0x%x-0x%x\n"),
			  tblname, un0, un1, fp0, fp1);
		  exit(EX_DATAERR);
	        }
	      for (i=fp0; i<=fp1; i++)
		addpair(i, un0-fp0+i);
	    }
        }
      else
	{
	    /* no range; expect a list of unicode values for a single font position */

	    while ( (un0 = getunicode(&p)) >= 0 )
	      addpair(fp0, un0);
	}
      while (*p == ' ' || *p == '\t')
	p++;
      if (*p && *p != '#')
	fprintf(stderr, _("%s: trailing junk (%s) ignored\n"), tblname, p);
    }

  /* Okay, we hit EOF, now glyph table should be read */

  fclose(ctbl);

  for ( i = 0 ; i < fontlen ; i++ )
    {
      for ( newmbr = unilist[i] ; newmbr ; newmbr = tmpmbr.next )
	{
	  tmpmbr = *newmbr;
	  unicodepair[0] = (tmpmbr.uc & 0xff);
	  unicodepair[1] = ((tmpmbr.uc >> 8) & 0xff);
	  fwrite(unicodepair, sizeof(unicodepair), 1, out);
	  free(newmbr);
	}

      /* Write string terminator */
      unicodepair[0] = (PSF_SEPARATOR & 0xff);
      unicodepair[1] = ((PSF_SEPARATOR >> 8) & 0xff);
      fwrite(unicodepair, sizeof(unicodepair), 1, out);
    }

  fclose(out);

  exit(EX_OK);
}
