#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include <keymap.h>

#include "modifiers.h"

struct modifier {
	const char *name;
	const int bit;
	const char ch;
};


START_TEST(test_parse_0)
{
	int c;
	lkfile_t f;
	struct lk_ctx *ctx;

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);

	f.pipe = 0;
	strcpy(f.pathname, "keymap0.map");
	f.fd = fopen(DATADIR "/keymap0.map", "r");

	fail_if(lk_parse_keymap(ctx, &f) != 0, "Unable to parse keymap");

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

	lk_free(ctx);
}
END_TEST

START_TEST(test_parse_1)
{
	int c;
	lkfile_t f;
	struct lk_ctx *ctx;

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);

	f.pipe = 0;
	strcpy(f.pathname, "keymap1.map");
	f.fd = fopen(DATADIR "/keymap1.map", "r");

	fail_if(lk_parse_keymap(ctx, &f) != 0, "Unable to parse keymap");

	c = lk_get_key(ctx, 0, 16);
	fail_if(KVAL(c) != 'q', "Unable to get keycode");

	c = lk_get_key(ctx, 1, 16);
	fail_if(KVAL(c) != 'Q', "Unable to get keycode");

	lk_free(ctx);
}
END_TEST

START_TEST(test_parse_2)
{
	int i = 0;
	lkfile_t f;
	struct lk_ctx *ctx;

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);

	f.pipe = 0;
	strcpy(f.pathname, "keymap2.map");
	f.fd = fopen(DATADIR "/keymap2.map", "r");

	fail_if(lk_parse_keymap(ctx, &f) != 0, "Unable to parse keymap");

	while (i < MAX_NR_KEYMAPS) {
		int c = lk_get_key(ctx, i, 17);
		fail_if(KVAL(c) != 'x', "Unable to get keycode");
		i++;
	}

	lk_free(ctx);
}
END_TEST

START_TEST(test_parse_3)
{
	int i;
	char str[] = "qwertyuiopasdfghjklzxcvbnm";
	lkfile_t f;
	struct lk_ctx *ctx;

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);

	f.pipe = 0;
	strcpy(f.pathname, "keymap3.map");
	f.fd = fopen(DATADIR "/keymap3.map", "r");

	fail_if(lk_parse_keymap(ctx, &f) != 0, "Unable to parse keymap");

	for (i = 0; i < 26; i++) {
		int c = lk_get_key(ctx, i, 17);
		fail_if(KVAL(c) != str[i], "Unable to get keycode");
	}

	lk_free(ctx);
}
END_TEST

START_TEST(test_parse_4)
{
	int c;
	lkfile_t f;
	struct lk_ctx *ctx;

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);

	f.pipe = 0;
	strcpy(f.pathname, "keymap4.map");
	f.fd = fopen(DATADIR "/keymap4.map", "r");

	fail_if(lk_parse_keymap(ctx, &f) != 0, "Unable to parse keymap");

	c = lk_get_key(ctx, 0, 16);
	fail_if(KVAL(c) != 'q', "Unable to get keycode");

	c = lk_get_key(ctx, 0, 17);
	fail_if(KVAL(c) != 'w', "Include40.map failed");

	c = lk_get_key(ctx, 0, 18);
	fail_if(KVAL(c) != 'e', "Include41.map failed");

	lk_free(ctx);
}
END_TEST

START_TEST(test_parse_5)
{
	int i;
	lkfile_t f;
	struct kbsentry kbs;
	struct lk_ctx *ctx;

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);

	f.pipe = 0;
	strcpy(f.pathname, "keymap5.map");
	f.fd = fopen(DATADIR "/keymap5.map", "r");

	fail_if(lk_parse_keymap(ctx, &f) != 0, "Unable to parse keymap");

	for(i = 0; i < MAX_NR_FUNC; i++) {
		kbs.kb_func = i;
		kbs.kb_string[0] = 0;
		fail_if(lk_get_func(ctx, &kbs) != 0,
			"Unable to get func %d", i);
	}

	lk_free(ctx);
}
END_TEST

START_TEST(test_parse_6)
{
	lkfile_t f;
	struct kbsentry kbs;
	struct lk_ctx *ctx;

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);

	f.pipe = 0;
	strcpy(f.pathname, "keymap6.map");
	f.fd = fopen(DATADIR "/keymap6.map", "r");

	fail_if(lk_parse_keymap(ctx, &f) != 0, "Unable to parse keymap");

	kbs.kb_func = 0;
	kbs.kb_string[0] = 0;
	fail_if(lk_get_func(ctx, &kbs) != 0, "Unable to get func 0");

	kbs.kb_func = 1;
	kbs.kb_string[0] = 0;
	fail_if(lk_get_func(ctx, &kbs) != 0, "Unable to get func 1");

	kbs.kb_func = 2;
	kbs.kb_string[0] = 0;
	fail_if(lk_get_func(ctx, &kbs) != -1, "Possible to get not alloced func");

	lk_free(ctx);
}
END_TEST

static Suite *
libkeymap_suite(void)
{
	Suite *s = suite_create("libkeymap");
	TCase *tc_core = tcase_create(NULL);

	setenv("LOADKEYS_INCLUDE_PATH", DATADIR, 1);

	tcase_add_test(tc_core, test_parse_0);
	tcase_add_test(tc_core, test_parse_1);
	tcase_add_test(tc_core, test_parse_2);
	tcase_add_test(tc_core, test_parse_3);
	tcase_add_test(tc_core, test_parse_4);
	tcase_add_test(tc_core, test_parse_5);
	tcase_add_test(tc_core, test_parse_6);

	suite_add_tcase(s, tc_core);
	return s;
}

int main(void)
{
	int number_failed;

	Suite *s = libkeymap_suite();
	SRunner *sr = srunner_create (s);

	srunner_run_all(sr, CK_NORMAL);

	number_failed = srunner_ntests_failed(sr);
	srunner_free (sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
