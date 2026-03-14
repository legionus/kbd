#ifndef LIBKBDFILE_TEST_H
#define LIBKBDFILE_TEST_H

#include <stdlib.h>
#include <string.h>

#include <kbdfile.h>
#include "libcommon.h"

static inline struct kbdfile *
new_test_kbdfile(void)
{
	struct kbdfile *fp = kbdfile_new(NULL);

	if (!fp)
		kbd_error(EXIT_FAILURE, 0, "unable to create kbdfile");

	return fp;
}

static inline void
expect_kbdfile_path(struct kbdfile *fp, const char *expected)
{
	const char *actual = kbdfile_get_pathname(fp);

	if (strcmp(expected, actual) != 0)
		kbd_error(EXIT_FAILURE, 0, "unexpected file: %s (expected %s)", actual, expected);
}

static inline void
find_kbdfile_or_die(struct kbdfile *fp, const char *name,
		const char *const *dirpath, const char *const *suffixes)
{
	if (kbdfile_find(name, dirpath, suffixes, fp) != 0)
		kbd_error(EXIT_FAILURE, 0, "unable to find file: %s", name);
}

static inline void
find_and_expect_kbdfile(struct kbdfile *fp, const char *name,
		const char *const *dirpath, const char *const *suffixes,
		const char *expected)
{
	find_kbdfile_or_die(fp, name, dirpath, suffixes);
	expect_kbdfile_path(fp, expected);
}

static inline void
open_and_expect_kbdfile(const char *path, const char *expected)
{
	struct kbdfile *fp = kbdfile_open(NULL, path);

	if (!fp)
		kbd_error(EXIT_FAILURE, 0, "unable to create kbdfile");

	expect_kbdfile_path(fp, expected);
	kbdfile_free(fp);
}

#endif
