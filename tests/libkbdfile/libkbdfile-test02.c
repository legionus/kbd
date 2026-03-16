#include "libkbdfile-test.h"

static void
test_top_level_map_suffix(void)
{
	struct kbdfile *fp = new_test_kbdfile();
	const char *const dirpath[]  = { "", TESTDIR "/data/findfile/test_0/keymaps/**", NULL };
	const char *const suffixes[] = { ".map", NULL };
	find_and_expect_kbdfile(fp, "test0", dirpath, suffixes,
				TESTDIR "/data/findfile/test_0/keymaps/test0.map");
	kbdfile_free(fp);
}

static void
test_nested_names_and_suffix_preference(void)
{
	struct kbdfile *fp = new_test_kbdfile();
	const char *const dirpath[]  = { "", TESTDIR "/data/findfile/test_0/keymaps/**", NULL };
	const char *expect = TESTDIR "/data/findfile/test_0/keymaps/i386/qwertz/test2";
	const char *const searches[] = { "test2", "qwertz/test2", "i386/qwertz/test2", NULL };
	const char *const suffixes_any[] = { "", ".kmap", ".map", NULL };
	const char *const suffixes_map[] = { ".map", "", ".kmap", NULL };
	const char *const suffixes_kmap[] = { ".kmap", ".map", "", NULL };
	const char *const suffixes_mixed[] = { ".foo", ".bar", ".map", ".baz", "", NULL };

	for (int i = 0; searches[i]; i++) {
		find_and_expect_kbdfile(fp, searches[i], dirpath, suffixes_any, expect);
		kbdfile_close(fp);
	}

	find_and_expect_kbdfile(fp, "test2", dirpath, suffixes_map,
				TESTDIR "/data/findfile/test_0/keymaps/i386/qwertz/test2.map");
	kbdfile_close(fp);

	find_and_expect_kbdfile(fp, "test2", dirpath, suffixes_kmap,
				TESTDIR "/data/findfile/test_0/keymaps/i386/qwertz/test2.kmap");
	kbdfile_close(fp);

	find_and_expect_kbdfile(fp, "test2", dirpath, suffixes_mixed,
				TESTDIR "/data/findfile/test_0/keymaps/i386/qwertz/test2.map");
	kbdfile_free(fp);
}

static void
test_layout_and_absolute_lookup(void)
{
	struct kbdfile *fp = new_test_kbdfile();
	const char *const dirpath[]  = { "", TESTDIR "/data/findfile/test_0/keymaps/**", NULL };
	const char *const suffixes[] = { "", ".map", ".kmap", NULL };

	find_and_expect_kbdfile(fp, "test3", dirpath,
				(const char *const[]){ ".map", "", ".kmap", NULL },
				TESTDIR "/data/findfile/test_0/keymaps/i386/qwerty/test3.map");
	kbdfile_close(fp);

	find_and_expect_kbdfile(fp, "i386/qwerty/test3", dirpath,
				(const char *const[]){ ".map", NULL },
				TESTDIR "/data/findfile/test_0/keymaps/i386/qwerty/test3.map");
	kbdfile_close(fp);

	find_and_expect_kbdfile(fp, "qwerty/test3", dirpath,
				(const char *const[]){ ".map", NULL },
				TESTDIR "/data/findfile/test_0/keymaps/i386/qwerty/test3.map");
	kbdfile_close(fp);

	find_and_expect_kbdfile(fp,
				TESTDIR "/data/findfile/test_0/keymaps/i386/qwerty/test0",
				dirpath, suffixes,
				TESTDIR "/data/findfile/test_0/keymaps/i386/qwerty/test0.map");
	kbdfile_free(fp);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	test_top_level_map_suffix();
	test_nested_names_and_suffix_preference();
	test_layout_and_absolute_lookup();
	return EXIT_SUCCESS;
}
