#include <errno.h>
#include <stddef.h>
#include <stdlib.h>
#include <dlfcn.h>

#include "elf-note.h"
#include "libcommon.h"

static void
assert_success_path(void)
{
	void *dl = NULL;
	void *(*malloc_fn)(size_t) = NULL;
	void (*free_fn)(void *) = NULL;
	void *ptr;
	int ret;

	ret = dlsym_many(&dl, "libc.so.6",
		&malloc_fn, "malloc",
		&free_fn, "free",
		NULL);
	if (ret != 1)
		kbd_error(EXIT_FAILURE, 0, "unexpected dlsym_many success return: %d", ret);

	if (!dl || !malloc_fn || !free_fn)
		kbd_error(EXIT_FAILURE, 0, "missing symbols after successful dlsym_many");

	ptr = malloc_fn(16);
	if (!ptr)
		kbd_error(EXIT_FAILURE, 0, "malloc symbol did not work");
	free_fn(ptr);

	ret = dlsym_many(&dl, "libc.so.6", &malloc_fn, "malloc", NULL);
	if (ret != 0)
		kbd_error(EXIT_FAILURE, 0, "unexpected cached dlsym_many return: %d", ret);

	dlclose(dl);
}

static void
assert_missing_symbol_path(void)
{
	void *dl = NULL;
	void (*missing_fn)(void) = NULL;
	int ret;

	ret = dlsym_many(&dl, "libc.so.6",
		&missing_fn, "__libkbdfile_missing_symbol__",
		NULL);
	if (ret != -ENXIO)
		kbd_error(EXIT_FAILURE, 0, "unexpected missing symbol return: %d", ret);

	if (dl != NULL)
		kbd_error(EXIT_FAILURE, 0, "library handle leaked after missing symbol");
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	assert_success_path();
	assert_missing_symbol_path();
	return EXIT_SUCCESS;
}
