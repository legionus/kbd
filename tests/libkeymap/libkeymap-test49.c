#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <keymap.h>
#include "contextP.h"
#include "ksyms.h"
#include "libcommon.h"

static void
expect_contains(const char *buf, const char *needle)
{
	if (!strstr(buf, needle))
		kbd_error(EXIT_FAILURE, 0, "missing output fragment: %s", needle);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
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

	return EXIT_SUCCESS;
}
