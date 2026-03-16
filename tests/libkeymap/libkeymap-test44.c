#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <errno.h>
#include <linux/kd.h>

#include <keymap.h>
#include "contextP.h"
#include "ksyms.h"
#include "libcommon.h"

#define TEST_CONSOLE_FD 109

static int
summary_fake_ioctl(int fd, unsigned long req, uintptr_t arg)
{
	struct kbentry *ke = (struct kbentry *)arg;

	if (fd != TEST_CONSOLE_FD)
		kbd_error(EXIT_FAILURE, 0, "unexpected fd: %d", fd);

	if (req == KDGKBENT)
		return 0;

	if (req != KDSKBENT)
		kbd_error(EXIT_FAILURE, 0, "unexpected request: %lu", req);

	if (KTYP(ke->kb_value) == 0 && KVAL(ke->kb_value) < 4)
		return 0;

	errno = EINVAL;
	return -1;
}

static void
expect_contains(const char *buf, const char *needle)
{
	if (!strstr(buf, needle))
		kbd_error(EXIT_FAILURE, 0, "missing output fragment: %s", needle);
}

static void
test_kmap_helpers(void)
{
	struct lk_ctx *ctx;

	ctx = lk_init();
	if (!ctx)
		kbd_error(EXIT_FAILURE, 0, "Unable to initialize structure by valid pointer");

	lk_set_log_fn(ctx, NULL, NULL);

	if (lk_get_keys_total(ctx, 0) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unexpected key count for missing keymap");

	if (lk_add_map(ctx, 8) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to preallocate alt table");

	if (lk_set_keywords(ctx, LK_KEYWORD_ALTISMETA) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to enable alt-is-meta");

	if (lk_add_key(ctx, 0, 5, K(KT_LATIN, 'd')) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to add plain key binding");

	if (lk_get_keys_total(ctx, 0) != 6)
		kbd_error(EXIT_FAILURE, 0, "Unexpected key count for populated keymap");

	if (!lk_key_exists(ctx, 0, 5))
		kbd_error(EXIT_FAILURE, 0, "Missing plain key binding");

	if (!lk_key_exists(ctx, 8, 5))
		kbd_error(EXIT_FAILURE, 0, "Missing synthesized alt-is-meta binding");

	if (lk_get_key(ctx, 8, 5) != K(KT_META, 'd'))
		kbd_error(EXIT_FAILURE, 0, "Unexpected synthesized alt-is-meta binding");

	if (lk_del_key(ctx, 0, 5) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to delete key binding");

	if (lk_key_exists(ctx, 0, 5))
		kbd_error(EXIT_FAILURE, 0, "Deleted key binding still exists");

	if (lk_get_key(ctx, 0, 5) != K_HOLE)
		kbd_error(EXIT_FAILURE, 0, "Deleted key binding did not become a hole");

	if (lk_set_keywords(ctx, LK_KEYWORD_KEYMAPS) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to enable explicit keymaps");

	if (lk_add_key(ctx, 3, 1, K(KT_LATIN, 'x')) == 0)
		kbd_error(EXIT_FAILURE, 0, "Explicit keymaps unexpectedly allowed implicit map creation");

	if (lk_get_key(ctx, 99, 0) != -1)
		kbd_error(EXIT_FAILURE, 0, "Missing keymap did not report failure");

	lk_free(ctx);
}

static void
test_dump_summary(void)
{
	struct lk_ctx *ctx;
	char *buf = NULL;
	size_t size = 0;
	FILE *fp;

	ctx = lk_init();
	if (!ctx)
		kbd_error(EXIT_FAILURE, 0, "Unable to initialize structure by valid pointer");

	lk_set_log_fn(ctx, NULL, NULL);

	if (lk_add_key(ctx, 0, 0, K_ALLOCATED) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to mark allocated keymap");

	{
		struct lk_ops ops = ctx->ops;
		ops.ioctl_fn = summary_fake_ioctl;
		lk_set_ops(ctx, &ops);
	}

	fp = open_memstream(&buf, &size);
	if (!fp)
		kbd_error(EXIT_FAILURE, 0, "Unable to allocate memory stream");

	lk_dump_summary(ctx, fp, TEST_CONSOLE_FD);
	fclose(fp);

	expect_contains(buf, "number of keymaps in actual use:");
	expect_contains(buf, "of which 1 dynamically allocated");
	expect_contains(buf, "0x0000 - 0x0003");

	free(buf);
	lk_free(ctx);
}

static int
kernel_fake_ioctl(int fd, unsigned long req, uintptr_t arg)
{
	if (fd != 113)
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

#ifdef KDGKBDIACRUC
	if (req == KDGKBDIACRUC) {
		struct kbdiacrsuc *kd = (struct kbdiacrsuc *)arg;

		kd->kb_cnt = 1;
		kd->kbdiacruc[0].diacr = '`';
		kd->kbdiacruc[0].base = 'a';
		kd->kbdiacruc[0].result = 0x00e0;
		return 0;
	}
#else
	if (req == KDGKBDIACR) {
		struct kbdiacrs *kd = (struct kbdiacrs *)arg;

		kd->kb_cnt = 1;
		kd->kbdiacr[0].diacr = '`';
		kd->kbdiacr[0].base = 'a';
		kd->kbdiacr[0].result = 0x00e0;
		return 0;
	}
#endif

	errno = EINVAL;
	return -1;
}

static void
test_kernel_keymap(void)
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
	ops.ioctl_fn = kernel_fake_ioctl;
	lk_set_ops(ctx, &ops);

	if (lk_kernel_keymap(ctx, 113) != 0)
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
}

static void
test_charset_and_symbol_helpers(void)
{
	struct lk_ctx *ctx;
	char *buf = NULL;
	size_t size = 0;
	FILE *fp;
	char *ksym;
	int code_home;
	int code_find;
	int unicode_code;

	ctx = lk_init();
	if (!ctx)
		kbd_error(EXIT_FAILURE, 0, "Unable to initialize structure by valid pointer");

	if (lk_get_log_fn(NULL) != NULL || lk_get_log_data(NULL) != NULL ||
	    lk_get_log_priority(NULL) != -1 || lk_get_parser_flags(NULL) != (lk_flags)-1 ||
	    lk_get_keywords(NULL) != (lk_keywords)-1 || lk_set_log_fn(NULL, NULL, NULL) != -1 ||
	    lk_set_log_priority(NULL, 0) != -1 || lk_set_parser_flags(NULL, 0) != -1 ||
	    lk_set_keywords(NULL, 0) != -1)
		kbd_error(EXIT_FAILURE, 0, "NULL context helpers returned unexpected values");

	lk_set_log_fn(ctx, NULL, NULL);

	fp = open_memstream(&buf, &size);
	if (!fp)
		kbd_error(EXIT_FAILURE, 0, "Unable to allocate memory stream");

	lk_list_charsets(fp);
	fclose(fp);

	expect_contains(buf, "iso-8859-{");
	expect_contains(buf, "koi8-{");
	free(buf);

	if (lk_set_charset(ctx, "ISO-8859-2") != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to set charset case-insensitively");

	if (strcmp(lk_get_charset(ctx), "iso-8859-2") != 0)
		kbd_error(EXIT_FAILURE, 0, "Unexpected active charset");

	if (lk_set_charset(ctx, "definitely-not-a-charset") == 0)
		kbd_error(EXIT_FAILURE, 0, "Unknown charset unexpectedly succeeded");

	ksym = lk_code_to_ksym(ctx, K(KT_LATIN, 0xa1));
	if (!ksym)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert charset keysym");

	code_home = ksymtocode(ctx, "Home", TO_8BIT);
	code_find = ksymtocode(ctx, "Find", TO_8BIT);
	if (code_home != code_find || code_home == CODE_FOR_UNKNOWN_KSYM)
		kbd_error(EXIT_FAILURE, 0, "Unable to resolve synonym keysym");

	unicode_code = lk_ksym_to_unicode(ctx, ksym);
	if (unicode_code >= 0 && unicode_code < 0x80)
		kbd_error(EXIT_FAILURE, 0, "Unexpected unicode result for charset keysym");
	free(ksym);

	if (lk_convert_code(ctx, K(KT_LETTER, 'a'), TO_UNICODE) == CODE_FOR_UNKNOWN_KSYM)
		kbd_error(EXIT_FAILURE, 0, "Letter conversion unexpectedly failed");

	if (lk_add_capslock(ctx, U(0x00e9)) != K(KT_LETTER, 0x00e9))
		kbd_error(EXIT_FAILURE, 0, "Unexpected capslock conversion for latin-1 supplement");

	lk_free(ctx);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	test_kmap_helpers();
	test_dump_summary();
	test_kernel_keymap();
	test_charset_and_symbol_helpers();

	return EXIT_SUCCESS;
}
