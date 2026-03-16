#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <linux/kd.h>
#include <linux/keyboard.h>

#include <keymap.h>
#include "contextP.h"
#include "libcommon.h"

#define TEST_CONSOLE_FD 113

#ifdef KDGKBDIACRUC
#define TEST_KDGKBDIACR_REQ KDGKBDIACRUC
#else
#define TEST_KDGKBDIACR_REQ KDGKBDIACR
#endif

static int
fake_ioctl(int fd, unsigned long req, uintptr_t arg)
{
	if (fd != TEST_CONSOLE_FD)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req == KDGKBENT) {
		struct kbentry *ke = (struct kbentry *)arg;

		if (ke->kb_table == 0) {
			if (ke->kb_index == 0) {
				ke->kb_value = K(KT_LATIN, 'a');
				return 0;
			}
			if (ke->kb_index == 1) {
				ke->kb_value = K(KT_FN, 2);
				return 0;
			}

			ke->kb_value = K_HOLE;
			return 0;
		}

		if (ke->kb_table == 1 && ke->kb_index == 0) {
			ke->kb_value = K_NOSUCHMAP;
			return 0;
		}

		ke->kb_value = K_NOSUCHMAP;
		return 0;
	}

	if (req == KDGKBSENT) {
		struct kbsentry *kbs = (struct kbsentry *)arg;

		if (kbs->kb_func == 0)
			strcpy((char *)kbs->kb_string, "kernel-func");
		else
			kbs->kb_string[0] = '\0';

		return 0;
	}

	if (req == TEST_KDGKBDIACR_REQ) {
#ifdef KDGKBDIACRUC
		struct kbdiacrsuc *kd = (struct kbdiacrsuc *)arg;

		kd->kb_cnt = 1;
		kd->kbdiacruc[0].diacr = '`';
		kd->kbdiacruc[0].base = 'a';
		kd->kbdiacruc[0].result = 0x00e0;
#else
		struct kbdiacrs *kd = (struct kbdiacrs *)arg;

		kd->kb_cnt = 1;
		kd->kbdiacr[0].diacr = '`';
		kd->kbdiacr[0].base = 'a';
		kd->kbdiacr[0].result = 0x00e0;
#endif
		return 0;
	}

	errno = EINVAL;
	return -1;
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct lk_ctx *ctx;
	struct lk_ops ops;
	struct kbsentry kbs;
	struct lk_kbdiacr diacr;

	ctx = lk_init();
	if (!ctx)
		kbd_error(EXIT_FAILURE, 0, "Unable to initialize structure by valid pointer");

	lk_set_log_fn(ctx, NULL, NULL);

	ops = ctx->ops;
	ops.ioctl_fn = fake_ioctl;
	lk_set_ops(ctx, &ops);

	if (lk_kernel_keymap(ctx, TEST_CONSOLE_FD) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to load kernel keymap state");

	if (lk_get_key(ctx, 0, 0) != K(KT_LATIN, 'a'))
		kbd_error(EXIT_FAILURE, 0, "Unexpected kernel key binding");

	memset(&kbs, 0, sizeof(kbs));
	kbs.kb_func = 0;
	if (lk_get_func(ctx, &kbs) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to get kernel function string");

	if (strcmp((char *)kbs.kb_string, "kernel-func") != 0)
		kbd_error(EXIT_FAILURE, 0, "Unexpected kernel function string");

	if (lk_get_diacr(ctx, 0, &diacr) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to get kernel compose entry");

	if (diacr.result != 0x00e0)
		kbd_error(EXIT_FAILURE, 0, "Unexpected kernel compose result");

	lk_free(ctx);

	return EXIT_SUCCESS;
}
