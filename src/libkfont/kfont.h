#ifndef KFONT_H
#define KFONT_H

enum kfont_error {
	KFONT_ERROR_SUCCESS                  = 0,
	KFONT_ERROR_BAD_MAGIC                = -1,
	KFONT_ERROR_BAD_PSF1_HEADER          = -2,
	KFONT_ERROR_UNSUPPORTED_PSF1_MODE    = -3,
	KFONT_ERROR_BAD_PSF2_HEADER          = -4,
	KFONT_ERROR_UNSUPPORTED_PSF2_VERSION = -5,
	KFONT_ERROR_TRAILING_GARBAGE         = -6,
	KFONT_ERROR_SHORT_UNICODE_TABLE      = -7,
	KFONT_ERROR_FONT_OFFSET_TOO_BIG      = -8,
	KFONT_ERROR_CHAR_SIZE_ZERO           = -9,
	KFONT_ERROR_CHAR_SIZE_TOO_BIG        = -10,
	KFONT_ERROR_FONT_LENGTH_TOO_BIG      = -11,
};

enum kfont_version {
	KFONT_VERSION_PSF1 = 1,
	KFONT_VERSION_PSF2 = 2,
};

struct kfont_string {
	uint8_t *data;
	size_t size;
};

struct kfont_slice {
	uint8_t *ptr;
	uint8_t *end;
};

struct kfont_unicode_pair {
	struct kfont_unicode_pair *next;
	unsigned int font_pos;
	uint32_t seq_length;
	uint32_t seq[1];
};

struct kfont {
	enum kfont_version version;

	uint32_t font_len;
	uint32_t font_offset;
	uint32_t font_width;
	uint32_t char_size;
	uint32_t has_table;

	struct kfont_unicode_pair *unicode_map_head;

	struct kfont_string content;
};

bool kfont_read_file(FILE *f, struct kfont *font);
enum kfont_error kfont_parse_psf_font(struct kfont *font);
void kfont_free(struct kfont *font);

#endif /* KFONT_H */
