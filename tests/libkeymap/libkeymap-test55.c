#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include <stdlib.h>

#include "libkeymap-test.h"

/*
 * Pull in the implementation directly so the test can exercise the
 * internal compose-selection helpers without widening the public API.
 */
#include "xkbsupport.c"

static void
expect_rule(struct lk_ctx *ctx, unsigned int index, unsigned int diacr,
	    unsigned int base, unsigned int result)
{
	struct lk_kbdiacr rule;

	if (!lk_diacr_exists(ctx, index))
		kbd_error(EXIT_FAILURE, 0, "Missing compose rule %u", index);

	if (lk_get_diacr(ctx, index, &rule) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to read compose rule %u", index);

	if (rule.diacr != diacr || rule.base != base || rule.result != result) {
		kbd_error(EXIT_FAILURE, 0,
			  "Unexpected compose rule %u: got (%u,%u,%u) expected (%u,%u,%u)",
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
test_negative_compose_scores(void)
{
	struct compose_candidate candidates[] = {
		{
			.seq = { XKB_KEY_a, XKB_KEY_space },
			.result_sym = XKB_KEY_ccaron,
			.diacr = { .diacr = 'a', .base = ' ', .result = 0x010d ^ 0xf000 },
		},
		{
			.seq = { XKB_KEY_b, XKB_KEY_space },
			.result_sym = XKB_KEY_ccaron,
			.diacr = { .diacr = 'b', .base = ' ', .result = 0x010d ^ 0xf000 },
		},
		{
			.seq = { XKB_KEY_c, XKB_KEY_space },
			.result_sym = XKB_KEY_ellipsis,
			.diacr = { .diacr = 'c', .base = ' ', .result = 0x2026 ^ 0xf000 },
		},
		{
			.seq = { XKB_KEY_a, XKB_KEY_space },
			.result_sym = XKB_KEY_aacute,
			.diacr = { .diacr = 'a', .base = ' ', .result = 0x00e1 ^ 0xf000 },
		},
		{
			.seq = { XKB_KEY_dead_acute, XKB_KEY_a },
			.result_sym = XKB_KEY_aacute,
			.diacr = { .diacr = '\'', .base = 'a', .result = 0x00e1 ^ 0xf000 },
		},
	};
	size_t selected;

	for (size_t i = 0; i < ARRAY_SIZE(candidates); i++)
		candidates[i].score = xkeymap_score_compose_candidate(&candidates[i]);
	selected = xkeymap_select_compose_candidates(candidates, ARRAY_SIZE(candidates));
	if (selected != 4 ||
	    candidates[0].seq[0] != XKB_KEY_dead_acute ||
	    candidates[1].seq[0] != XKB_KEY_a ||
	    candidates[1].result_sym != XKB_KEY_aacute ||
	    candidates[2].seq[0] != XKB_KEY_b ||
	    candidates[3].seq[0] != XKB_KEY_c)
		kbd_error(EXIT_FAILURE, 0, "Negative compose scores displaced higher-priority rules");
}

static void
test_kernel_rule_dedup_happens_after_selection(int unicode)
{
	struct parsed_keymap keymap;
	struct xkeymap xkeymap = { 0 };
	struct compose_candidate candidates[] = {
		{
			.seq = { 10, 20 },
			.result_sym = 30,
			.diacr = { .diacr = 'a', .base = 'b', .result = 3 },
			.score = 30,
		},
		{
			.seq = { 11, 21 },
			.result_sym = 31,
			.diacr = { .diacr = 0xf061, .base = 0xf062, .result = 7 },
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
	if (unicode && lk_set_parser_flags(keymap.ctx, LK_FLAG_PREFER_UNICODE) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to enable Unicode conversion");
	xkeymap.ctx = keymap.ctx;

	if (xkeymap_append_compose_candidates(&xkeymap, candidates, 3, &total_rules) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to append compose candidates");

	if (total_rules != 2)
		kbd_error(EXIT_FAILURE, 0, "Expected 2 unique kernel compose rules, got %zu", total_rules);

	expect_rule(keymap.ctx, 0, 'a', 'b', 3);
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

static void
test_dead_key_compose_inputs(int unicode)
{
	static const char compose[] =
		"<dead_acute> <a> : \"á\" aacute\n"
		"<dead_acute> <dead_acute> : \"´\" acute\n";
	struct parsed_keymap keymap;
	struct xkeymap xkeymap = { 0 };
	int direction = unicode ? TO_UNICODE : TO_8BIT;
	int found_letter = 0, found_dead = 0;

	init_test_keymap(&keymap, "xkb-dead-compose");
	xkeymap.ctx = keymap.ctx;
	if (unicode && lk_set_parser_flags(keymap.ctx, LK_FLAG_PREFER_UNICODE) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to enable Unicode conversion");
	xkeymap.xkb = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if (!xkeymap.xkb)
		kbd_error(EXIT_FAILURE, 0, "Unable to create XKB context");
	xkeymap.compose = xkb_compose_table_new_from_buffer(xkeymap.xkb, compose,
							    sizeof(compose) - 1, "C",
							    XKB_COMPOSE_FORMAT_TEXT_V1,
							    XKB_COMPOSE_COMPILE_NO_FLAGS);
	if (!xkeymap.compose)
		kbd_error(EXIT_FAILURE, 0, "Unable to compile dead-key compose rules");

	remember_reachable_sym(&xkeymap, XKB_KEY_dead_acute,
			       xkeymap_get_code(&xkeymap, XKB_KEY_dead_acute));
	remember_reachable_sym(&xkeymap, XKB_KEY_a, xkeymap_get_code(&xkeymap, XKB_KEY_a));
	if (xkeymap_compose(&xkeymap) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to import dead-key compose rules");

	for (unsigned int i = 0; lk_diacr_exists(keymap.ctx, i); i++) {
		struct lk_kbdiacr rule;

		if (lk_get_diacr(keymap.ctx, i, &rule) != 0)
			kbd_error(EXIT_FAILURE, 0, "Unable to read compose rule");
		if (rule.base == (unsigned int) lk_convert_code(keymap.ctx, 'a', direction)) {
			expect_rule(keymap.ctx, i,
				    (unsigned int) lk_convert_code(keymap.ctx, '\'', direction),
				    rule.base,
				    (unsigned int) lk_convert_code(keymap.ctx, 0xe1 ^ 0xf000, direction));
			found_letter++;
		} else {
			expect_rule(keymap.ctx, i,
				    (unsigned int) lk_convert_code(keymap.ctx, '\'', direction),
				    (unsigned int) lk_convert_code(keymap.ctx, '\'', direction),
				    (unsigned int) lk_convert_code(keymap.ctx, 0xb4 ^ 0xf000, direction));
			found_dead++;
		}
	}
	if (found_letter != 1 || found_dead != 1)
		kbd_error(EXIT_FAILURE, 0, "Missing or duplicate dead-key compose rule");

	tdestroy(xkeymap.reachable_syms, free);
	xkb_compose_table_unref(xkeymap.compose);
	xkb_context_unref(xkeymap.xkb);
	free_test_keymap(&keymap);
}

static void
test_distinct_dead_key_actions(void)
{
	static const struct {
		xkb_keysym_t sym;
		int action;
		unsigned int accent, base, result;
	} cases[] = {
		{ XKB_KEY_dead_circumflex,  K_DCIRCM,   '^', 'c', 0x0109 },
		{ XKB_KEY_dead_caron,       K_DCARON,   'c', 'c', 0x010d },
		{ XKB_KEY_dead_tilde,       K_DTILDE,   '~', 'a', 0x00e3 },
		{ XKB_KEY_dead_breve,       K_DBREVE,   'U', 'a', 0x0103 },
		{ XKB_KEY_dead_breve,       K_DBREVE,   'U', 'U', 0x016c },
		{ XKB_KEY_dead_doubleacute, K_DDBACUTE, '=', 'o', 0x0151 },
		{ XKB_KEY_dead_cedilla,     K_DCEDIL,   ',', 'c', 0x00e7 },
		{ XKB_KEY_dead_ogonek,      K_DOGONEK,  'k', 'a', 0x0105 },
	};
	static const char compose[] =
		"<dead_circumflex> <c> : ccircumflex\n"
		"<dead_caron> <c> : ccaron\n"
		"<dead_caron> <dead_caron> : U02C7\n"
		"<dead_tilde> <a> : atilde\n"
		"<dead_breve> <a> : abreve\n"
		"<dead_breve> <U> : Ubreve\n"
		"<dead_breve> <dead_breve> : U02D8\n"
		"<dead_doubleacute> <o> : odoubleacute\n"
		"<dead_cedilla> <c> : ccedilla\n"
		"<dead_ogonek> <a> : aogonek\n";
	struct parsed_keymap keymap;
	struct xkeymap xkeymap = { 0 };
	unsigned int found = 0;

	init_test_keymap(&keymap, "xkb-distinct-dead-keys");
	xkeymap.ctx = keymap.ctx;
	if (lk_set_parser_flags(keymap.ctx, LK_FLAG_PREFER_UNICODE) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to enable Unicode conversion");
	xkeymap.xkb = xkb_context_new(XKB_CONTEXT_NO_FLAGS);
	if (!xkeymap.xkb)
		kbd_error(EXIT_FAILURE, 0, "Unable to create XKB context");
	xkeymap.compose = xkb_compose_table_new_from_buffer(xkeymap.xkb, compose,
							    sizeof(compose) - 1, "C", XKB_COMPOSE_FORMAT_TEXT_V1,
							    XKB_COMPOSE_COMPILE_NO_FLAGS);
	if (!xkeymap.compose)
		kbd_error(EXIT_FAILURE, 0, "Unable to compile distinct dead-key rules");

	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		int code = xkeymap_get_code(&xkeymap, cases[i].sym);

		if (code != cases[i].action)
			kbd_error(EXIT_FAILURE, 0, "Dead keysym 0x%x: got action 0x%x, expected 0x%x",
				  cases[i].sym, code, cases[i].action);
		remember_reachable_sym(&xkeymap, cases[i].sym, code);
		remember_reachable_sym(&xkeymap, cases[i].base,
				       xkeymap_get_code(&xkeymap, cases[i].base));
	}
	if (xkeymap_compose(&xkeymap) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to import distinct dead-key rules");

	for (unsigned int i = 0; lk_diacr_exists(keymap.ctx, i); i++) {
		struct lk_kbdiacr rule;
		size_t j;

		if (lk_get_diacr(keymap.ctx, i, &rule) != 0)
			kbd_error(EXIT_FAILURE, 0, "Unable to read compose rule");
		for (j = 0; j < ARRAY_SIZE(cases); j++) {
			if (rule.diacr != cases[j].accent || rule.base != cases[j].base)
				continue;
			expect_rule(keymap.ctx, i, cases[j].accent, cases[j].base, cases[j].result);
			if (found & (1U << j))
				kbd_error(EXIT_FAILURE, 0, "Duplicate dead-key compose input");
			found |= 1U << j;
			break;
		}
		if (j == ARRAY_SIZE(cases))
			kbd_error(EXIT_FAILURE, 0, "Unexpected dead-key compose input");
	}
	if (found != (1U << ARRAY_SIZE(cases)) - 1)
		kbd_error(EXIT_FAILURE, 0, "Missing distinct dead-key compose rule");

	tdestroy(xkeymap.reachable_syms, free);
	xkb_compose_table_unref(xkeymap.compose);
	xkb_context_unref(xkeymap.xkb);
	free_test_keymap(&keymap);
}

static void
test_named_character_codes(const char *charset)
{
	static const struct {
		xkb_keysym_t sym;
		unsigned int unicode;
	} cases[] = {
		{ XKB_KEY_ccaron, 0x010d },
		{ XKB_KEY_breve,  0x02d8 },
		{ XKB_KEY_caron,  0x02c7 },
		{ XKB_KEY_ogonek, 0x02db },
		{ XKB_KEY_eacute, 0x00e9 },
	};
	struct parsed_keymap keymap;
	struct xkeymap xkeymap = { 0 };

	init_test_keymap(&keymap, "xkb-named-characters");
	xkeymap.ctx = keymap.ctx;
	if (lk_set_charset(keymap.ctx, charset) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to select charset %s", charset);
	for (size_t i = 0; i < ARRAY_SIZE(cases); i++) {
		int code = xkeymap_get_code(&xkeymap, cases[i].sym);

		if (code < 0 || lk_convert_code(keymap.ctx, code, TO_UNICODE) != (int) cases[i].unicode)
			kbd_error(EXIT_FAILURE, 0, "Named XKB character changed meaning in %s", charset);
	}
	/* Characters already represented correctly keep their existing type. */
	if (xkeymap_get_code(&xkeymap, XKB_KEY_eacute) != 0xe9 ||
	    xkeymap_get_code(&xkeymap, XKB_KEY_a) != 'a' ||
	    xkeymap_get_code(&xkeymap, XKB_KEY_BackSpace) != 0x08 ||
	    xkeymap_get_code(&xkeymap, XKB_KEY_dead_acute) != K_DACUTE ||
	    xkeymap_get_code(&xkeymap, XKB_KEY_KP_1) != K_P1 ||
	    xkeymap_get_code(&xkeymap, XKB_KEY_ISO_Left_Tab) != K(KT_META, 9))
		kbd_error(EXIT_FAILURE, 0, "Named XKB symbol type changed");
	free_test_keymap(&keymap);
}

static void
test_unicode_action_collisions(void)
{
	static const char keymap_text[] =
		"xkb_keymap {"
		" xkb_keycodes { minimum=8; maximum=10; <BAD>=9; <GOOD>=10; };"
		" xkb_types { type \"ONE_LEVEL\" { modifiers=None; map[None]=Level1; }; };"
		" xkb_compatibility {};"
		" xkb_symbols { key <BAD> { [ UF701 ] }; key <GOOD> { [ UEFFF ] }; };"
		"};";
	static const uint32_t supplementary[] = { 0x10000, 0x1f600, 0x10ffff };
	struct parsed_keymap keymap;
	struct xkeymap xkeymap = { 0 };

	init_test_keymap(&keymap, "xkb-unicode-action-collisions");
	xkeymap.ctx = keymap.ctx;

	/* Every value in this range would overlap a kernel symbol type. */
	for (uint32_t unicode = 0xf000; unicode <= 0xffff; unicode++) {
		if (xkeymap_get_code(&xkeymap, 0x01000000 | unicode) >= 0)
			kbd_error(EXIT_FAILURE, 0, "Unrepresentable U+%04X became a console binding", unicode);
	}

	for (size_t i = 0; i < ARRAY_SIZE(supplementary); i++) {
		if (xkeymap_get_code(&xkeymap, 0x01000000 | supplementary[i]) >= 0)
			kbd_error(EXIT_FAILURE, 0, "Supplementary Unicode became a console binding");
	}

	if (xkeymap_get_code(&xkeymap, 0x0100e000) != (0xe000 ^ 0xf000) ||
	    xkeymap_get_code(&xkeymap, 0x0100efff) != (0xefff ^ 0xf000) ||
	    xkeymap_get_code(&xkeymap, XKB_KEY_ISO_Level3_Shift) != K_ALTGR)
		kbd_error(EXIT_FAILURE, 0, "Unicode boundary or real AltGr binding changed");

	xkeymap.xkb = xkb_context_new(XKB_CONTEXT_NO_FLAGS);

	if (!xkeymap.xkb)
		kbd_error(EXIT_FAILURE, 0, "Unable to create XKB context");

	xkeymap.keymap = xkb_keymap_new_from_string(xkeymap.xkb, keymap_text,
						    XKB_KEYMAP_FORMAT_TEXT_V1, XKB_KEYMAP_COMPILE_NO_FLAGS);
	if (!xkeymap.keymap)
		kbd_error(EXIT_FAILURE, 0, "Unable to compile Unicode boundary keymap");

	if (xkeymap_walk(&xkeymap) != 0)
		kbd_error(EXIT_FAILURE, 0, "Unable to convert Unicode boundary keymap");

	for (unsigned int table = 0; table < MAX_NR_KEYMAPS; table++) {
		if (!lk_map_exists(keymap.ctx, table))
			continue;

		if (lk_get_key(keymap.ctx, table, 1) != K_HOLE ||
		    lk_get_key(keymap.ctx, table, 2) != (0xefff ^ 0xf000))
			kbd_error(EXIT_FAILURE, 0, "Unicode character became an action in table %u", table);
	}

	tdestroy(xkeymap.reachable_syms, free);
	xkb_keymap_unref(xkeymap.keymap);
	xkb_context_unref(xkeymap.xkb);
	free_test_keymap(&keymap);
}

int
main(int argc KBD_ATTR_UNUSED, char **argv KBD_ATTR_UNUSED)
{
	test_sequence_dedup_keeps_best_candidate();
	test_negative_compose_scores();
	test_kernel_rule_dedup_happens_after_selection(0);
	test_kernel_rule_dedup_happens_after_selection(1);
	test_compose_append_uses_kbd_conversion_rules();
	test_console_dead_rule_policy_prefers_historic_letter_sets();
	test_dead_key_compose_inputs(0);
	test_dead_key_compose_inputs(1);
	test_distinct_dead_key_actions();
	test_named_character_codes("iso-8859-1");
	test_named_character_codes("iso-8859-2");
	test_unicode_action_collisions();

	return EXIT_SUCCESS;
}
