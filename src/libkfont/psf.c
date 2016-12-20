#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

// xmalloc.h
extern void *xmalloc(size_t sz);
extern void *xrealloc(void *p, size_t sz);
extern void xfree(void *p);

#ifdef __APPLE__
void *xmalloc(size_t sz)
{
	void *result = malloc(sz);
	if (!result) {
		abort();
	}
	return result;
}
void *xrealloc(void *p, size_t sz)
{
	void *result = realloc(p, sz);
	if (!result) {
		abort();
	}
	return result;
}
void xfree(void *p)
{
	free(p);
}
#endif

// psffontop.h
#define MAXFONTSIZE 65536

// new internal header
#define PSF1_MAGIC 0x0436
#define PSF2_MAGIC 0x864ab572

#define PSF1_MODE512 0x01
#define PSF1_MODE_HAS_TAB 0x02
#define PSF1_MODE_HAS_SEQ 0x04
#define PSF1_MAXMODE 0x05 // TODO(dmage): why not 0x07?

/* bits used in flags */
#define PSF2_HAS_UNICODE_TABLE 0x01

/* max version recognized so far */
#define PSF2_MAXVERSION 0

enum kfont_error {
	KFONT_ERROR_SUCCESS                  = 0,
	KFONT_ERROR_BAD_MAGIC                = -1,
	KFONT_ERROR_BAD_PSF1_HEADER          = -2,
	KFONT_ERROR_UNSUPPORTED_PSF1_MODE    = -3,
	KFONT_ERROR_BAD_PSF2_HEADER          = -4,
	KFONT_ERROR_UNSUPPORTED_PSF2_VERSION = -5,
};

struct kfont_string {
	uint8_t *data;
	size_t size;
};

struct kfont_slice {
	uint8_t *ptr;
	uint8_t *end;
};

struct kfont_psf1_header {
	uint8_t mode;      /* PSF font mode */
	uint8_t char_size; /* character size */
};

struct kfont_psf2_header {
	uint32_t version;
	uint32_t header_size; /* offset of bitmaps in file */
	uint32_t flags;
	uint32_t length;        /* number of glyphs */
	uint32_t char_size;     /* number of bytes for each character */
	uint32_t height, width; /* max dimensions of glyphs */
	                        /* charsize = height * ((width + 7) / 8) */
};

// struct kfont_psf_data {
// 	char **fontbufp
// 	int *fontszp
// 	int *fontwidthp
// 	int *fontlenp
// 	int fontpos0
// 	struct unicode_list **uclistheadsp
// };

static inline uint16_t peek_uint16(uint8_t *data)
{
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	// TODO(dmage): is unaligned access allowed?
	return *(uint16_t *)data;
#else
#error TODO
#endif
}

static inline uint32_t peek_uint32(uint8_t *data)
{
#if defined(__BYTE_ORDER__) && __BYTE_ORDER__ == __ORDER_LITTLE_ENDIAN__
	// TODO(dmage): is unaligned access allowed?
	return *(uint32_t *)data;
#else
#error TODO
#endif
}

static inline bool read_uint32(struct kfont_slice *slice, uint32_t *out)
{
	if (slice->ptr + sizeof(uint32_t) > slice->end) {
		return false;
	}

	*out = peek_uint32(slice->ptr);
	slice->ptr += sizeof(uint32_t);
	return true;
}

bool kfont_read_psf1_header(struct kfont_slice *slice, struct kfont_psf1_header *header)
{
	if (slice->ptr + 2 > slice->end) {
		return false;
	}

	header->mode      = slice->ptr[0];
	header->char_size = slice->ptr[1];
	slice->ptr += 2;
	return true;
}

bool kfont_read_psf2_header(struct kfont_slice *slice, struct kfont_psf2_header *header)
{
	return read_uint32(slice, &header->version) &&
	       read_uint32(slice, &header->header_size) &&
	       read_uint32(slice, &header->flags) &&
	       read_uint32(slice, &header->length) &&
	       read_uint32(slice, &header->char_size) &&
	       read_uint32(slice, &header->height) &&
	       read_uint32(slice, &header->width);
}

bool kfont_read_file(FILE *f, struct kfont_string *result)
{
	size_t buflen = MAXFONTSIZE / 4; /* actually an arbitrary value */
	size_t n      = 0;
	uint8_t *buf  = xmalloc(buflen);

	while (1) {
		if (n == buflen) {
			buflen *= 2;
			buf = xrealloc(buf, buflen);
		}
		n += fread(buf + n, 1, buflen - n, f);
		if (ferror(f)) {
			return false;
		}
		if (feof(f)) {
			break;
		}
	}

	result->data = buf;
	result->size = n;

	return true;
}

void kfont_free_string(struct kfont_string *string)
{
	xfree(string->data);
	string->data = 0;
}

enum kfont_error kfont_parse_psf_font(struct kfont_string content)
// char **fontbufp, int *fontszp,
// int *fontwidthp, int *fontlenp, int fontpos0,
// struct unicode_list **uclistheadsp)
{
	struct kfont_slice p;
	p.ptr = content.data;
	p.end = content.data + content.size;

	// FIXME(dmage): is it a result?
	uint32_t font_len, char_size, has_table, ft_offset, font_width, utf8;

	if (content.size >= 4 && peek_uint32(content.data) == PSF2_MAGIC) {
		p.ptr += 4;

		struct kfont_psf2_header psf2_header;
		if (!kfont_read_psf2_header(&p, &psf2_header)) {
			return KFONT_ERROR_BAD_PSF2_HEADER;
		}

		// FIXME(dmage): remove these printfs
		printf("PSF2\n");
		printf("version     : %lu\n", (unsigned long)psf2_header.version);
		printf("header size : %lu\n", (unsigned long)psf2_header.header_size);
		printf("flags       : %lu\n", (unsigned long)psf2_header.flags);
		printf("length      : %lu\n", (unsigned long)psf2_header.length);
		printf("char size   : %lu\n", (unsigned long)psf2_header.char_size);
		printf("height      : %lu\n", (unsigned long)psf2_header.height);
		printf("width       : %lu\n", (unsigned long)psf2_header.width);

		if (psf2_header.version > PSF2_MAXVERSION) {
			return KFONT_ERROR_UNSUPPORTED_PSF2_VERSION;
		}

		font_len   = psf2_header.length;
		char_size  = psf2_header.char_size;
		has_table  = (psf2_header.flags & PSF2_HAS_UNICODE_TABLE);
		ft_offset  = psf2_header.header_size;
		font_width = psf2_header.width;
		utf8       = 1;
	} else if (content.size >= 2 && peek_uint16(content.data) == PSF1_MAGIC) {
		p.ptr += 2;

		struct kfont_psf1_header psf1_header;
		if (!kfont_read_psf1_header(&p, &psf1_header)) {
			return KFONT_ERROR_BAD_PSF1_HEADER;
		}

		// FIXME(dmage): remove these printfs
		printf("PSF1\n");
		printf("mode      : %u\n", (unsigned int)psf1_header.mode);
		printf("char size : %u\n", (unsigned int)psf1_header.char_size);

		if (psf1_header.mode > PSF1_MAXMODE) {
			return KFONT_ERROR_UNSUPPORTED_PSF1_MODE;
		}

		font_len   = 0; // ((psfhdr->mode & PSF1_MODE512) ? 512 : 256);
		char_size  = 0; // psfhdr->charsize;
		has_table  = 0; // (psfhdr->mode & (PSF1_MODEHASTAB | PSF1_MODEHASSEQ));
		ft_offset  = 0; // sizeof(struct psf1_header);
		font_width = 8;
		utf8       = 0;
	} else {
		return KFONT_ERROR_BAD_MAGIC;
	}

	return KFONT_ERROR_SUCCESS;
}

int main(int argc, char *argv[])
{
	if (argc != 2) {
		fprintf(stderr, "usage: %s FILENAME.psf\n", argv[0]);
		exit(1);
	}

	FILE *f = fopen(argv[1], "rb");
	if (!f) {
		perror("fopen");
		exit(1);
	}

	struct kfont_string content;
	if (!kfont_read_file(f, &content)) {
		fprintf(stderr, "kfont_read_file failed\n");
		exit(1);
	}

	enum kfont_error error;
	error = kfont_parse_psf_font(content);
	if (error != KFONT_ERROR_SUCCESS) {
		fprintf(stderr, "kfont_parse_psf_font: %d\n", error);
		exit(1);
	}
}
