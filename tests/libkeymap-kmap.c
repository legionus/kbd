#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include <keymap.h>


START_TEST(test_add_map_border)
{
	struct lk_ctx ctx;

	lk_init(&ctx);
	ctx.log_fn = NULL;

	fail_unless(lk_add_map(&ctx, MAX_NR_KEYMAPS) == 0,
		"Unable to define map == MAX_NR_KEYMAPS");

	fail_unless(lk_add_map(&ctx, MAX_NR_KEYMAPS*2) == 0,
		"Unable to define map == MAX_NR_KEYMAPS*2");

	fail_unless(lk_add_map(&ctx, 0) == 0,
		"Unable to define map");

	fail_unless(lk_add_map(&ctx, 0) == 0,
		"Unable to define map");

	lk_free(&ctx);
}
END_TEST

START_TEST(test_add_map_0)
{
	struct lk_ctx ctx;

	lk_init(&ctx);
	ctx.log_fn = NULL;

	fail_if(lk_add_map(&ctx, 0) != 0, "Unable to define map");
	fail_if(ctx.keymap->count != 1, "Wrong keymap number");

	fail_if(lk_add_map(&ctx, 0) != 0, "Unable to define map");
	fail_if(ctx.keymap->count != 1, "Wrong keymap number");
		
	fail_if(lk_add_map(&ctx, 1) != 0, "Unable to define map");
	fail_if(ctx.keymap->count != 2, "Wrong keymap number");

	fail_if(lk_add_map(&ctx, 2) != 0, "Unable to define map");
	fail_if(ctx.keymap->count != 3, "Wrong keymap number");

	lk_free(&ctx);
}
END_TEST

static Suite *
libkeymap_suite(void)
{
	Suite *s = suite_create("libkeymap");
	TCase *tc_core = tcase_create(NULL);

	tcase_add_test(tc_core, test_add_map_border);
	tcase_add_test(tc_core, test_add_map_0);

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
