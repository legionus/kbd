#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <check.h>

#include <kbdfile.h>

START_TEST(test_0)
{
	struct kbdfile_ctx *ctx = kbdfile_context_new();
	fail_if(!ctx, "unable to create context");

	struct kbdfile *fp = kbdfile_new(ctx);
	fail_if(!fp, "unable to create kbdfile");

	kbdfile_free(fp);
	kbdfile_context_free(ctx);
}
END_TEST

START_TEST(test_1)
{
	struct kbdfile_ctx *ctx = kbdfile_context_new();
	fail_if(!ctx, "unable to create context");

	struct kbdfile *fp = kbdfile_new(ctx);
	fail_if(!fp, "unable to create kbdfile");

	const char *const dirpath[]  = { "", DATADIR "/findfile/test_0/keymaps/**", 0 };
	const char *const suffixes[] = { ".map", 0 };

	const char *expect = DATADIR "/findfile/test_0/keymaps/test0.map";

	int rc = 0;

	rc = kbdfile_find((char *)"test0", dirpath, suffixes, fp);

	fail_if(rc != 0, "unable to find file");
	fail_if(strcmp(expect, kbdfile_get_pathname(fp)) != 0, "unexpected file: %s (expected %s)", kbdfile_get_pathname(fp), expect);

	kbdfile_free(fp);
	kbdfile_context_free(ctx);
}
END_TEST

START_TEST(test_2)
{
	struct kbdfile_ctx *ctx = kbdfile_context_new();
	fail_if(!ctx, "unable to create context");

	struct kbdfile *fp = kbdfile_new(ctx);
	fail_if(!fp, "unable to create kbdfile");

	const char *const dirpath[]  = { "", DATADIR "/findfile/test_0/keymaps/**", 0 };
	const char *const suffixes[] = { "", ".kmap", ".map", 0 };

	const char *expect = DATADIR "/findfile/test_0/keymaps/i386/qwertz/test2";

	int rc = kbdfile_find((char *)"test2", dirpath, suffixes, fp);

	fail_if(rc != 0, "unable to find file");
	fail_if(strcmp(expect, kbdfile_get_pathname(fp)) != 0, "unexpected file: %s (expected %s)", kbdfile_get_pathname(fp), expect);

	kbdfile_free(fp);
	kbdfile_context_free(ctx);
}
END_TEST

START_TEST(test_3)
{
	struct kbdfile_ctx *ctx = kbdfile_context_new();
	fail_if(!ctx, "unable to create context");

	struct kbdfile *fp = kbdfile_new(ctx);
	fail_if(!fp, "unable to create kbdfile");

	const char *const dirpath[]  = { "", DATADIR "/findfile/test_0/keymaps/**", 0 };
	const char *const suffixes[] = { ".map", "", ".kmap", 0 };

	const char *expect = DATADIR "/findfile/test_0/keymaps/i386/qwertz/test2.map";

	int rc = kbdfile_find((char *)"test2", dirpath, suffixes, fp);

	fail_if(rc != 0, "unable to find file");
	fail_if(strcmp(expect, kbdfile_get_pathname(fp)) != 0, "unexpected file: %s (expected %s)", kbdfile_get_pathname(fp), expect);

	kbdfile_free(fp);
	kbdfile_context_free(ctx);
}
END_TEST

START_TEST(test_4)
{
	struct kbdfile_ctx *ctx = kbdfile_context_new();
	fail_if(!ctx, "unable to create context");

	struct kbdfile *fp = kbdfile_new(ctx);
	fail_if(!fp, "unable to create kbdfile");

	const char *const dirpath[]  = { "", DATADIR "/findfile/test_0/keymaps/**", 0 };
	const char *const suffixes[] = { ".kmap", ".map", "", 0 };

	const char *expect = DATADIR "/findfile/test_0/keymaps/i386/qwertz/test2.kmap";

	int rc = kbdfile_find((char *)"test2", dirpath, suffixes, fp);

	fail_if(rc != 0, "unable to find file");
	fail_if(strcmp(expect, kbdfile_get_pathname(fp)) != 0, "unexpected file: %s (expected %s)", kbdfile_get_pathname(fp), expect);

	kbdfile_free(fp);
	kbdfile_context_free(ctx);
}
END_TEST

START_TEST(test_5)
{
	struct kbdfile_ctx *ctx = kbdfile_context_new();
	fail_if(!ctx, "unable to create context");

	struct kbdfile *fp = kbdfile_new(ctx);
	fail_if(!fp, "unable to create kbdfile");

	const char *const dirpath[]  = { "", DATADIR "/findfile/test_0/keymaps/**", 0 };
	const char *const suffixes[] = { ".foo", ".bar", ".map", ".baz", "", 0 };

	const char *expect = DATADIR "/findfile/test_0/keymaps/i386/qwertz/test2.map";

	int rc = kbdfile_find((char *)"test2", dirpath, suffixes, fp);

	fail_if(rc != 0, "unable to find file");
	fail_if(strcmp(expect, kbdfile_get_pathname(fp)) != 0, "unexpected file: %s (expected %s)", kbdfile_get_pathname(fp), expect);

	kbdfile_free(fp);
	kbdfile_context_free(ctx);
}
END_TEST

START_TEST(test_6)
{
	struct kbdfile_ctx *ctx = kbdfile_context_new();
	fail_if(!ctx, "unable to create context");

	struct kbdfile *fp = kbdfile_new(ctx);
	fail_if(!fp, "unable to create kbdfile");

	const char *const dirpath[]  = { "", DATADIR "/findfile/test_0/keymaps/**", 0 };
	const char *const suffixes[] = { ".map", "", ".kmap", 0 };

	const char *expect = DATADIR "/findfile/test_0/keymaps/i386/qwerty/test3.map";

	int rc = kbdfile_find((char *)"test3", dirpath, suffixes, fp);

	fail_if(rc != 0, "unable to find file");
	fail_if(strcmp(expect, kbdfile_get_pathname(fp)) != 0, "unexpected file: %s (expected %s)", kbdfile_get_pathname(fp), expect);

	kbdfile_free(fp);
	kbdfile_context_free(ctx);
}
END_TEST

START_TEST(test_7)
{
	struct kbdfile_ctx *ctx = kbdfile_context_new();
	fail_if(!ctx, "unable to create context");

	struct kbdfile *fp = kbdfile_new(ctx);
	fail_if(!fp, "unable to create kbdfile");

	const char *const dirpath[]  = { "", DATADIR "/findfile/test_0/keymaps/**", 0 };
	const char *const suffixes[] = { "", ".map", ".kmap", 0 };

	const char *expect = ABS_DATADIR "/findfile/test_0/keymaps/i386/qwerty/test0.map";

	int rc = 0;

	rc = kbdfile_find((char *)(ABS_DATADIR "/findfile/test_0/keymaps/i386/qwerty/test0"), dirpath, suffixes, fp);

	fail_if(rc != 0, "unable to find file");
	fail_if(strcmp(expect, kbdfile_get_pathname(fp)) != 0, "unexpected file: %s (expected %s)", kbdfile_get_pathname(fp), expect);

	kbdfile_free(fp);
	kbdfile_context_free(ctx);
}
END_TEST

START_TEST(test_8)
{
	struct kbdfile_ctx *ctx = kbdfile_context_new();
	fail_if(!ctx, "unable to create context");

	struct kbdfile *fp = kbdfile_open(ctx, DATADIR "/findfile/test_0/keymaps/i386/qwerty/test0.map");
	fail_if(!fp, "unable to create kbdfile");

	const char *expect = DATADIR "/findfile/test_0/keymaps/i386/qwerty/test0.map";
	fail_if(strcmp(expect, kbdfile_get_pathname(fp)) != 0, "unexpected file: %s (expected %s)", kbdfile_get_pathname(fp), expect);

	kbdfile_free(fp);
	kbdfile_context_free(ctx);
}
END_TEST

START_TEST(test_9)
{
	struct kbdfile_ctx *ctx = kbdfile_context_new();
	fail_if(!ctx, "unable to create context");

	struct kbdfile *fp = kbdfile_open(ctx, DATADIR "/findfile/test_0/keymaps/i386/qwerty/test0");
	fail_if(fp, "unexpected kbdfile");

	kbdfile_free(fp);
	kbdfile_context_free(ctx);
}
END_TEST

START_TEST(test_10)
{
	struct kbdfile_ctx *ctx = kbdfile_context_new();
	fail_if(!ctx, "unable to create context");

	struct kbdfile *fp = kbdfile_new(ctx);
	fail_if(!fp, "unable to create kbdfile");

	const char *const dirpath[]  = { "", DATADIR "/findfile/test_0/keymaps/**", 0 };
	const char *const suffixes[] = { ".map", 0 };

	const char *expect = DATADIR "/findfile/test_0/keymaps/i386/qwerty/test3.map";

	int rc = 0;

	rc = kbdfile_find((char *)"i386/qwerty/test3", dirpath, suffixes, fp);

	fail_if(rc != 0, "unable to find file");
	fail_if(strcmp(expect, kbdfile_get_pathname(fp)) != 0, "unexpected file: %s (expected %s)", kbdfile_get_pathname(fp), expect);

	kbdfile_free(fp);
	kbdfile_context_free(ctx);
}
END_TEST

START_TEST(test_11)
{
	struct kbdfile_ctx *ctx = kbdfile_context_new();
	fail_if(!ctx, "unable to create context");

	struct kbdfile *fp = kbdfile_new(ctx);
	fail_if(!fp, "unable to create kbdfile");

	const char *const dirpath[]  = { "", DATADIR "/findfile/test_0/keymaps/**", 0 };
	const char *const suffixes[] = { ".map", 0 };

	const char *expect = DATADIR "/findfile/test_0/keymaps/i386/qwerty/test3.map";

	int rc = 0;

	rc = kbdfile_find((char *)"qwerty/test3", dirpath, suffixes, fp);

	fail_if(rc != 0, "unable to find file");
	fail_if(strcmp(expect, kbdfile_get_pathname(fp)) != 0, "unexpected file: %s (expected %s)", kbdfile_get_pathname(fp), expect);

	kbdfile_free(fp);
	kbdfile_context_free(ctx);
}
END_TEST

static Suite *
libfindfile_suite(void)
{
	Suite *s       = suite_create("libkbdfile");
	TCase *tc_core = tcase_create(NULL);

	tcase_add_test(tc_core, test_0);
	tcase_add_test(tc_core, test_1);
	tcase_add_test(tc_core, test_2);
	tcase_add_test(tc_core, test_3);
	tcase_add_test(tc_core, test_4);
	tcase_add_test(tc_core, test_5);
	tcase_add_test(tc_core, test_6);
	tcase_add_test(tc_core, test_7);
	tcase_add_test(tc_core, test_8);
	tcase_add_test(tc_core, test_9);
	tcase_add_test(tc_core, test_10);
	tcase_add_test(tc_core, test_11);

	suite_add_tcase(s, tc_core);
	return s;
}

int main(void)
{
	int number_failed;

	Suite *s    = libfindfile_suite();
	SRunner *sr = srunner_create(s);

	srunner_run_all(sr, CK_NORMAL);

	number_failed = srunner_ntests_failed(sr);
	srunner_free(sr);

	return (number_failed == 0) ? EXIT_SUCCESS : EXIT_FAILURE;
}
