#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "version.h"
#include "kfont.h"

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

	if (font.font_len != 0) {
		// TODO(dmage): check content size ; add some wrapper?
		printf("first glyph:");
		for (int i = 0; i < 8*font.char_size; i++) {
			if (i % font.font_width == 0) {
				printf("\n");
			}
			int bit = (font.content.data[font.font_offset + i / 8] >> (i % 8)) & 1;
			printf("%c", bit ? '%' : '-');
		}
		printf("\n");
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
