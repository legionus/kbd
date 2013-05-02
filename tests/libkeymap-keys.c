#include <stdio.h>
#include <stdlib.h>
#include <check.h>
#include <keymap.h>


START_TEST(test_add_key_0)
{
	struct keymap kmap;
	lk_init(&kmap);
	kmap.log_fn = NULL;

	fail_if(lk_add_key(&kmap, 0, -1, 0) == 0,
		"Possible to use index < 0");

	fail_if(lk_add_key(&kmap, 0, NR_KEYS + 1, 0) == 0,
		"Possible to use index > NR_KEYS");

	fail_if(lk_add_key(&kmap, -1, 0, 0) == 0,
		"Possible to use table < 0");

	fail_if(lk_add_key(&kmap, MAX_NR_KEYMAPS + 1, 0, 0) == 0,
		"Possible to use table > MAX_NR_KEYMAPS");

	fail_if(lk_add_key(&kmap, 0, 0, -1) == 0,
		"Possible to add key a -1 keycode");

	lk_free(&kmap);
}
END_TEST

START_TEST(test_add_key_1)
{
	struct keymap kmap;
	lk_init(&kmap);
	kmap.log_fn = NULL;

	fail_unless(lk_add_key(&kmap, 0, 0, 0) == 0,
		"Unable to add keycode = 0");

	fail_unless(lk_add_key(&kmap, 0, 0, 16) == 0,
		"Unable to add keycode = 16");

	fail_unless(lk_add_key(&kmap, 1, 1, K_HOLE) == 0,
		"Unable to add keycode = K_HOLE");

	lk_free(&kmap);
}
END_TEST

START_TEST(test_add_key_2)
{
	struct keymap kmap;
	lk_init(&kmap);
	kmap.log_fn = NULL;

	kmap.flags |= LK_KEYWORD_ALTISMETA;

	fail_unless(lk_add_key(&kmap, 0, 0, 16) == 0,
		"Unable to add keycode");

	fail_unless(lk_get_key(&kmap, 0, 0) == 16,
		"Unable to get keycode");

	lk_free(&kmap);
}
END_TEST

START_TEST(test_add_func_0)
{
	char *stringvalues[30] = {
		/* F1 .. F20 */
		"\033[[A",  "\033[[B",  "\033[[C",  "\033[[D",  "\033[[E",
		"\033[17~", "\033[18~", "\033[19~", "\033[20~", "\033[21~",
		"\033[23~", "\033[24~", "\033[25~", "\033[26~",
		"\033[28~", "\033[29~",
		"\033[31~", "\033[32~", "\033[33~", "\033[34~",
		/* Find,    Insert,     Remove,     Select,     Prior */
		"\033[1~",  "\033[2~",  "\033[3~",  "\033[4~",  "\033[5~",
		/* Next,    Macro,      Help,       Do,         Pause */
		"\033[6~",  0,          0,          0,          0
	};
	int i;
	struct keymap kmap;

	lk_init(&kmap);
	kmap.log_fn = NULL;

	for (i = 0; i < 30; i++) {
		struct kbsentry ke;

		if (!(stringvalues[i]))
			continue;

		strncpy((char *)ke.kb_string, stringvalues[i],
			sizeof(ke.kb_string));
		ke.kb_string[sizeof(ke.kb_string) - 1] = 0;
		ke.kb_func = i;

		fail_if(lk_add_func(&kmap, ke) == -1,
			"Unable to add function");
	}

	lk_free(&kmap);
}
END_TEST

START_TEST(test_add_diacr_0)
{
	int i = MAX_DIACR;
	struct keymap kmap;
	lk_init(&kmap);
	kmap.log_fn = NULL;

	while (i > 0) {
		fail_if(lk_add_diacr(&kmap, 0, 0, 0) != 0,
			"Unable to add diacr");
		i--;
	}

	fail_if(lk_add_diacr(&kmap, 0, 0, 0) == 0,
		"Possible to add diacr (MAX_DIACR)");

	lk_free(&kmap);
}
END_TEST

static Suite *
libkeymap_suite(void)
{
	Suite *s = suite_create("libkeymap");
	TCase *tc_core = tcase_create(NULL);

	tcase_add_test(tc_core, test_add_key_0);
	tcase_add_test(tc_core, test_add_key_1);
	tcase_add_test(tc_core, test_add_key_2);
	tcase_add_test(tc_core, test_add_func_0);
	tcase_add_test(tc_core, test_add_diacr_0);

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
