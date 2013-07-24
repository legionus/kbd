#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include <keymap.h>


START_TEST(test0)
{
	char *s;
	lkfile_t f;
	struct lk_ctx *ctx;

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);

	f.pipe = 0;
	strcpy(f.pathname, "charset-keymap0.map");
	f.fd = fopen(DATADIR "/charset-keymap0.map", "r");

	fail_if(lk_parse_keymap(ctx, &f) != 0, "Unable to parse keymap");

	s = lk_get_charset(ctx);

	fail_if(strcmp(s, "iso-8859-2"), "Unable to parse charset");

	lk_free(ctx);
}
END_TEST

START_TEST(test1)
{
	char *s;
	lkfile_t f;
	struct lk_ctx *ctx;

	ctx = lk_init();
	lk_set_log_fn(ctx, NULL, NULL);

	f.pipe = 0;
	strcpy(f.pathname, "null");
	f.fd = fopen("/dev/null", "r");

	fail_if(lk_parse_keymap(ctx, &f) != 0, "Unable to parse keymap");

	s = lk_get_charset(ctx);

	fail_if(s != NULL, "Wrong charset");

	lk_free(ctx);
}
END_TEST

static Suite *
libkeymap_suite(void)
{
	Suite *s = suite_create("libkeymap");
	TCase *tc_core = tcase_create(NULL);

	tcase_add_test(tc_core, test0);
	tcase_add_test(tc_core, test1);

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
