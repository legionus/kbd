#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include <keymap.h>
#include "libcommon.h"

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	FILE *f = NULL;
	struct kbdfile *fp = NULL;
	struct kbdfile_ctx *kbdfile_ctx;
	struct lk_ctx *ctx;
	struct lk_kbdiacr dcr;

	kbdfile_ctx = kbdfile_context_new();
	if (!kbdfile_ctx)
		kbd_error(EXIT_FAILURE, 0, "Unable to create kbdfile context");

	fp = kbdfile_new(kbdfile_ctx);
	if (!fp)
		kbd_error(EXIT_FAILURE, 0, "Unable to create kbdfile");

	ctx = lk_init();
	//lk_set_log_fn(ctx, NULL, NULL);

	kbdfile_set_pathname(fp, "keymap7.map");

	f = fopen(TESTDIR "/data/libkeymap/keymap7.map", "r");
	if (!f)
		kbd_error(EXIT_FAILURE, 0, "Unable to open: " TESTDIR "/data/libkeymap/keymap7.map: %s", strerror(errno));

	kbdfile_set_file(fp, f);

	if (lk_parse_keymap(ctx, fp) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to parse keymap");

	lk_dump_diacs(ctx, stdout);

	if (!lk_diacr_exists(ctx, 1))
		kbd_warning(0, "Unable to check existance of diacr with index 1");

	if (lk_get_diacr(ctx, 1, &dcr) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to get of diacr with index 1");

	if (dcr.diacr != '`' || dcr.base != 'a' || dcr.result != 0340)
		kbd_error(EXIT_FAILURE, 0, "Unexpected diacr with index 1");

	if (lk_del_diacr(ctx, 1) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to remove diacr with index 1");

	if (lk_diacr_exists(ctx, 1))
		kbd_warning(0, "Removed diacr with index 1 exist");

	if (lk_add_diacr(ctx, 1, &dcr) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to add diacr with index 1");

	kbdfile_free(fp);
	kbdfile_context_free(kbdfile_ctx);
	lk_free(ctx);

	return EXIT_SUCCESS;
}
