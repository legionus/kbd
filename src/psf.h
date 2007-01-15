#define PSF_MAGIC1	0x36
#define PSF_MAGIC2	0x04

#define PSF_MODE512    0x01
#define PSF_MODEHASTAB 0x02
#define PSF_MAXMODE    0x03
#define PSF_SEPARATOR  0xFFFF

struct psf_header
{
  unsigned char magic1, magic2;	/* Magic number */
  unsigned char mode;		/* PSF font mode */
  unsigned char charsize;	/* Character size */
};

#define PSF_MAGIC_OK(x)	((x).magic1 == PSF_MAGIC1 && (x).magic2 == PSF_MAGIC2)
