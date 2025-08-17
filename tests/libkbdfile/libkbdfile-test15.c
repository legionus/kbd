#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <kbdfile.h>
#include "libcommon.h"

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct kbdfile *fp = kbdfile_new(NULL);
	if (!fp)
		kbd_error(EXIT_FAILURE, 0, "unable to create kbdfile");

	const char *const dirpath[]  = { "", TESTDIR "/data/findfile/test_0/keymaps/**", NULL };
	const char *const suffixes[] = { ".map", NULL };

	struct testcase {
		const char *file;
		const char *text;
		int compressed;
	} cases[] = {
#ifdef HAVE_ZLIB
		{ TESTDIR "/data/findfile/test_0/keymaps/i386/qwerty/test3.map.gz",  "qwerty zlib",  1 },
#endif
#ifdef HAVE_BZIP2
		{ TESTDIR "/data/findfile/test_0/keymaps/i386/qwerty/test3.map.bz2", "qwerty bzip2", 1 },
#endif
#ifdef HAVE_LZMA
		{ TESTDIR "/data/findfile/test_0/keymaps/i386/qwerty/test3.map.xz",  "qwerty lzma",  1 },
#endif
#ifdef HAVE_ZSTD
		{ TESTDIR "/data/findfile/test_0/keymaps/i386/qwerty/test3.map.zst", "qwerty zstd",  1 },
#endif
		{ TESTDIR "/data/findfile/test_0/keymaps/i386/qwerty/test3.map",     "qwerty",       0 },
		{ NULL, NULL, 0 }
	};
	struct testcase *ts = &cases[0];

	setenv("KBDFILE_IGNORE_DECOMP_UTILS", "1", 1);

	for (ts = &cases[0]; ts->file; ts++) {
		char buf[256];
		size_t len;
		FILE *f;

		//kbd_warning(0, "Check: %s", ts->file);

		int rc = kbdfile_find(ts->file, dirpath, suffixes, fp);

		if (rc != 0)
			kbd_error(EXIT_FAILURE, 0, "unable to find file: %s", ts->file);

		if (strcmp(ts->file, kbdfile_get_pathname(fp)) != 0)
			kbd_error(EXIT_FAILURE, 0, "unexpected file: %s (expected %s)",
					kbdfile_get_pathname(fp), ts->file);

		if (ts->compressed && !kbdfile_is_compressed(fp))
			kbd_error(EXIT_FAILURE, 0, "not compressed: %s",
					kbdfile_get_pathname(fp));

		f = kbdfile_get_file(fp);
		if (!f)
			kbd_error(EXIT_FAILURE, 0, "unable to get file: %s", ts->file);

		if (fgets(buf, sizeof(buf), f) == NULL)
			kbd_error(EXIT_FAILURE, 0, "unable to read file: %s", ts->file);

		len = strlen(buf);

		if (buf[len - 1] == '\n')
			buf[len - 1] = '\0';

		if (strcmp(buf, ts->text))
			kbd_error(EXIT_FAILURE, 0, "unexpected content of file: %s", ts->file);

		kbdfile_close(fp);
	}

	kbdfile_free(fp);

	return EXIT_SUCCESS;
}
