#define _GNU_SOURCE
#include <stdlib.h>

#include "libkeymap-test.h"

/*
 * Pull in the implementation directly so the test can exercise the
 * internal compose-selection helpers without widening the public API.
 */
#include "xkbsupport.c"

static void
expect_rule(struct lk_ctx *ctx, int index, unsigned int diacr,
	    unsigned int base, unsigned int result)
{
	struct lk_kbdiacr rule;

	if (!lk_diacr_exists(ctx, index))
		kbd_error(EXIT_FAILURE, 0, "Missing compose rule %d", index);

	if (lk_get_diacr(ctx, index, &rule) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to read compose rule %d", index);

	if (rule.diacr != diacr || rule.base != base || rule.result != result) {
		kbd_error(EXIT_FAILURE, 0,
			  "Unexpected compose rule %d: got (%u,%u,%u) expected (%u,%u,%u)",
			  index, rule.diacr, rule.base, rule.result, diacr, base, result);
	}
}

static void
test_sequence_dedup_keeps_best_candidate(void)
{
	struct compose_candidate candidates[] = {
		{
			.seq = { 10, 20 },
			.result_sym = 30,
			.diacr = { .diacr = 1, .base = 2, .result = 3 },
			.score = 10,
		},
		{
			.seq = { 40, 50 },
			.result_sym = 60,
			.diacr = { .diacr = 4, .base = 5, .result = 6 },
			.score = 15,
		},
		{
			.seq = { 10, 20 },
			.result_sym = 31,
			.diacr = { .diacr = 7, .base = 8, .result = 9 },
			.score = 20,
		},
	};
	size_t selected;

	selected = xkeymap_select_compose_candidates(candidates, 3);

	if (selected != 2)
		kbd_error(EXIT_FAILURE, 0, "Expected 2 selected candidates, got %zu", selected);

	if (candidates[0].seq[0] != 10 || candidates[0].seq[1] != 20 ||
	    candidates[0].diacr.diacr != 7 || candidates[0].score != 20) {
		kbd_error(EXIT_FAILURE, 0, "Best candidate for the duplicated sequence was not preserved");
	}
}

static void
test_kernel_rule_dedup_happens_after_selection(void)
{
	struct parsed_keymap keymap;
	struct xkeymap xkeymap = { 0 };
	struct compose_candidate candidates[] = {
		{
			.seq = { 10, 20 },
			.result_sym = 30,
			.diacr = { .diacr = 1, .base = 2, .result = 3 },
			.score = 30,
		},
		{
			.seq = { 11, 21 },
			.result_sym = 31,
			.diacr = { .diacr = 1, .base = 2, .result = 3 },
			.score = 25,
		},
		{
			.seq = { 12, 22 },
			.result_sym = 32,
			.diacr = { .diacr = 4, .base = 5, .result = 6 },
			.score = 20,
		},
	};
	size_t total_rules = 0;

	init_test_keymap(&keymap, "xkb-compose-selection");
	xkeymap.ctx = keymap.ctx;

	if (xkeymap_append_compose_candidates(&xkeymap, candidates, 3, &total_rules) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to append compose candidates");

	if (total_rules != 2)
		kbd_error(EXIT_FAILURE, 0, "Expected 2 unique kernel compose rules, got %zu", total_rules);

	expect_rule(keymap.ctx, 0, 1, 2, 3);
	expect_rule(keymap.ctx, 1, 4, 5, 6);

	if (lk_diacr_exists(keymap.ctx, 2))
		kbd_error(EXIT_FAILURE, 0, "Kernel-rule duplicate was appended unexpectedly");

	free_test_keymap(&keymap);
}

static void
test_compose_append_uses_kbd_conversion_rules(void)
{
	struct parsed_keymap keymap;
	struct xkeymap xkeymap = { 0 };
	struct compose_candidate candidate = {
		.seq = { 'a', 'b' },
		.result_sym = 'c',
		.diacr = {
			.diacr = (unsigned int) ('a' ^ 0xf000),
			.base = (unsigned int) ('b' ^ 0xf000),
			.result = (unsigned int) ('c' ^ 0xf000),
		},
		.score = 10,
	};
	size_t total_rules = 0;

	init_test_keymap(&keymap, "xkb-compose-conversion");
	xkeymap.ctx = keymap.ctx;

	if (xkeymap_append_compose_candidates(&xkeymap, &candidate, 1, &total_rules) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to append compose candidate through kbd conversion");

	if (total_rules != 1)
		kbd_error(EXIT_FAILURE, 0, "Expected 1 kernel compose rule, got %zu", total_rules);

	expect_rule(keymap.ctx, 0,
		    (unsigned int) lk_convert_code(keymap.ctx, 'a' ^ 0xf000, TO_8BIT),
		    (unsigned int) lk_convert_code(keymap.ctx, 'b' ^ 0xf000, TO_8BIT),
		    (unsigned int) lk_convert_code(keymap.ctx, 'c' ^ 0xf000, TO_8BIT));

	free_test_keymap(&keymap);
}

static void
test_console_dead_rule_policy_prefers_historic_letter_sets(void)
{
	struct compose_candidate preferred = {
		.seq = { XKB_KEY_dead_caron, 'c' },
		.diacr = { .diacr = 1, .base = 'c', .result = (unsigned int) ('c' ^ 0xf000) },
	};
	struct compose_candidate rejected = {
		.seq = { XKB_KEY_dead_caron, 'q' },
		.diacr = { .diacr = 1, .base = 'q', .result = (unsigned int) ('q' ^ 0xf000) },
	};

	if (!xkeymap_is_preferred_console_dead_rule(&preferred))
		kbd_error(EXIT_FAILURE, 0, "Expected dead_caron + c to be preferred");

	if (xkeymap_is_preferred_console_dead_rule(&rejected))
		kbd_error(EXIT_FAILURE, 0, "Unexpected preferred dead-key rule for dead_caron + q");
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	test_sequence_dedup_keeps_best_candidate();
	test_kernel_rule_dedup_happens_after_selection();
	test_compose_append_uses_kbd_conversion_rules();
	test_console_dead_rule_policy_prefers_historic_letter_sets();

	return EXIT_SUCCESS;
}
