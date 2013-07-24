#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include <keymap.h>


START_TEST(test_create_0)
{
	struct lk_ctx *ctx;

	ctx = lk_init();

	fail_if(ctx == NULL,
		"Unable to initialize structure by valid pointer");
	lk_free(ctx);
}
END_TEST

START_TEST(test_free_0)
{
	struct lk_ctx *ctx;

	ctx = lk_init();

	fail_if(ctx == NULL,
		"Unable to initialize structure by valid pointer");

	fail_unless(lk_free(ctx) == 0,
		"Unable to free by valid pointer");
}
END_TEST

START_TEST(test_free_1)
{
	fail_if(lk_free(NULL) == 0,
		"Possible to free NULL pointer");
}
END_TEST


static Suite *
libkeymap_suite(void)
{
	Suite *s = suite_create("libkeymap");
	TCase *tc_core = tcase_create(NULL);

	tcase_add_test(tc_core, test_create_0);
	tcase_add_test(tc_core, test_free_0);
	tcase_add_test(tc_core, test_free_1);

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
