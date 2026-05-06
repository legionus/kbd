#define _GNU_SOURCE

/*
 * xkb-keysym-coverage classifies explicit libxkbcommon keysyms according
 * to the current xkbsupport lookup path: semantic remap, direct libkeymap
 * name lookup, Unicode fallback, hex-name fallback, or unresolved.
 *
 * Build it as a standalone tool linked against libkeymap and
 * libxkbcommon, then run it without arguments to scan the vendored
 * xkbcommon-keysyms.h header:
 *
 *     ./xkb-keysym-coverage
 *
 * Pass an explicit header path to classify a different libxkbcommon
 * keysym header:
 *
 *     ./xkb-keysym-coverage /path/to/xkbcommon-keysyms.h
 */

#include <ctype.h>
#include <limits.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <keymap.h>
#include <xkbcommon/xkbcommon.h>

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

struct builtin_keysym_map {
	xkb_keysym_t sym;
	const char *kbd_name;
};

enum coverage_kind {
	COVERAGE_SEMANTIC = 0,
	COVERAGE_NAME,
	COVERAGE_UNICODE,
	COVERAGE_HEX,
	COVERAGE_UNRESOLVED,
	COVERAGE_COUNT,
};

static const char *coverage_kind_name[] = {
	[COVERAGE_SEMANTIC] = "semantic",
	[COVERAGE_NAME] = "name",
	[COVERAGE_UNICODE] = "unicode",
	[COVERAGE_HEX] = "hex",
	[COVERAGE_UNRESOLVED] = "unresolved",
};

static const struct builtin_keysym_map semantic_map[] = {
	{ XKB_KEY_Shift_L,		"Shift" },
	{ XKB_KEY_Shift_R,		"Shift" },
	{ XKB_KEY_Control_L,		"Control" },
	{ XKB_KEY_Control_R,		"Control" },
	{ XKB_KEY_Alt_L,		"Alt" },
	{ XKB_KEY_Alt_R,		"Alt" },
	{ XKB_KEY_Meta_L,		"Alt" },
	{ XKB_KEY_Meta_R,		"Alt" },
	{ XKB_KEY_Super_L,		"Alt" },
	{ XKB_KEY_Super_R,		"Alt" },
	{ XKB_KEY_Hyper_L,		"Alt" },
	{ XKB_KEY_Hyper_R,		"Alt" },
	{ XKB_KEY_Mode_switch,		"AltGr" },
	{ XKB_KEY_Multi_key,		"Compose" },
	{ XKB_KEY_Sys_Req,		"Last_Console" },
	{ XKB_KEY_Print,		"Control_backslash" },
	{ XKB_KEY_Delete,		"Remove" },
	{ XKB_KEY_ISO_Level2_Latch,	"Shift" },
	{ XKB_KEY_ISO_Lock,		"Caps_Lock" },
	{ XKB_KEY_ISO_Level3_Shift,	"AltGr" },
	{ XKB_KEY_ISO_Level3_Latch,	"AltGr" },
	{ XKB_KEY_ISO_Level3_Lock,	"AltGr_Lock" },
	{ XKB_KEY_ISO_Level5_Shift,	"AltGr" },
	{ XKB_KEY_ISO_Level5_Latch,	"AltGr" },
	{ XKB_KEY_ISO_Level5_Lock,	"AltGr_Lock" },
	{ XKB_KEY_ISO_Group_Shift,	"ShiftL" },
	{ XKB_KEY_ISO_Group_Latch,	"ShiftL" },
	{ XKB_KEY_ISO_Group_Lock,	"ShiftL_Lock" },
	{ XKB_KEY_ISO_First_Group,	"ShiftL_Lock" },
	{ XKB_KEY_ISO_First_Group_Lock,	"ShiftL_Lock" },
	{ XKB_KEY_ISO_Last_Group,	"ShiftL_Lock" },
	{ XKB_KEY_ISO_Last_Group_Lock,	"ShiftL_Lock" },
	{ XKB_KEY_ISO_Next_Group,	"ShiftL_Lock" },
	{ XKB_KEY_ISO_Next_Group_Lock,	"ShiftL_Lock" },
	{ XKB_KEY_ISO_Prev_Group,	"ShiftL_Lock" },
	{ XKB_KEY_ISO_Prev_Group_Lock,	"ShiftL_Lock" },
	{ XKB_KEY_ISO_Left_Tab,		"Meta_Tab" },
	{ XKB_KEY_KP_Insert,		"KP_0" },
	{ XKB_KEY_KP_End,		"KP_1" },
	{ XKB_KEY_KP_Down,		"KP_2" },
	{ XKB_KEY_KP_Next,		"KP_3" },
	{ XKB_KEY_KP_Left,		"KP_4" },
	{ XKB_KEY_KP_Begin,		"KP_5" },
	{ XKB_KEY_KP_Right,		"KP_6" },
	{ XKB_KEY_KP_Home,		"KP_7" },
	{ XKB_KEY_KP_Up,		"KP_8" },
	{ XKB_KEY_KP_Prior,		"KP_9" },
	{ XKB_KEY_KP_Delete,		"KP_Period" },
	{ XKB_KEY_KP_Decimal,		"KP_Comma" },
};

static int validate_code(int code)
{
	if (code <= 0 || code > USHRT_MAX)
		return -1;

	return code;
}

static int lookup_builtin_name(struct lk_ctx *ctx, const char *name)
{
	if (!lk_valid_ksym(ctx, name, TO_UNICODE))
		return -1;

	return lk_ksym_to_unicode(ctx, name);
}

static int parse_hexcode(struct lk_ctx *ctx, const char *symname)
{
	int value;
	size_t len = strlen(symname);

	if (len >= 4 && symname[0] == '0' && symname[1] == 'x') {
		value = (int) strtol(symname + 2, NULL, 16);
		return lk_convert_code(ctx, value, TO_UNICODE);
	}

	if (len >= 5 && symname[0] == 'U' &&
	    isxdigit((unsigned char) symname[1]) &&
	    isxdigit((unsigned char) symname[2]) &&
	    isxdigit((unsigned char) symname[3]) &&
	    isxdigit((unsigned char) symname[4])) {
		value = (int) strtol(symname + 1, NULL, 16);
		return lk_convert_code(ctx, value ^ 0xf000, TO_UNICODE);
	}

	return 0;
}

static int get_code_from_semantic_keysym(struct lk_ctx *ctx, xkb_keysym_t symbol)
{
	char console[16];
	size_t i;

	for (i = 0; i < ARRAY_SIZE(semantic_map); i++) {
		if (semantic_map[i].sym == symbol)
			return validate_code(lookup_builtin_name(ctx, semantic_map[i].kbd_name));
	}

	if (symbol >= XKB_KEY_XF86Switch_VT_1 && symbol <= XKB_KEY_XF86Switch_VT_12) {
		if (snprintf(console, sizeof(console), "Console_%u",
			     (unsigned int) (symbol - XKB_KEY_XF86Switch_VT_1 + 1)) >=
		    (int) sizeof(console))
			return -1;

		return validate_code(lookup_builtin_name(ctx, console));
	}

	return -1;
}

static int get_code_from_name(struct lk_ctx *ctx, xkb_keysym_t symbol)
{
	int ret;
	char symbuf[BUFSIZ];

	symbuf[0] = '\0';
	ret = xkb_keysym_get_name(symbol, symbuf, sizeof(symbuf));
	if (ret < 0 || (size_t) ret >= sizeof(symbuf))
		return -1;

	if (!lk_valid_ksym(ctx, symbuf, TO_UNICODE))
		return -1;

	return validate_code(lk_ksym_to_unicode(ctx, symbuf));
}

static int get_code_from_unicode(xkb_keysym_t symbol)
{
	uint32_t unicode = xkb_keysym_to_utf32(symbol);

	if (unicode < 0x20 || unicode == 0x7f)
		return -1;

	return validate_code((int) (unicode ^ 0xf000));
}

static int get_code_from_hex(struct lk_ctx *ctx, xkb_keysym_t symbol)
{
	int ret;
	char symbuf[BUFSIZ];

	symbuf[0] = '\0';
	ret = xkb_keysym_get_name(symbol, symbuf, sizeof(symbuf));
	if (ret < 0 || (size_t) ret >= sizeof(symbuf))
		return -1;

	return validate_code(parse_hexcode(ctx, symbuf));
}

static int classify_keysym(struct lk_ctx *ctx, xkb_keysym_t symbol, enum coverage_kind *kind)
{
	int code;

	code = get_code_from_semantic_keysym(ctx, symbol);
	if (code >= 0) {
		*kind = COVERAGE_SEMANTIC;
		return code;
	}

	code = get_code_from_name(ctx, symbol);
	if (code >= 0) {
		*kind = COVERAGE_NAME;
		return code;
	}

	code = get_code_from_unicode(symbol);
	if (code >= 0) {
		*kind = COVERAGE_UNICODE;
		return code;
	}

	code = get_code_from_hex(ctx, symbol);
	if (code >= 0) {
		*kind = COVERAGE_HEX;
		return code;
	}

	*kind = COVERAGE_UNRESOLVED;
	return -1;
}

static void print_code(FILE *out, struct lk_ctx *ctx, int code)
{
	char *name;

	if (code < 0) {
		fputs("<none>", out);
		return;
	}

	name = lk_code_to_ksym(ctx, code);
	if (name) {
		fprintf(out, "0x%x:%s", code, name);
		free(name);
		return;
	}

	fprintf(out, "0x%x", code);
}

static void print_usage(FILE *out, const char *progname)
{
	fprintf(out,
		"usage: %s [xkbcommon-keysyms.h]\n"
		"\n"
		"Classify XKB keysyms according to the current xkbsupport lookup path.\n"
		"If no header path is given, use the vendored libxkbcommon header.\n",
		progname);
}

int main(int argc, char **argv)
{
	struct lk_ctx *ctx;
	unsigned int counts[COVERAGE_COUNT] = { 0 };
	const char *keysym_header =
		"external/libxkbcommon/include/xkbcommon/xkbcommon-keysyms.h";
	unsigned int printed = 0;
	FILE *fp;
	char line[512];

	if (argc > 2) {
		print_usage(stderr, argv[0]);
		return EXIT_FAILURE;
	}

	if (argc == 2) {
		if (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")) {
			print_usage(stdout, argv[0]);
			return EXIT_SUCCESS;
		}
		keysym_header = argv[1];
	}

	ctx = lk_init();
	if (!ctx) {
		fprintf(stderr, "failed to init libkeymap context\n");
		return EXIT_FAILURE;
	}

	fp = fopen(keysym_header, "r");
	if (!fp) {
		perror(keysym_header);
		lk_free(ctx);
		return EXIT_FAILURE;
	}

	while (fgets(line, sizeof(line), fp)) {
		char name[256];
		unsigned int value;
		enum coverage_kind kind;
		int code;

		if (sscanf(line, "#define XKB_KEY_%255s 0x%x", name, &value) != 2)
			continue;

		code = classify_keysym(ctx, (xkb_keysym_t) value, &kind);
		counts[kind]++;

		if (kind == COVERAGE_UNRESOLVED && printed < 80) {
			printf("unresolved\t%s\t", name);
			print_code(stdout, ctx, code);
			fputc('\n', stdout);
			printed++;
		}
	}

	puts("");
	puts("summary");
	for (size_t i = 0; i < ARRAY_SIZE(counts); i++)
		printf("%-12s %u\n", coverage_kind_name[i], counts[i]);
	printf("%-12s %u\n", "total",
	       counts[0] + counts[1] + counts[2] + counts[3] + counts[4]);
	printf("%-12s %u\n", "printed", printed);

	fclose(fp);
	lk_free(ctx);
	return EXIT_SUCCESS;
}
