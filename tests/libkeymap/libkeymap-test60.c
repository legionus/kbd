#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <keymap.h>
#include <linux/keyboard.h>

#include "libkeymap-test.h"
#include "xkbsupport.h"

static void
set_xkb_translation_table(void)
{
	char path[512];

	if (snprintf(path, sizeof(path), "%s/../data/xkbtrans/names", TESTDIR) >= (int) sizeof(path))
		kbd_error(EXIT_FAILURE, 0, "translation table path is too long");

	if (setenv("LK_XKB_TRANSLATION_TABLE", path, 1) != 0)
		kbd_error(EXIT_FAILURE, errno, "unable to set LK_XKB_TRANSLATION_TABLE");
}

static void
set_xkb_config_root(void)
{
	char path[512];

	if (snprintf(path, sizeof(path), "%s/data/xkb", TESTDIR) >= (int) sizeof(path))
		kbd_error(EXIT_FAILURE, 0, "xkb config root path is too long");

	if (setenv("XKB_CONFIG_ROOT", path, 1) != 0)
		kbd_error(EXIT_FAILURE, errno, "unable to set XKB_CONFIG_ROOT");
}

static void
set_xcomposefile(void)
{
	char path[512];

	if (snprintf(path, sizeof(path), "%s/data/xkb/compose/en_US", TESTDIR) >= (int) sizeof(path))
		kbd_error(EXIT_FAILURE, 0, "compose file path is too long");

	if (setenv("XCOMPOSEFILE", path, 1) != 0)
		kbd_error(EXIT_FAILURE, errno, "unable to set XCOMPOSEFILE");
}

static void
set_xkb_suppress_warnings(void)
{
	if (setenv("LK_XKB_SUPPRESS_WARNINGS", "1", 1) != 0)
		kbd_error(EXIT_FAILURE, errno, "unable to set LK_XKB_SUPPRESS_WARNINGS");
}

static void
dump_compose_symbol(struct lk_ctx *ctx, FILE *fp, unsigned int code)
{
	char *sym;
	const unsigned char *p;

	if (code < 0x1000 &&
	    (KTYP(code) == KT_LATIN || KTYP(code) == KT_LETTER) &&
	    KVAL(code) >= 0x20 && KVAL(code) <= 0x7e &&
	    KVAL(code) != '\'' && KVAL(code) != '\\') {
		fprintf(fp, "'%c'", KVAL(code));
		return;
	}

	sym = lk_code_to_ksym(ctx, (int) code);
	if (sym && strcmp(sym, "-") != 0) {
		for (p = (const unsigned char *) sym; *p; p++) {
			if (*p < 0x20 || *p == 0x7f) {
				free(sym);
				fprintf(fp, "0x%x", code);
				return;
			}
		}
		fputs(sym, fp);
		free(sym);
		return;
	}

	free(sym);
	fprintf(fp, "0x%x", code);
}

static void
dump_compose_table(struct lk_ctx *ctx, FILE *fp)
{
	struct lk_kbdiacr diacr;
	int i = 0;

	while (lk_get_diacr(ctx, i++, &diacr) == 0) {
		fputs("compose ", fp);
		dump_compose_symbol(ctx, fp, diacr.diacr);
		fputc(' ', fp);
		dump_compose_symbol(ctx, fp, diacr.base);
		fputs(" to ", fp);
		dump_compose_symbol(ctx, fp, diacr.result);
		fputc('\n', fp);
	}
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	struct parsed_keymap keymap;
	struct xkeymap_params params = {
		.model = "pc104",
		.layout = "gr",
		.locale = "en_US.UTF-8",
	};

	init_test_keymap(&keymap, "xkb-gr-dump");
	set_xkb_config_root();
	set_xcomposefile();
	set_xkb_translation_table();
	set_xkb_suppress_warnings();

	if (convert_xkb_keymap(keymap.ctx, &params) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert XKB gr layout");

	lk_dump_keymap(keymap.ctx, stdout, LK_SHAPE_SEPARATE_LINES, 0);
	dump_compose_table(keymap.ctx, stdout);

	free_test_keymap(&keymap);
	return EXIT_SUCCESS;
}
