#include <errno.h>
#include <linux/kd.h>
#include <linux/keyboard.h>
#include <stdio.h>
#include <stdlib.h>

#include <keymap.h>

#include "contextP.h"
#include "libcommon.h"

#define TEST_CONSOLE_FD 131
#define TEST_PROBE_TABLE 1
#define KT_CSI_TYPE 15

enum probe_result {
	PROBE_SUPPORTED,
	PROBE_UNSUPPORTED,
	PROBE_UNAVAILABLE,
};

static enum probe_result result;
static int kbd_mode;
static int calls_find_table;
static int calls_probe;
static int calls_remove_table;
static int calls_set_xlate;
static int calls_restore_mode;

static int
fake_ioctl(int fd, unsigned long req, uintptr_t arg)
{
	struct kbentry *entry = (struct kbentry *)arg;

	if (fd != TEST_CONSOLE_FD)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	switch (req) {
		case KDGKBENT:
			if (entry->kb_index != 0)
				kbd_error(EXIT_FAILURE, 0, "unexpected key index: %u", entry->kb_index);
			if (entry->kb_table == TEST_PROBE_TABLE) {
				calls_find_table++;
				entry->kb_value = K_NOSUCHMAP;
				return 0;
			}
			kbd_error(EXIT_FAILURE, 0, "unexpected key table: %u", entry->kb_table);
			return -1;
		case KDGKBMODE:
			*(int *)arg = kbd_mode;
			return 0;
		case KDSKBMODE:
			if ((int)arg == K_XLATE) {
				calls_set_xlate++;
				kbd_mode = K_XLATE;
			} else {
				calls_restore_mode++;
				kbd_mode = (int)arg;
			}
			return 0;
		case KDSKBENT:
			if (entry->kb_table != TEST_PROBE_TABLE || entry->kb_index != 0)
				kbd_error(EXIT_FAILURE, 0, "unexpected keymap probe entry");
			if (entry->kb_value == K(KT_CSI_TYPE, 0)) {
				calls_probe++;
				if (result == PROBE_UNSUPPORTED) {
					errno = EINVAL;
					return -1;
				}
				if (result == PROBE_UNAVAILABLE) {
					errno = EIO;
					return -1;
				}
				return 0;
			}

			if (entry->kb_value != K_NOSUCHMAP)
				kbd_error(EXIT_FAILURE, 0, "unexpected cleanup entry: %u", entry->kb_value);
			calls_remove_table++;
			return 0;
	}

	kbd_error(EXIT_FAILURE, 0, "unexpected request: %lu", req);
	return -1;
}

static void
reset(enum probe_result new_result)
{
	result = new_result;
	kbd_mode = K_UNICODE;
	calls_find_table = 0;
	calls_probe = 0;
	calls_remove_table = 0;
	calls_set_xlate = 0;
	calls_restore_mode = 0;
}

static void
check_result(struct lk_ctx *ctx, enum probe_result new_result,
	     enum lk_feature_status expected, int expected_rc)
{
	reset(new_result);
	if (lk_check_features(ctx, TEST_CONSOLE_FD) != expected_rc)
		kbd_error(EXIT_FAILURE, 0, "unexpected feature probe result");
	if (lk_get_feature_status(ctx, LK_FEATURE_KT_CSI) != expected)
		kbd_error(EXIT_FAILURE, 0, "unexpected KT_CSI feature status");
	if (calls_find_table != 1 || calls_probe != 1 || calls_set_xlate != 1 ||
	    calls_restore_mode != 1)
		kbd_error(EXIT_FAILURE, 0, "incomplete KT_CSI probe");
	if (calls_remove_table != 1)
		kbd_error(EXIT_FAILURE, 0, "feature probe did not remove temporary keymap");
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct lk_ctx *ctx;
	struct lk_ops ops;

	ctx = lk_init();
	if (!ctx)
		kbd_error(EXIT_FAILURE, 0, "Unable to initialize structure");

	lk_set_log_fn(ctx, NULL, NULL);
	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl;
	lk_set_ops(ctx, &ops);

	if (lk_get_feature_status(ctx, LK_FEATURE_KT_CSI) != LK_FEATURE_UNCHECKED)
		kbd_error(EXIT_FAILURE, 0, "feature was checked before probing");
	if (lk_feature_from_name("KT_CSI") != LK_FEATURE_KT_CSI ||
	    lk_feature_from_name("unknown") != LK_FEATURE_COUNT ||
	    !lk_feature_name(LK_FEATURE_KT_CSI))
		kbd_error(EXIT_FAILURE, 0, "feature registry lookup failed");

	check_result(ctx, PROBE_SUPPORTED, LK_FEATURE_SUPPORTED, 0);
	check_result(ctx, PROBE_UNSUPPORTED, LK_FEATURE_UNSUPPORTED, 0);
	check_result(ctx, PROBE_UNAVAILABLE, LK_FEATURE_UNAVAILABLE, -1);

	lk_free(ctx);
	return EXIT_SUCCESS;
}
