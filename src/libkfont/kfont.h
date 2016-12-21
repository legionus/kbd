#ifndef KFONT_H
#define KFONT_H

enum kfont_error {
	KFONT_ERROR_SUCCESS                  = 0,
	KFONT_ERROR_BAD_MAGIC                = -1,
	KFONT_ERROR_BAD_PSF1_HEADER          = -2,
	KFONT_ERROR_UNSUPPORTED_PSF1_MODE    = -3,
	KFONT_ERROR_BAD_PSF2_HEADER          = -4,
	KFONT_ERROR_UNSUPPORTED_PSF2_VERSION = -5,
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

struct kfont {
	enum kfont_version version;

	uint32_t font_len;
	uint32_t font_offset;
	uint32_t font_width;
	uint32_t char_size;
	uint32_t has_table;
	uint32_t utf8;

	struct kfont_string content;
};

bool kfont_read_file(FILE *f, struct kfont *font);
enum kfont_error kfont_parse_psf_font(struct kfont *font);
void kfont_free(struct kfont *font);

#endif /* KFONT_H */
