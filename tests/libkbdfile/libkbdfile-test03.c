#include "config.h"

#include "libkbdfile-test.h"

static void
test_direct_open_paths(void)
{
	struct kbdfile *fp;

	open_and_expect_kbdfile(TESTDIR "/data/findfile/test_0/keymaps/i386/qwerty/test0.map",
				TESTDIR "/data/findfile/test_0/keymaps/i386/qwerty/test0.map");

	fp = kbdfile_open(NULL, TESTDIR "/data/findfile/test_0/keymaps/i386/qwerty/test0");
	if (fp)
		kbd_error(EXIT_FAILURE, 0, "unexpected kbdfile for suffixless open");
	kbdfile_free(fp);
}

static void
test_compressed_find_and_open(void)
{
	static const struct testcase {
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
	struct kbdfile *fp = new_test_kbdfile();
	const char *const dirpath[]  = { "", TESTDIR "/data/findfile/test_0/keymaps/**", NULL };
	const char *const suffixes[] = { ".map", NULL };
	FILE *f;

	find_and_expect_kbdfile(fp, "simple-1.psf",
				(const char *const[]){ "", TESTDIR "/data/findfile/test_1/consolefonts/", NULL },
				(const char *const[]){ "", ".psfu", ".psf", ".cp", ".fnt", NULL },
				TESTDIR "/data/findfile/test_1/consolefonts/simple-1.psf.gz");

	if (!kbdfile_is_compressed(fp))
		kbd_error(EXIT_FAILURE, 0, "not compressed: %s", kbdfile_get_pathname(fp));
	kbdfile_close(fp);

	setenv("KBDFILE_IGNORE_DECOMP_UTILS", "1", 1);

	for (const struct testcase *ts = cases; ts->file; ts++) {
		char buf[256];
		size_t len;

		find_and_expect_kbdfile(fp, ts->file, dirpath, suffixes, ts->file);

		if (ts->compressed && !kbdfile_is_compressed(fp))
			kbd_error(EXIT_FAILURE, 0, "not compressed: %s", kbdfile_get_pathname(fp));

		f = kbdfile_get_file(fp);
		if (!f)
			kbd_error(EXIT_FAILURE, 0, "unable to get file: %s", ts->file);

		if (fgets(buf, sizeof(buf), f) == NULL)
			kbd_error(EXIT_FAILURE, 0, "unable to read file: %s", ts->file);

		len = strlen(buf);
		if (len && buf[len - 1] == '\n')
			buf[len - 1] = '\0';

		if (strcmp(buf, ts->text) != 0)
			kbd_error(EXIT_FAILURE, 0, "unexpected content of file: %s", ts->file);

		kbdfile_close(fp);
	}

	kbdfile_free(fp);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	test_direct_open_paths();
	test_compressed_find_and_open();
	return EXIT_SUCCESS;
}
