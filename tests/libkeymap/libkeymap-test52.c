#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include <errno.h>
#include <sys/stat.h>

#include "libkeymap-test.h"

static void
write_file(const char *path, const char *content)
{
	FILE *fp = fopen(path, "w");

	if (!fp)
		kbd_error(EXIT_FAILURE, errno, "Unable to open %s", path);

	if (fputs(content, fp) == EOF)
		kbd_error(EXIT_FAILURE, errno, "Unable to write %s", path);

	fclose(fp);
}

static void
test_override_from_main_file(void)
{
	struct parsed_keymap keymap;
	char dir[] = "/tmp/libkeymap-test52-XXXXXX";
	char main_path[256];
	char child_path[256];

	if (!mkdtemp(dir))
		kbd_error(EXIT_FAILURE, errno, "mkdtemp failed");

	if (snprintf(main_path, sizeof(main_path), "%s/main.map", dir) >= (int)sizeof(main_path) ||
	    snprintf(child_path, sizeof(child_path), "%s/child.map", dir) >= (int)sizeof(child_path))
		kbd_error(EXIT_FAILURE, 0, "temporary path is too long");

	write_file(child_path, "keycode 30 = b\nkeycode 31 = c\n");
	write_file(main_path, "include \"child.map\"\nkeycode 30 = a\n");

	setenv("LOADKEYS_INCLUDE_PATH", dir, 1);

	init_test_keymap(&keymap, main_path);
	keymap.file = fopen(main_path, "r");
	if (!keymap.file)
		kbd_error(EXIT_FAILURE, errno, "Unable to open main keymap");

	if (parse_test_keymap_stream(&keymap, keymap.file) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to parse included keymap");

	if (KVAL(lk_get_key(keymap.ctx, 0, 30)) != 'a')
		kbd_error(EXIT_FAILURE, 0, "Main keymap did not override included binding");

	if (KVAL(lk_get_key(keymap.ctx, 0, 31)) != 'c')
		kbd_error(EXIT_FAILURE, 0, "Included binding was not preserved");

	free_test_keymap(&keymap);
	unlink(main_path);
	unlink(child_path);
	rmdir(dir);
}

static void
test_include_near_symlink_target(void)
{
	struct parsed_keymap keymap;
	char base_dir[] = "/tmp/libkeymap-test52-link-XXXXXX";
	char real_dir[256];
	char main_real[256];
	char child_real[256];
	char link_path[256];

	if (!mkdtemp(base_dir))
		kbd_error(EXIT_FAILURE, errno, "mkdtemp failed");

	snprintf(real_dir, sizeof(real_dir), "%s/real", base_dir);
	if (mkdir(real_dir, 0700) != 0)
		kbd_error(EXIT_FAILURE, errno, "mkdir failed");

	if (snprintf(main_real, sizeof(main_real), "%s/main.map", real_dir) >= (int)sizeof(main_real) ||
	    snprintf(child_real, sizeof(child_real), "%s/child.map", real_dir) >= (int)sizeof(child_real) ||
	    snprintf(link_path, sizeof(link_path), "%s/link.map", base_dir) >= (int)sizeof(link_path))
		kbd_error(EXIT_FAILURE, 0, "temporary path is too long");

	write_file(child_real, "keycode 30 = z\n");
	write_file(main_real, "include \"child.map\"\n");

	if (symlink(main_real, link_path) != 0)
		kbd_error(EXIT_FAILURE, errno, "symlink failed");

	unsetenv("LOADKEYS_INCLUDE_PATH");

	init_test_keymap(&keymap, link_path);
	keymap.file = fopen(link_path, "r");
	if (!keymap.file)
		kbd_error(EXIT_FAILURE, errno, "Unable to open symlinked keymap");

	if (parse_test_keymap_stream(&keymap, keymap.file) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to parse included keymap via symlink");

	if (KVAL(lk_get_key(keymap.ctx, 0, 30)) != 'z')
		kbd_error(EXIT_FAILURE, 0, "Symlink include did not resolve near target");

	free_test_keymap(&keymap);
	unlink(link_path);
	unlink(main_real);
	unlink(child_real);
	rmdir(real_dir);
	rmdir(base_dir);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	test_override_from_main_file();
	test_include_near_symlink_target();

	return EXIT_SUCCESS;
}
