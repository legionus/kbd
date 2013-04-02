#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include <keymap.h>


START_TEST(test_add_map_border)
{
	struct keymap kmap;

	lk_init(&kmap);
	kmap.log_fn = NULL;

	fail_if(lk_add_map(&kmap, -1, 0) == 0,
		"Possible to define the map with index -1");

	fail_if(lk_add_map(&kmap, MAX_NR_KEYMAPS, 0) == 0,
		"Possible to define the map with index -1");

	fail_unless(lk_add_map(&kmap, 0, 0) == 0,
		"Unable to define map");

	fail_unless(lk_add_map(&kmap, 0, 0) == 0,
		"Unable to define map");

	lk_free(&kmap);
}
END_TEST

START_TEST(test_add_map_0)
{
	int explicit;
	struct keymap kmap;

	lk_init(&kmap);
	kmap.log_fn = NULL;
	kmap.keymaps_line_seen = 1;

	explicit = 0;
	fail_if(lk_add_map(&kmap, 0, explicit) == 0,
		"Possible to define map (keymaps_line_seen=%d, explicit=%d)",
			kmap.keymaps_line_seen, explicit);

	explicit = 1;
	fail_unless(lk_add_map(&kmap, 1, explicit) == 0,
		"Possible to define map (keymaps_line_seen=%d, explicit=%d)",
			kmap.keymaps_line_seen, explicit);

	lk_free(&kmap);
}
END_TEST

START_TEST(test_add_map_1)
{
	int explicit = 0;
	struct keymap kmap;

	lk_init(&kmap);
	kmap.log_fn = NULL;

	fail_if(lk_add_map(&kmap, 0, explicit) != 0, "Unable to define map");
	fail_if(kmap.max_keymap != 1, "Wrong max_keymap number");

	fail_if(lk_add_map(&kmap, 0, explicit) != 0, "Unable to define map");
	fail_if(kmap.max_keymap != 1, "Wrong max_keymap number");
		
	fail_if(lk_add_map(&kmap, 1, explicit) != 0, "Unable to define map");
	fail_if(kmap.max_keymap != 2, "Wrong max_keymap number");

	fail_if(lk_add_map(&kmap, 2, explicit) != 0, "Unable to define map");
	fail_if(kmap.max_keymap != 3, "Wrong max_keymap number");

	lk_free(&kmap);
}
END_TEST

static Suite *
libkeymap_suite(void)
{
	Suite *s = suite_create("libkeymap");
	TCase *tc_core = tcase_create(NULL);

	tcase_add_test(tc_core, test_add_map_border);
	tcase_add_test(tc_core, test_add_map_0);
	tcase_add_test(tc_core, test_add_map_1);

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
