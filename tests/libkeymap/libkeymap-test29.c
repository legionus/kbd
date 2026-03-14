#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <linux/keyboard.h>

#include <keymap.h>
#include "contextP.h"
#include "libcommon.h"

#define TEST_CONSOLE_FD 97
#define TEST_FUNC_INDEX 5
#define TEST_FUNC_TEXT "hello\n"

static unsigned int set_calls;
static unsigned int clear_calls;

static int
fake_ioctl(int fd, unsigned long req, uintptr_t arg)
{
	struct kbsentry *kbs = (struct kbsentry *)arg;

	if (fd != TEST_CONSOLE_FD)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req != KDSKBSENT)
		kbd_error(EXIT_FAILURE, 0, "unexpected request: %lu", req);

	if (kbs->kb_func == TEST_FUNC_INDEX) {
		if (strcmp((char *)kbs->kb_string, TEST_FUNC_TEXT) != 0)
			kbd_error(EXIT_FAILURE, 0, "unexpected function string");

		set_calls++;
		return 0;
	}

	if (kbs->kb_string[0] != '\0')
		kbd_error(EXIT_FAILURE, 0, "unexpected non-empty clear string for function %u",
			kbs->kb_func);

	clear_calls++;
	return 0;
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct lk_ctx *ctx;
	struct lk_ops ops;
	struct kbsentry kbs;

	ctx = lk_init();
	if (!ctx)
		kbd_error(EXIT_FAILURE, 0, "Unable to initialize structure by valid pointer");

	lk_set_log_fn(ctx, NULL, NULL);

	memset(&kbs, 0, sizeof(kbs));
	kbs.kb_func = TEST_FUNC_INDEX;
	strlcpy((char *)kbs.kb_string, TEST_FUNC_TEXT, sizeof(kbs.kb_string));

	if (lk_add_func(ctx, &kbs) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to add function string");

	if (lk_set_parser_flags(ctx, LK_FLAG_CLEAR_STRINGS) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to enable clear strings");

	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl;
	lk_set_ops(ctx, &ops);

	if (lk_load_keymap(ctx, TEST_CONSOLE_FD, K_XLATE) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to load function strings");

	if (set_calls != 1U)
		kbd_error(EXIT_FAILURE, 0, "unexpected set call count: %u", set_calls);

	if (clear_calls != MAX_NR_FUNC - 1U)
		kbd_error(EXIT_FAILURE, 0, "unexpected clear call count: %u", clear_calls);

	lk_free(ctx);

	return EXIT_SUCCESS;
}
