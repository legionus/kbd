#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <check.h>

#include <keymap.h>
#include "modifiers.h"

struct modifier {
	const char *name;
	const int bit;
	const char ch;
};

START_TEST(charset_0)
{
	const char *s;
	FILE *f = NULL;
	struct kbdfile *fp = NULL;
	struct kbdfile_ctx *kbdfile_ctx;
	struct lk_ctx *ctx;

	kbdfile_ctx = kbdfile_context_new();
	fail_if(!kbdfile_ctx, "Unable to create kbdfile context");

	fp = kbdfile_new(kbdfile_ctx);
	fail_if(!fp, "Unable to create kbdfile");

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);

	kbdfile_set_pathname(fp, "charset-keymap0.map");

	f = fopen(DATADIR "/charset-keymap0.map", "r");
	fail_if(!f, "Unable to open: %s", DATADIR "/charset-keymap0.map: %s", strerror(errno));

	kbdfile_set_file(fp, f);

	fail_if(lk_parse_keymap(ctx, fp) != 0, "Unable to parse keymap");

	s = lk_get_charset(ctx);

	fail_if(strcmp(s, "iso-8859-2"), "Unable to parse charset");

	kbdfile_free(fp);
	kbdfile_context_free(kbdfile_ctx);
	lk_free(ctx);
}
END_TEST

START_TEST(charset_1)
{
	const char *s;
	FILE *f = NULL;
	struct kbdfile *fp = NULL;
	struct kbdfile_ctx *kbdfile_ctx;
	struct lk_ctx *ctx;

	kbdfile_ctx = kbdfile_context_new();
	fail_if(!kbdfile_ctx, "Unable to create kbdfile context");

	fp = kbdfile_new(kbdfile_ctx);
	fail_if(!fp, "Unable to create kbdfile");

	kbdfile_set_pathname(fp, "null");

	f = fopen("/dev/null", "r");
	fail_if(!f, "Unable to open: /dev/null: %s", strerror(errno));

	kbdfile_set_file(fp, f);

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);

	fail_if(lk_parse_keymap(ctx, fp) != 0, "Unable to parse keymap");

	s = lk_get_charset(ctx);

	fail_if(s == NULL, "Charset not found");
	fail_if(strcmp(s, "iso-8859-1"), "Unable to parse charset");

	kbdfile_free(fp);
	kbdfile_context_free(kbdfile_ctx);
	lk_free(ctx);
}
END_TEST

START_TEST(init_create_0)
{
	struct lk_ctx *ctx;

	ctx = lk_init();

	fail_if(ctx == NULL,
	        "Unable to initialize structure by valid pointer");
	lk_free(ctx);
}
END_TEST

START_TEST(init_free_0)
{
	struct lk_ctx *ctx;

	ctx = lk_init();

	fail_if(ctx == NULL,
	        "Unable to initialize structure by valid pointer");

	fail_unless(lk_free(ctx) == 0,
	            "Unable to free by valid pointer");
}
END_TEST

START_TEST(init_free_1)
{
	fail_if(lk_free(NULL) == 0,
	        "Possible to free NULL pointer");
}
END_TEST

START_TEST(keymap_parse_0)
{
	int c;
	FILE *f = NULL;
	struct kbdfile *fp = NULL;
	struct kbdfile_ctx *kbdfile_ctx;
	struct lk_ctx *ctx;

	kbdfile_ctx = kbdfile_context_new();
	fail_if(!kbdfile_ctx, "Unable to create kbdfile context");

	fp = kbdfile_new(kbdfile_ctx);
	fail_if(!fp, "Unable to create kbdfile");

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);

	kbdfile_set_pathname(fp, "keymap0.map");

	f = fopen(DATADIR "/keymap0.map", "r");
	fail_if(!f, "Unable to open: %s", DATADIR "/keymap0.map: %s", strerror(errno));

	kbdfile_set_file(fp, f);

	fail_if(lk_parse_keymap(ctx, fp) != 0, "Unable to parse keymap");

	c = lk_get_key(ctx, 0, 16);
	fail_if(KVAL(c) != 'q', "Unable to get keycode 16");

	c = lk_get_key(ctx, 0, 17);
	fail_if(KVAL(c) != 'w', "Unable to get keycode 17");

	c = lk_get_key(ctx, 0, 18);
	fail_if(KVAL(c) != 'e', "Unable to get keycode 18");

	c = lk_get_key(ctx, 0, 19);
	fail_if(KVAL(c) != 'r', "Unable to get keycode 19");

	c = lk_get_key(ctx, 0, 20);
	fail_if(KVAL(c) != 't', "Unable to get keycode 20");

	c = lk_get_key(ctx, 0, 21);
	fail_if(KVAL(c) != 'y', "Unable to get keycode 21");

	kbdfile_free(fp);
	kbdfile_context_free(kbdfile_ctx);
	lk_free(ctx);
}
END_TEST

START_TEST(keymap_parse_1)
{
	int c;
	FILE *f = NULL;
	struct kbdfile *fp = NULL;
	struct kbdfile_ctx *kbdfile_ctx;
	struct lk_ctx *ctx;

	kbdfile_ctx = kbdfile_context_new();
	fail_if(!kbdfile_ctx, "Unable to create kbdfile context");

	fp = kbdfile_new(kbdfile_ctx);
	fail_if(!fp, "Unable to create kbdfile");

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);

	kbdfile_set_pathname(fp, "keymap1.map");

	f = fopen(DATADIR "/keymap1.map", "r");
	fail_if(!f, "Unable to open: %s", DATADIR "/keymap1.map: %s", strerror(errno));

	kbdfile_set_file(fp, f);

	fail_if(lk_parse_keymap(ctx, fp) != 0, "Unable to parse keymap");

	c = lk_get_key(ctx, 0, 16);
	fail_if(KVAL(c) != 'q', "Unable to get keycode");

	c = lk_get_key(ctx, 1, 16);
	fail_if(KVAL(c) != 'Q', "Unable to get keycode");

	kbdfile_free(fp);
	kbdfile_context_free(kbdfile_ctx);
	lk_free(ctx);
}
END_TEST

START_TEST(keymap_parse_2)
{
	unsigned int i = 0;
	FILE *f = NULL;
	struct kbdfile *fp = NULL;
	struct kbdfile_ctx *kbdfile_ctx;
	struct lk_ctx *ctx;

	kbdfile_ctx = kbdfile_context_new();
	fail_if(!kbdfile_ctx, "Unable to create kbdfile context");

	fp = kbdfile_new(kbdfile_ctx);
	fail_if(!fp, "Unable to create kbdfile");

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);

	kbdfile_set_pathname(fp, "keymap2.map");

	f = fopen(DATADIR "/keymap2.map", "r");
	fail_if(!f, "Unable to open: %s", DATADIR "/keymap2.map: %s", strerror(errno));

	kbdfile_set_file(fp, f);

	fail_if(lk_parse_keymap(ctx, fp) != 0, "Unable to parse keymap");

	while (i < MAX_NR_KEYMAPS) {
		int c = lk_get_key(ctx, i, 17);
		fail_if(KVAL(c) != 'x', "Unable to get keycode");
		i++;
	}

	kbdfile_free(fp);
	kbdfile_context_free(kbdfile_ctx);
	lk_free(ctx);
}
END_TEST

START_TEST(keymap_parse_3)
{
	unsigned int i;
	char str[] = "qwertyuiopasdfghjklzxcvbnm";
	FILE *f = NULL;
	struct kbdfile *fp = NULL;
	struct kbdfile_ctx *kbdfile_ctx;
	struct lk_ctx *ctx;

	kbdfile_ctx = kbdfile_context_new();
	fail_if(!kbdfile_ctx, "Unable to create kbdfile context");

	fp = kbdfile_new(kbdfile_ctx);
	fail_if(!fp, "Unable to create kbdfile");

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);

	kbdfile_set_pathname(fp, "keymap3.map");

	f = fopen(DATADIR "/keymap3.map", "r");
	fail_if(!f, "Unable to open: %s", DATADIR "/keymap3.map: %s", strerror(errno));

	kbdfile_set_file(fp, f);

	fail_if(lk_parse_keymap(ctx, fp) != 0, "Unable to parse keymap");

	for (i = 0; i < 26; i++) {
		int c = lk_get_key(ctx, i, 17);
		fail_if(KVAL(c) != str[i], "Unable to get keycode");
	}

	kbdfile_free(fp);
	kbdfile_context_free(kbdfile_ctx);
	lk_free(ctx);
}
END_TEST

START_TEST(keymap_parse_4)
{
	int c;
	FILE *f = NULL;
	struct kbdfile *fp = NULL;
	struct kbdfile_ctx *kbdfile_ctx;
	struct lk_ctx *ctx;

	kbdfile_ctx = kbdfile_context_new();
	fail_if(!kbdfile_ctx, "Unable to create kbdfile context");

	fp = kbdfile_new(kbdfile_ctx);
	fail_if(!fp, "Unable to create kbdfile");

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);

	kbdfile_set_pathname(fp, "keymap4.map");

	f = fopen(DATADIR "/keymap4.map", "r");
	fail_if(!f, "Unable to open: %s", DATADIR "/keymap4.map: %s", strerror(errno));

	kbdfile_set_file(fp, f);

	fail_if(lk_parse_keymap(ctx, fp) != 0, "Unable to parse keymap");

	c = lk_get_key(ctx, 0, 16);
	fail_if(KVAL(c) != 'q', "Unable to get keycode");

	c = lk_get_key(ctx, 0, 17);
	fail_if(KVAL(c) != 'w', "Include40.map failed");

	c = lk_get_key(ctx, 0, 18);
	fail_if(KVAL(c) != 'e', "Include41.map failed");

	kbdfile_free(fp);
	kbdfile_context_free(kbdfile_ctx);
	lk_free(ctx);
}
END_TEST

START_TEST(keymap_parse_5)
{
	unsigned int i;
	FILE *f = NULL;
	struct kbdfile *fp = NULL;
	struct kbdfile_ctx *kbdfile_ctx;
	struct kbsentry kbs;
	struct lk_ctx *ctx;

	kbdfile_ctx = kbdfile_context_new();
	fail_if(!kbdfile_ctx, "Unable to create kbdfile context");

	fp = kbdfile_new(kbdfile_ctx);
	fail_if(!fp, "Unable to create kbdfile");

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);

	kbdfile_set_pathname(fp, "keymap5.map");

	f = fopen(DATADIR "/keymap5.map", "r");
	fail_if(!f, "Unable to open: %s", DATADIR "/keymap5.map: %s", strerror(errno));

	kbdfile_set_file(fp, f);

	fail_if(lk_parse_keymap(ctx, fp) != 0, "Unable to parse keymap");

	for (i = 0; i < MAX_NR_FUNC; i++) {
		kbs.kb_func      = (unsigned char) i;
		kbs.kb_string[0] = 0;
		fail_if(lk_get_func(ctx, &kbs) != 0,
		        "Unable to get func %d", i);
	}

	kbdfile_free(fp);
	kbdfile_context_free(kbdfile_ctx);
	lk_free(ctx);
}
END_TEST

START_TEST(keymap_parse_6)
{
	FILE *f = NULL;
	struct kbdfile *fp = NULL;
	struct kbdfile_ctx *kbdfile_ctx;
	struct kbsentry kbs;
	struct lk_ctx *ctx;

	kbdfile_ctx = kbdfile_context_new();
	fail_if(!kbdfile_ctx, "Unable to create kbdfile context");

	fp = kbdfile_new(kbdfile_ctx);
	fail_if(!fp, "Unable to create kbdfile");

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);

	kbdfile_set_pathname(fp, "keymap6.map");

	f = fopen(DATADIR "/keymap6.map", "r");
	fail_if(!f, "Unable to open: %s", DATADIR "/keymap6.map: %s", strerror(errno));

	kbdfile_set_file(fp, f);

	fail_if(lk_parse_keymap(ctx, fp) != 0, "Unable to parse keymap");

	kbs.kb_func      = 0;
	kbs.kb_string[0] = 0;
	fail_if(lk_get_func(ctx, &kbs) != 0, "Unable to get func 0");

	kbs.kb_func      = 1;
	kbs.kb_string[0] = 0;
	fail_if(lk_get_func(ctx, &kbs) != 0, "Unable to get func 1");

	kbs.kb_func      = 2;
	kbs.kb_string[0] = 0;
	fail_if(lk_get_func(ctx, &kbs) != -1, "Possible to get not alloced func");

	kbdfile_free(fp);
	kbdfile_context_free(kbdfile_ctx);
	lk_free(ctx);
}
END_TEST

START_TEST(kmap_add_map_border)
{
	struct lk_ctx *ctx;

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);

	fail_unless(lk_add_map(ctx, MAX_NR_KEYMAPS) == 0,
	            "Unable to define map == MAX_NR_KEYMAPS");

	fail_unless(lk_add_map(ctx, MAX_NR_KEYMAPS * 2) == 0,
	            "Unable to define map == MAX_NR_KEYMAPS*2");

	fail_unless(lk_add_map(ctx, 0) == 0,
	            "Unable to define map");

	fail_unless(lk_add_map(ctx, 0) == 0,
	            "Unable to define map");

	lk_free(ctx);
}
END_TEST

START_TEST(kmap_add_map_0)
{
	struct lk_ctx *ctx;
	struct kmapinfo info;

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);

	fail_if(lk_add_map(ctx, 0) != 0, "Unable to define map");
	lk_get_kmapinfo(ctx, &info);
	fail_if(info.keymaps != 1, "Wrong keymap number");

	fail_if(lk_add_map(ctx, 0) != 0, "Unable to define map");
	lk_get_kmapinfo(ctx, &info);
	fail_if(info.keymaps != 1, "Wrong keymap number");

	fail_if(lk_add_map(ctx, 1) != 0, "Unable to define map");
	lk_get_kmapinfo(ctx, &info);
	fail_if(info.keymaps != 2, "Wrong keymap number");

	fail_if(lk_add_map(ctx, 2) != 0, "Unable to define map");
	lk_get_kmapinfo(ctx, &info);
	fail_if(info.keymaps != 3, "Wrong keymap number");

	lk_free(ctx);
}
END_TEST

START_TEST(test_add_key_0)
{
	struct lk_ctx *ctx;

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);

	fail_if(lk_add_key(ctx, 0, NR_KEYS + 1, 0) != 0,
	        "Unable to use index > NR_KEYS");

	fail_if(lk_add_key(ctx, MAX_NR_KEYMAPS + 1, 0, 0) != 0,
	        "Unable to use table > MAX_NR_KEYMAPS");

	lk_free(ctx);
}
END_TEST

START_TEST(test_add_key_1)
{
	struct lk_ctx *ctx;

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);

	fail_unless(lk_add_key(ctx, 0, 0, 0) == 0,
	            "Unable to add keycode = 0");

	fail_unless(lk_add_key(ctx, 0, 0, 16) == 0,
	            "Unable to add keycode = 16");

	fail_unless(lk_add_key(ctx, 1, 1, K_HOLE) == 0,
	            "Unable to add keycode = K_HOLE");

	lk_free(ctx);
}
END_TEST

START_TEST(test_add_key_2)
{
	struct lk_ctx *ctx;

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);
	lk_set_parser_flags(ctx, LK_KEYWORD_ALTISMETA);

	fail_unless(lk_add_key(ctx, 0, 0, 16) == 0,
	            "Unable to add keycode");

	fail_unless(lk_get_key(ctx, 0, 0) == 16,
	            "Unable to get keycode");

	lk_free(ctx);
}
END_TEST

START_TEST(test_add_func_0)
{
	const char *stringvalues[30] = {
		/* F1 .. F20 */
		"\033[[A", "\033[[B", "\033[[C", "\033[[D", "\033[[E",
		"\033[17~", "\033[18~", "\033[19~", "\033[20~", "\033[21~",
		"\033[23~", "\033[24~", "\033[25~", "\033[26~",
		"\033[28~", "\033[29~",
		"\033[31~", "\033[32~", "\033[33~", "\033[34~",
		/* Find,    Insert,     Remove,     Select,     Prior */
		"\033[1~", "\033[2~", "\033[3~", "\033[4~", "\033[5~",
		/* Next,    Macro,      Help,       Do,         Pause */
		"\033[6~", 0, 0, 0, 0
	};
	unsigned int i;
	struct lk_ctx *ctx;

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);

	for (i = 0; i < 30; i++) {
		struct kbsentry ke;

		if (!(stringvalues[i]))
			continue;

		strncpy((char *)ke.kb_string, stringvalues[i],
		        sizeof(ke.kb_string));
		ke.kb_string[sizeof(ke.kb_string) - 1] = 0;
		ke.kb_func                             = (unsigned char) i;

		fail_if(lk_add_func(ctx, &ke) == -1,
		        "Unable to add function");
	}

	lk_free(ctx);
}
END_TEST

START_TEST(test_add_diacr_0)
{
	int i = MAX_DIACR + 10;
	struct lk_ctx *ctx;
	struct lk_kbdiacr ptr;

	ptr.diacr  = 0;
	ptr.base   = 0;
	ptr.result = 0;

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);

	while (i > 0) {
		fail_if(lk_append_diacr(ctx, &ptr) != 0,
		        "Unable to add diacr");
		i--;
	}

	lk_free(ctx);
}
END_TEST

static Suite *
libkeymap_suite(void)
{
	Suite *s = suite_create("libkeymap");

	TCase *tc_charset = tcase_create("charset");
	tcase_add_test(tc_charset, charset_0);
	tcase_add_test(tc_charset, charset_1);
	suite_add_tcase(s, tc_charset);

	TCase *tc_init = tcase_create("init");
	tcase_add_test(tc_init, init_create_0);
	tcase_add_test(tc_init, init_free_0);
	tcase_add_test(tc_init, init_free_1);
	suite_add_tcase(s, tc_init);

	setenv("LOADKEYS_INCLUDE_PATH", DATADIR, 1);

	TCase *tc_keymap_parse = tcase_create("keymap parser");
	tcase_add_test(tc_keymap_parse, keymap_parse_0);
	tcase_add_test(tc_keymap_parse, keymap_parse_1);
	tcase_add_test(tc_keymap_parse, keymap_parse_2);
	tcase_add_test(tc_keymap_parse, keymap_parse_3);
	tcase_add_test(tc_keymap_parse, keymap_parse_4);
	tcase_add_test(tc_keymap_parse, keymap_parse_5);
	tcase_add_test(tc_keymap_parse, keymap_parse_6);
	suite_add_tcase(s, tc_keymap_parse);

	TCase *tc_keymap = tcase_create("keymap");
	tcase_add_test(tc_keymap, kmap_add_map_border);
	tcase_add_test(tc_keymap, kmap_add_map_0);
	suite_add_tcase(s, tc_keymap);

	TCase *tc_keys = tcase_create("keys");
	tcase_add_test(tc_keys, test_add_key_0);
	tcase_add_test(tc_keys, test_add_key_1);
	tcase_add_test(tc_keys, test_add_key_2);
	tcase_add_test(tc_keys, test_add_func_0);
	tcase_add_test(tc_keys, test_add_diacr_0);
	suite_add_tcase(s, tc_keys);

	return s;
}

int main(void)
{
	int number_failed;

	Suite *s    = libkeymap_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);

	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
