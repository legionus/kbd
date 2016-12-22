#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "version.h"
#include "kfont.h"

static void draw_bit(int bit)
{
	if (bit) {
		printf("\x1b[7m  \x1b[0m");
	} else {
		printf("  ");
	}
}

int main(int argc, char *argv[])
{
	set_progname(argv[0]);

	if (argc != 2) {
		fprintf(stderr, "usage: %s FILENAME.psf\n", progname);
		exit(1);
	}

	FILE *f = fopen(argv[1], "rb");
	if (!f) {
		perror("fopen");
		exit(1);
	}

	struct kfont font;

	if (!kfont_read_file(f, &font)) {
		fprintf(stderr, "kfont_read_file failed\n");
		exit(1);
	}

	enum kfont_error error;
	error = kfont_parse_psf_font(&font);
	if (error != KFONT_ERROR_SUCCESS) {
		fprintf(stderr, "kfont_parse_psf_font: %d\n", error);
		exit(1);
	}

	printf("kfont data:\n");
	printf("version     : %lu\n", (unsigned long)font.version);
	printf("font length : %lu\n", (unsigned long)font.font_len);
	printf("char size   : %lu\n", (unsigned long)font.char_size);
	printf("has table   : %lu\n", (unsigned long)font.has_table);
	printf("font offset : %lu\n", (unsigned long)font.font_offset);
	printf("font width  : %lu\n", (unsigned long)font.font_width);

	if (font.font_width > 0 && font.font_width <= 32) {
		int row_size = (font.font_width + 7) / 8;
		for (unsigned int font_pos = 0; font_pos < font.font_len; font_pos++) {
			printf("position %u:\n", font_pos);
			for (unsigned int row = 0; (row + 1) * row_size <= font.char_size; row++) {
				printf("|");
				for (int col = 0; col < font.font_width; col++) {
					int offset = font.font_offset + font_pos * font.char_size + row * row_size;
					int value = (font.content.data[offset + col / 8] << (col % 8)) & 0x80;
					draw_bit(value);
				}
				printf("|\n");
			}
		}
	}

	struct kfont_unicode_pair *pair = font.unicode_map_head;
	for ( ; pair; pair = pair->next) {
		for (int i = 0; i < pair->seq_length; i++) {
			printf("U+%04X ", pair->seq[i]);
		}
		printf("-> %u\n", pair->font_pos);
	}

	kfont_free(&font);
}
