#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>

#include <kbd/keymap/array.h>
#include <keymap.h>
#include "contextP.h"
#include "libcommon.h"
#include "libkeymap-test.h"

static void
expect_parse_failure(const char *pathname, const char *content, const char *what)
{
	struct parsed_keymap keymap;

	if (parse_test_keymap_string(&keymap, pathname, content) == 0)
		kbd_error(EXIT_FAILURE, 0, "Unexpected success for %s", what);

	free_test_keymap(&keymap);
}

static void
expect_out_of_range_keycode_ignored(const char *pathname, const char *content)
{
	struct parsed_keymap keymap;

	if (parse_test_keymap_string(&keymap, pathname, content) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unexpected failure for %s", pathname);

	if (keymap.ctx->key_constant->total > NR_KEYS)
		kbd_error(EXIT_FAILURE, 0, "Out-of-range keycode grew key_constant to %zu",
			  keymap.ctx->key_constant->total);

	if (KVAL(lk_get_key(keymap.ctx, 0, 30)) != 'a')
		kbd_error(EXIT_FAILURE, 0, "Valid keycode binding missing after ignored keycode");

	free_test_keymap(&keymap);
}

int main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct lk_array a;
	int val = 42;
	int ret;

	/* Zero element size must be rejected */
	if (lk_array_init(&a, 0, 0) == 0)
		kbd_error(EXIT_FAILURE, 0, "lk_array_init accepted zero memb");

	/* Overflowing initial size must be rejected and leave array zeroed */
	if (lk_array_init(&a, SIZE_MAX - 1, SIZE_MAX - 1) == 0)
		kbd_error(EXIT_FAILURE, 0, "lk_array_init accepted overflowing size * memb");

	if (a.memb != 0 || a.total != 0 || a.array != NULL)
		kbd_error(EXIT_FAILURE, 0, "failed lk_array_init left non-zero state");

	if (lk_array_init(&a, sizeof(int), 0) < 0)
		kbd_error(EXIT_FAILURE, errno, "lk_array_init failed");

	/* NULL element pointer must be rejected without resizing the array */
	if (lk_array_set(&a, 5, NULL) == 0 || a.total != 0)
		kbd_error(EXIT_FAILURE, 0, "lk_array_set accepted NULL element or resized array");

	if (lk_array_append(&a, NULL) == 0 || a.total != 0)
		kbd_error(EXIT_FAILURE, 0, "lk_array_append accepted NULL element or resized array");

	/* SIZE_MAX index must not wrap i + 1 to 0 and must be rejected */
	if (lk_array_set(&a, SIZE_MAX, &val) == 0)
		kbd_error(EXIT_FAILURE, 0, "lk_array_set accepted SIZE_MAX index");

	/* Index causing size_t overflow in (i + 1) * memb must fail cleanly */
	ret = lk_array_set(&a, SIZE_MAX / sizeof(int), &val);
	if (ret == 0)
		kbd_error(EXIT_FAILURE, 0, "lk_array_set accepted overflowing index");

	if (errno != EINVAL)
		kbd_error(EXIT_FAILURE, 0, "expected EINVAL, got %d", errno);

	/* Normal insert and retrieval */
	if (lk_array_set(&a, 10, &val) < 0)
		kbd_error(EXIT_FAILURE, errno, "lk_array_set failed for valid index");

	int *got = lk_array_get(&a, 10);
	if (!got || *got != 42)
		kbd_error(EXIT_FAILURE, 0, "lk_array_get returned invalid value");

	/* Cleanup must succeed without memory corruption or double-free */
	lk_array_free(&a);

	/* Out-of-range keycodes must be ignored with a warning, without oversized allocations */
	expect_out_of_range_keycode_ignored("inline-large-keycode.map",
					    "keycode 1073741824 = b\n"
					    "keycode 464 = VoidSymbol\n"
					    "plain keycode 256 = c\n"
					    "control keycode 256 = d\n"
					    "keycode 30 = a\n");

	/* Out-of-range keymap tables must fail */
	expect_parse_failure("inline-keymaps-out-of-range.map",
			     "keymaps 0-300\n",
			     "keymap range exceeding MAX_NR_KEYMAPS");

	return EXIT_SUCCESS;
}
