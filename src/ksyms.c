#include <linux/keyboard.h>
#include <stdio.h>
#include <string.h>
#include "ksyms.h"
#include "nls.h"

static char *latin1_syms[] = {
	"nul",
	"Control_a",
	"Control_b",
	"Control_c",
	"Control_d",
	"Control_e",
	"Control_f",
	"Control_g",
	"BackSpace",
	"Tab",
	"Linefeed",
	"Control_k",
	"Control_l",
	"Control_m",
	"Control_n",
	"Control_o",
	"Control_p",
	"Control_q",
	"Control_r",
	"Control_s",
	"Control_t",
	"Control_u",
	"Control_v",
	"Control_w",
	"Control_x",
	"Control_y",
	"Control_z",
	"Escape",
	"Control_backslash",
	"Control_bracketright",
	"Control_asciicircum",
	"Control_underscore",
	"space",
	"exclam",
	"quotedbl",
	"numbersign",
	"dollar",
	"percent",
	"ampersand",
	"apostrophe",
	"parenleft",
	"parenright",
	"asterisk",
	"plus",
	"comma",
	"minus",
	"period",
	"slash",
	"zero",
	"one",
	"two",
	"three",
	"four",
	"five",
	"six",
	"seven",
	"eight",
	"nine",
	"colon",
	"semicolon",
	"less",
	"equal",
	"greater",
	"question",
	"at",
	"A",
	"B",
	"C",
	"D",
	"E",
	"F",
	"G",
	"H",
	"I",
	"J",
	"K",
	"L",
	"M",
	"N",
	"O",
	"P",
	"Q",
	"R",
	"S",
	"T",
	"U",
	"V",
	"W",
	"X",
	"Y",
	"Z",
	"bracketleft",
	"backslash",
	"bracketright",
	"asciicircum",
	"underscore",
	"grave",
	"a",
	"b",
	"c",
	"d",
	"e",
	"f",
	"g",
	"h",
	"i",
	"j",
	"k",
	"l",
	"m",
	"n",
	"o",
	"p",
	"q",
	"r",
	"s",
	"t",
	"u",
	"v",
	"w",
	"x",
	"y",
	"z",
	"braceleft",
	"bar",
	"braceright",
	"asciitilde",
	"Delete",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"",
	"nobreakspace",
	"exclamdown",
	"cent",
	"sterling",
	"currency",
	"yen",
	"brokenbar",
	"section",
	"diaeresis",
	"copyright",
	"ordfeminine",
	"guillemotleft",
	"notsign",
	"hyphen",
	"registered",
	"macron",
	"degree",
	"plusminus",
	"twosuperior",
	"threesuperior",
	"acute",
	"mu",
	"paragraph",
	"periodcentered",
	"cedilla",
	"onesuperior",
	"masculine",
	"guillemotright",
	"onequarter",
	"onehalf",
	"threequarters",
	"questiondown",
	"Agrave",
	"Aacute",
	"Acircumflex",
	"Atilde",
	"Adiaeresis",
	"Aring",
	"AE",
	"Ccedilla",
	"Egrave",
	"Eacute",
	"Ecircumflex",
	"Ediaeresis",
	"Igrave",
	"Iacute",
	"Icircumflex",
	"Idiaeresis",
	"ETH",
	"Ntilde",
	"Ograve",
	"Oacute",
	"Ocircumflex",
	"Otilde",
	"Odiaeresis",
	"multiply",
	"Ooblique",
	"Ugrave",
	"Uacute",
	"Ucircumflex",
	"Udiaeresis",
	"Yacute",
	"THORN",
	"ssharp",
	"agrave",
	"aacute",
	"acircumflex",
	"atilde",
	"adiaeresis",
	"aring",
	"ae",
	"ccedilla",
	"egrave",
	"eacute",
	"ecircumflex",
	"ediaeresis",
	"igrave",
	"iacute",
	"icircumflex",
	"idiaeresis",
	"eth",
	"ntilde",
	"ograve",
	"oacute",
	"ocircumflex",
	"otilde",
	"odiaeresis",
	"division",
	"oslash",
	"ugrave",
	"uacute",
	"ucircumflex",
	"udiaeresis",
	"yacute",
	"thorn",
	"ydiaeresis"
};

static char *latin2_syms[] = {
    "", "Aogonek", "breve", "Lstroke", "", "Lcaron", "Sacute", "",
    "", "Scaron", "Scedilla", "Tcaron", "Zacute", "", "Zcaron", "Zabovedot",
    "", "aogonek", "ogonek", "lstroke", "", "lcaron", "sacute", "caron",
    "", "scaron", "scedilla", "tcaron", "zacute", "doubleacute", "zcaron", "zabovedot",
    "Racute", "", "", "Abreve", "", "Lacute", "Cacute", "",
    "Ccaron", "", "Eogonek", "", "Ecaron", "", "", "Dcaron",
    "Dstroke", "Nacute", "Ncaron", "", "", "Odoubleacute", "", "",
    "Rcaron", "Uring", "", "Udoubleacute", "", "", "Tcedilla", "",
    "racute", "", "", "abreve", "", "lacute", "cacute", "",
    "ccaron", "", "eogonek", "", "ecaron", "", "", "dcaron",
    "dstroke", "nacute", "ncaron", "", "", "odoubleacute", "", "",
    "rcaron", "uring", "", "udoubleacute", "", "", "tcedilla", "abovedot"
};

static char *mazovia_syms[] = { /* as specified by Wlodek Bzyl <matwb@univ.gda.pl> */
    "", "", "", "", "", "", "aogonek", "", /* 8x */
    "", "", "", "", "", "cacute", "", "Aogonek",
    "Eogonek", "eogonek", "lstroke", "", "", "Cacute", "", "", /* 9x */
    "Sacute", "", "", "", "Lstroke", "", "sacute", "",
    "Zacute", "Zabovedot", "oacute", "Oacute",
    "nacute", "Nacute", "zacute", "zabovedot", /* ax */
    "", "", "", "", "", "", "guillemotleft", "guillemotright",
    "", "", "", "", "", "", "", "", /* bx */
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", /* cx */
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", /* dx */
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", /* ex */
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "", /* fx */
    "", "", "", "", "", "", "", "quotedblbase"
};

static char *latin3_syms[] = {
    "", "Hstroke", "", "", "", "", "Hcircumflex", "",
    "", "Iabovedot", "", "Gbreve", "Jcircumflex", "", "", "",
    "", "hstroke", "", "", "", "", "hcircumflex", "",
    "", "idotless", "", "gbreve", "jcircumflex", "", "", "",
    "", "", "", "", "", "Cabovedot", "Ccircumflex", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "Gabovedot", "", "",
    "Gcircumflex", "", "", "", "", "Ubreve", "Scircumflex", "",
    "", "", "", "", "", "cabovedot", "ccircumflex", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "gabovedot", "", "",
    "gcircumflex", "", "", "", "", "ubreve", "scircumflex", ""
};

static char *latin4_syms[] = {
    "", "", "kra", "Rcedilla", "", "Itilde", "Lcedilla", "",
    "", "", "Emacron", "Gcedilla", "Tslash", "", "", "",
    "", "", "", "rcedilla", "", "itilde", "lcedilla", "",
    "", "", "emacron", "gcedilla", "tslash", "ENG", "", "eng",
    "Amacron", "", "", "", "", "", "", "Iogonek",
    "", "", "", "", "Eabovedot", "", "", "Imacron",
    "", "Ncedilla", "Omacron", "Kcedilla", "", "", "", "",
    "", "Uogonek", "", "", "", "Utilde", "Umacron", "",
    "amacron", "", "", "", "", "", "", "iogonek",
    "", "", "", "", "eabovedot", "", "", "imacron",
    "", "ncedilla", "omacron", "kcedilla", "", "", "", "",
    "", "uogonek", "", "", "", "utilde", "umacron", ""
};

static char *iso_8859_5_syms[] = { /* 160-255 */
    "no-break_space",					/* 0240 */
    "cyrillic_capital_letter_io",
    "serbocroatian_cyrillic_capital_letter_dje",
    "macedonian_cyrillic_capital_letter_gje",
    "ukrainian_cyrillic_capital_letter_ie",
    "macedonian_cyrillic_capital_letter_dze",
    "ukrainian_cyrillic_capital_letter_i",
    "ukrainian_cyrillic_capital_letter_yi",
    "cyrillic_capital_letter_je", 			/* 0250 */
    "cyrillic_capital_letter_lje",
    "cyrillic_capital_letter_nje",
    "serbocroatian_cyrillic_capital_letter_chje",
    "macedonian_cyrillic_capital_letter_kje",
    "soft_hyphen",
    "bielorussian_cyrillic_capital_letter_short_u",
    "cyrillic_capital_letter_dze",
    "cyrillic_capital_letter_a", 			/* 0260 */
    "cyrillic_capital_letter_be",
    "cyrillic_capital_letter_ve",
    "cyrillic_capital_letter_ghe",
    "cyrillic_capital_letter_de",
    "cyrillic_capital_letter_ie",
    "cyrillic_capital_letter_zhe",
    "cyrillic_capital_letter_ze",
    "cyrillic_capital_letter_i", 			/* 0270 */
    "cyrillic_capital_letter_short_i",
    "cyrillic_capital_letter_ka",
    "cyrillic_capital_letter_el",
    "cyrillic_capital_letter_em",
    "cyrillic_capital_letter_en",
    "cyrillic_capital_letter_o",
    "cyrillic_capital_letter_pe",
    "cyrillic_capital_letter_er", 			/* 0300 */
    "cyrillic_capital_letter_es",
    "cyrillic_capital_letter_te",
    "cyrillic_capital_letter_u",
    "cyrillic_capital_letter_ef",
    "cyrillic_capital_letter_ha",
    "cyrillic_capital_letter_tse",
    "cyrillic_capital_letter_che",
    "cyrillic_capital_letter_sha", 			/* 0310 */
    "cyrillic_capital_letter_shcha",
    "cyrillic_capital_hard_sign",
    "cyrillic_capital_letter_yeru",
    "cyrillic_capital_soft_sign",
    "cyrillic_capital_letter_e",
    "cyrillic_capital_letter_yu",
    "cyrillic_capital_letter_ya",
    "cyrillic_small_letter_a",				/* 0320 */
    "cyrillic_small_letter_be",
    "cyrillic_small_letter_ve",
    "cyrillic_small_letter_ghe",
    "cyrillic_small_letter_de",
    "cyrillic_small_letter_ie",
    "cyrillic_small_letter_zhe",
    "cyrillic_small_letter_ze",
    "cyrillic_small_letter_i",				/* 0330 */
    "cyrillic_small_letter_short_i",
    "cyrillic_small_letter_ka",
    "cyrillic_small_letter_el",
    "cyrillic_small_letter_em",
    "cyrillic_small_letter_en",
    "cyrillic_small_letter_o",
    "cyrillic_small_letter_pe",
    "cyrillic_small_letter_er",				/* 0340 */
    "cyrillic_small_letter_es",
    "cyrillic_small_letter_te",
    "cyrillic_small_letter_u",
    "cyrillic_small_letter_ef",
    "cyrillic_small_letter_ha",
    "cyrillic_small_letter_tse",
    "cyrillic_small_letter_che",
    "cyrillic_small_letter_sha", 			/* 0350 */
    "cyrillic_small_letter_shcha",
    "cyrillic_small_hard_sign",
    "cyrillic_small_letter_yeru",
    "cyrillic_small_soft_sign",
    "cyrillic_small_letter_e",
    "cyrillic_small_letter_yu",
    "cyrillic_small_letter_ya",
    "number_acronym",					/* 0360 */
    "cyrillic_small_letter_io",
    "serbocroatian_cyrillic_small_letter_dje",
    "macedonian_cyrillic_small_letter_gje",
    "ukrainian_cyrillic_small_letter_ie",
    "macedonian_cyrillic_small_letter_dze",
    "ukrainian_cyrillic_small_letter_i",
    "ukrainian_cyrillic_small_letter_yi",
    "cyrillic_small_letter_je",				/* 0370 */
    "cyrillic_small_letter_lje",
    "cyrillic_small_letter_nje",
    "serbocroatian_cyrillic_small_letter_chje",
    "macedonian_cyrillic_small_letter_kje",
    "paragraph_sign",
    "bielorussian_cyrillic_small_letter_short_u", 	/* printing error in ECMA-113 */
    "cyrillic_small_letter_dze"
};

static char *iso_8859_7_syms[] = { /* 160-255 */
    "", "leftquote", "rightquote", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "accent", "diaeresisaccent", "Alphaaccent", "",
    "Epsilonaccent", "Etaaccent", "Iotaaccent", "rightanglequote", "Omicronaccent", "onehalf", "Upsilonaccent", "Omegaaccent",
    "iotadiaeresisaccent", "Alpha", "Beta", "Gamma", "Delta", "Epsilon", "Zeta", "Eta",
    "Theta", "Iota", "Kappa", "Lamda" /*sic*/, "Mu", "Nu", "Ksi", "Omicron",
    "Pi", "Rho", "", "Sigma", "Tau", "Upsilon", "Phi", "Khi",
    "Psi", "Omega", "Iotadiaeresis", "Upsilondiaeresis", "alphaaccent", "epsilonaccent", "etaaccent", "iotaaccent",
    "upsilondiaeresisaccent", "alpha", "beta", "gamma", "delta", "epsilon", "zeta", "eta",
    "theta", "iota", "kappa", "lamda" /*sic*/, "mu", "nu", "ksi", "omicron",
    "pi", "rho", "terminalsigma", "sigma", "tau", "upsilon", "phi", "khi",
    "psi", "omega", "iotadiaeresis", "upsilondiaeresis", "omicronaccent", "upsilonaccent", "omegaaccent", ""
};

static char *iso_8859_8_syms[] = {
    "", "", "", "", "", "", "", "",
    "", "", "multiplication", "", "", "", "", "overscore",
    "", "", "", "", "", "", "", "",
    "", "", "division", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "",
    "", "", "", "", "", "", "", "doubleunderscore",
    "alef", "bet", "gimel", "dalet", "he", "vav", "zayin", "het",
    "tet", "yod", "finalkaf", "kaf", "lamed", "finalmem", "mem", "finalnun",
    "nun", "samekh", "ayin", "finalpe", "pe", "finaltsadi", "tsadi", "qof",
    "resh", "shin", "tav", "", "", "", "", ""
};

static char *iso_8859_9_syms[] = { /* latin-5 */
	/* Identical to latin-1, but with the 6 symbols
	   ETH, eth, THORN, thorn, Yacute, yacute replaced by
	   Gbreve, gbreve, Scedilla, scedilla, Idotabove, dotlessi */
	"Gbreve", "", "", "", "", "", "", "",         /* 0320-0327 */
	"", "", "", "", "", "Idotabove", "Scedilla", "",
	"", "", "", "", "", "", "", "",	              /* 0340-0347 */
	"", "", "", "", "", "", "", "",	              /* 0350-0357 */
	"gbreve", "", "", "", "", "", "", "",         /* 0360-0367 */
	"", "", "", "", "", "dotlessi", "scedilla", ""
};

#include "cyrillic.syms.h"
#include "ethiopic.syms.h"

static char *fn_syms[] = {
	"F1", 	"F2",	"F3",	"F4",	"F5",
	"F6",	"F7",	"F8",	"F9",	"F10",
	"F11",	"F12",	"F13",	"F14",	"F15",
	"F16",	"F17",	"F18",	"F19",	"F20",
	"Find",			/* also called: "Home" */
	"Insert",
	"Remove",
	"Select",		/* also called: "End" */
	"Prior",		/* also called: "PageUp" */
	"Next",			/* also called: "PageDown" */
	"Macro",
	"Help",
	"Do",
	"Pause",
	"F21",	"F22",	"F23",	"F24",	"F25",
	"F26",	"F27",	"F28",	"F29",	"F30",
	"F31",	"F32",	"F33",	"F34",	"F35",
	"F36",	"F37",	"F38",	"F39",	"F40",
	"F41",	"F42",	"F43",	"F44",	"F45",
	"F46",	"F47",	"F48",	"F49",	"F50",
	"F51",	"F52",	"F53",	"F54",	"F55",
	"F56",	"F57",	"F58",	"F59",	"F60",
	"F61",	"F62",	"F63",	"F64",	"F65",
	"F66",	"F67",	"F68",	"F69",	"F70",
	"F71",	"F72",	"F73",	"F74",	"F75",
	"F76",	"F77",	"F78",	"F79",	"F80",
	"F81",	"F82",	"F83",	"F84",	"F85",
	"F86",	"F87",	"F88",	"F89",	"F90",
	"F91",	"F92",	"F93",	"F94",	"F95",
	"F96",	"F97",	"F98",	"F99",	"F100",
	"F101",	"F102",	"F103",	"F104",	"F105",
	"F106",	"F107",	"F108",	"F109",	"F110",
	"F111",	"F112",	"F113",	"F114",	"F115",
	"F116",	"F117",	"F118",	"F119",	"F120",
	"F121",	"F122",	"F123",	"F124",	"F125",
	"F126",	"F127",	"F128",	"F129",	"F130",
	"F131",	"F132",	"F133",	"F134",	"F135",
	"F136",	"F137",	"F138",	"F139",	"F140",
	"F141",	"F142",	"F143",	"F144",	"F145",
	"F146",	"F147",	"F148",	"F149",	"F150",
	"F151",	"F152",	"F153",	"F154",	"F155",
	"F156",	"F157",	"F158",	"F159",	"F160",
	"F161",	"F162",	"F163",	"F164",	"F165",
	"F166",	"F167",	"F168",	"F169",	"F170",
	"F171",	"F172",	"F173",	"F174",	"F175",
	"F176",	"F177",	"F178",	"F179",	"F180",
	"F181",	"F182",	"F183",	"F184",	"F185",
	"F186",	"F187",	"F188",	"F189",	"F190",
	"F191",	"F192",	"F193",	"F194",	"F195",
	"F196",	"F197",	"F198",	"F199",	"F200",
	"F201",	"F202",	"F203",	"F204",	"F205",
	"F206",	"F207",	"F208",	"F209",	"F210",
	"F211",	"F212",	"F213",	"F214",	"F215",
	"F216",	"F217",	"F218",	"F219",	"F220",
	"F221",	"F222",	"F223",	"F224",	"F225",
	"F226",	"F227",	"F228",	"F229",	"F230",
	"F231",	"F232",	"F233",	"F234",	"F235",
	"F236",	"F237",	"F238",	"F239",	"F240",
	"F241",	"F242",	"F243",	"F244",	"F245",
	"F246"		/* there are 10 keys named Insert etc., total 256 */
};

static char *spec_syms[] = {
	"VoidSymbol",
	"Return",
	"Show_Registers",
	"Show_Memory",
	"Show_State",
	"Break",
	"Last_Console",
	"Caps_Lock",
	"Num_Lock",
	"Scroll_Lock",
	"Scroll_Forward",
	"Scroll_Backward",
	"Boot",
	"Caps_On",
	"Compose",
	"SAK",
	"Decr_Console",
	"Incr_Console",
	"KeyboardSignal",
	"Bare_Num_Lock"
};

static char *pad_syms[] = {
	"KP_0",
	"KP_1",
	"KP_2",
	"KP_3",
	"KP_4",
	"KP_5",
	"KP_6",
	"KP_7",
	"KP_8",
	"KP_9",
	"KP_Add",
	"KP_Subtract",
	"KP_Multiply",
	"KP_Divide",
	"KP_Enter",
	"KP_Comma",
	"KP_Period",
	"KP_MinPlus"
};

static char *dead_syms[] = {
	"dead_grave",
	"dead_acute",
	"dead_circumflex",
	"dead_tilde",
	"dead_diaeresis",
	"dead_cedilla"
};

static char *cons_syms[] = {
	"Console_1",
	"Console_2",
	"Console_3",
	"Console_4",
	"Console_5",
	"Console_6",
	"Console_7",
	"Console_8",
	"Console_9",
	"Console_10",
	"Console_11",
	"Console_12",
	"Console_13",
	"Console_14",
	"Console_15",
	"Console_16",
	"Console_17",
	"Console_18",
	"Console_19",
	"Console_20",
	"Console_21",
	"Console_22",
	"Console_23",
	"Console_24",
	"Console_25",
	"Console_26",
	"Console_27",
	"Console_28",
	"Console_29",
	"Console_30",
	"Console_31",
	"Console_32",
	"Console_33",
	"Console_34",
	"Console_35",
	"Console_36",
	"Console_37",
	"Console_38",
	"Console_39",
	"Console_40",
	"Console_41",
	"Console_42",
	"Console_43",
	"Console_44",
	"Console_45",
	"Console_46",
	"Console_47",
	"Console_48",
	"Console_49",
	"Console_50",
	"Console_51",
	"Console_52",
	"Console_53",
	"Console_54",
	"Console_55",
	"Console_56",
	"Console_57",
	"Console_58",
	"Console_59",
	"Console_60",
	"Console_61",
	"Console_62",
	"Console_63"
};

static char *cur_syms[] = {
	"Down",
	"Left",
	"Right",
	"Up"
};

static char *shift_syms[] = {
	"Shift",
	"AltGr",
	"Control",
	"Alt",
	"ShiftL",
	"ShiftR",
	"CtrlL",
	"CtrlR",
	"CapsShift"
};

static char *ascii_syms[] = {
	"Ascii_0",
	"Ascii_1",
	"Ascii_2",
	"Ascii_3",
	"Ascii_4",
	"Ascii_5",
	"Ascii_6",
	"Ascii_7",
	"Ascii_8",
	"Ascii_9",
	"Hex_0",
	"Hex_1",
	"Hex_2",
	"Hex_3",
	"Hex_4",
	"Hex_5",
	"Hex_6",
	"Hex_7",
	"Hex_8",
	"Hex_9",
	"Hex_A",
	"Hex_B",
	"Hex_C",
	"Hex_D",
	"Hex_E",
	"Hex_F"
};

static char *lock_syms[] = {
	"Shift_Lock",
	"AltGr_Lock",
	"Control_Lock",
	"Alt_Lock",
	"ShiftL_Lock",
	"ShiftR_Lock",
	"CtrlL_Lock",
	"CtrlR_Lock"
};

static char *sticky_syms[] = {
	"SShift",
	"SAltGr",
	"SControl",
	"SAlt",
	"SShiftL",
	"SShiftR",
	"SCtrlL",
	"SCtrlR"
};

#define E(x) { x, sizeof(x) / sizeof(char **) }

syms_entry syms[] = {
	E(latin1_syms),
	E(fn_syms),
	E(spec_syms),
	E(pad_syms),
	E(dead_syms),
	E(cons_syms),
	E(cur_syms),
	E(shift_syms),
	{ 0, 0 },		/* KT_META */
	E(ascii_syms),
	E(lock_syms),
	{ 0, 0 },		/* KT_LETTER */
	E(sticky_syms)
};

#undef E

struct syn
synonyms[] = {
	{ "Control_h",		"BackSpace" },
	{ "Control_i",		"Tab" },
	{ "Control_j",		"Linefeed" },
	{ "Home",		"Find" },
/* Unfortunately Delete already denotes ASCII 0177 */
/*	{ "Delete",		"Remove" }, */
	{ "End",		"Select" },
	{ "PageUp",		"Prior" },
	{ "PageDown",		"Next" },
	{ "multiplication",	"multiply" },
	{ "pound",		"sterling" },
	{ "pilcrow",		"paragraph" },
	{ "Oslash",		"Ooblique" },
	{ "Shift_L",		"ShiftL" },
	{ "Shift_R",		"ShiftR" },
	{ "Control_L",		"CtrlL" },
	{ "Control_R",		"CtrlR" },
	{ "AltL",		"Alt" },
	{ "AltR",		"AltGr" },
	{ "Alt_L",		"Alt" },
	{ "Alt_R",		"AltGr" },
	{ "AltGr_L",		"Alt" },
	{ "AltGr_R",		"AltGr" },
	{ "AltLLock",		"Alt_Lock" },
	{ "AltRLock",		"AltGr_Lock" },
	{ "SCtrl",		"SControl" },
	{ "Spawn_Console",	"KeyboardSignal" },
	{ "Uncaps_Shift",	"CapsShift" },
/* the names of the Greek letters are spelled differently
   in the iso-8859-7 and the Unicode standards */
	{ "lambda",             "lamda" },
	{ "Lambda",             "Lamda" },
	{ "xi",                 "ksi" },
	{ "Xi",                 "Ksi" },
	{ "chi",                "khi" },
	{ "Chi",                "Khi" },
/* diacriticals */
	{ "tilde",		"asciitilde" },
	{ "circumflex",		"asciicircum" },
/* as dead_ogonek, dead_caron, dead_breve and dead_doubleacute do not exist
 * (yet), I put also compose lines for use with respectively dead_cedilla,
 * dead_circumflex, dead_tilde and dead_tilde */
	{ "dead_ogonek",        "dead_cedilla" },
	{ "dead_caron",         "dead_circumflex" },
	{ "dead_breve",         "dead_tilde" },
	{ "dead_doubleacute",   "dead_tilde" },
/* cyrillic */
	{ "bielorussian_cyrillic_capital_letter_i", "ukrainian_cyrillic_capital_letter_i" },
	{ "cyrillic_capital_letter_kha", "cyrillic_capital_letter_ha" },
	{ "cyrillic_capital_letter_ge", "cyrillic_capital_letter_ghe" },
	{ "cyrillic_capital_letter_ia", "cyrillic_capital_letter_ya" },
	{ "cyrillic_capital_letter_iu", "cyrillic_capital_letter_yu" },
	{ "cyrillic_capital_letter_yeri", "cyrillic_capital_letter_yeru" },
	{ "cyrillic_capital_letter_reversed_e", "cyrillic_capital_letter_e" },
	{ "cyrillic_capital_letter_ii", "cyrillic_capital_letter_i" },
	{ "cyrillic_capital_letter_short_ii", "cyrillic_capital_letter_short_i" },
	{ "bielorussian_cyrillic_small_letter_i", "ukrainian_cyrillic_small_letter_i" },
	{ "cyrillic_small_letter_kha", "cyrillic_small_letter_ha" },
	{ "cyrillic_small_letter_ge", "cyrillic_small_letter_ghe" },
	{ "cyrillic_small_letter_ia", "cyrillic_small_letter_ya" },
	{ "cyrillic_small_letter_iu", "cyrillic_small_letter_yu" },
	{ "cyrillic_small_letter_yeri", "cyrillic_small_letter_yeru" },
	{ "cyrillic_small_letter_reversed_e", "cyrillic_small_letter_e" },
	{ "cyrillic_small_letter_ii", "cyrillic_small_letter_i" },
	{ "cyrillic_small_letter_short_ii", "cyrillic_small_letter_short_i" }
};

const int syms_size = sizeof(syms) / sizeof(syms_entry);
const int syn_size = sizeof(synonyms) / sizeof(synonyms[0]);

struct cs {
    char *charset;
    char **charnames;
    int start;
} charsets[] = {
    { "iso-8859-1", 0, 0 },
    { "mazovia", mazovia_syms, 128 },
    { "iso-8859-2", latin2_syms, 160 },
    { "iso-8859-3", latin3_syms, 160 },
    { "iso-8859-4", latin4_syms, 160 },
    { "iso-8859-5", iso_8859_5_syms, 160 },
    { "iso-8859-7", iso_8859_7_syms, 160 },
    { "iso-8859-8", iso_8859_8_syms, 160 },
    { "iso-8859-9", iso_8859_9_syms, 208 },
    { "koi8-r",     koi8_r_syms, 160 },
    { "koi8-u",     koi8_r_syms, 160 },
/* I don't know the differences between koi8-r and koi8-u,
   so I use the same syms table - Pablo Saratxaga */
    { "iso-10646-18", iso_10646_18_syms, 159 },		/* ethiopic */
};

int
set_charset(char *charset) {
	char **p;
	int i;

	for (i = 0; i < sizeof(charsets)/sizeof(charsets[0]); i++) {
	    if (!strcasecmp(charsets[i].charset, charset)) {
		p = charsets[i].charnames;
		if (p)
		  for (i = charsets[i].start; i < 256; i++,p++) {
		    if(*p && **p)
		      syms[0].table[i] = *p;
		}
		return 0;
	    }
	}
	if (!strcasecmp(charset, "unicode")) {
	  fprintf (stderr, _("unknown charset %s - ignoring charset request\n"),
		   charset);
	  return 1;
	}
	return 0;
}

int
ksymtocode(char *s)
{
	int i;
	int j;
	int keycode;
	static int warn = 0;

	if (!strncmp(s, "Meta_", 5)) {
		keycode = ksymtocode(s+5);
		if (KTYP(keycode) == KT_LATIN)
			return K(KT_META, KVAL(keycode));
		/* fall through to error printf */
	}

	for (i = 0; i < syms_size; i++)
		for (j = 0; j < syms[i].size; j++)
			if (!strcmp(s,syms[i].table[j]))
				return K(i, j);

	for (i = 0; i < syn_size; i++)
		if (!strcmp(s, synonyms[i].synonym))
			return ksymtocode(synonyms[i].official_name);

	for (i = 0; i < 256 - 160; i++)
		if (!strcmp(s, latin2_syms[i])) {
			if(!warn++)
				fprintf(stderr, _("assuming iso-8859-2 %s\n"), s);
			return K(KT_LATIN, 160 + i);
		}

	for (i = 0; i < 256 - 160; i++)
		if (!strcmp(s, latin3_syms[i])) {
			if(!warn++)
				fprintf(stderr, _("assuming iso-8859-3 %s\n"), s);
			return K(KT_LATIN, 160 + i);
		}

	for (i = 0; i < 256 - 160; i++)
		if (!strcmp(s, latin4_syms[i])) {
			if(!warn++)
				fprintf(stderr, _("assuming iso-8859-4 %s\n"), s);
			return K(KT_LATIN, 160 + i);
		}

	fprintf(stderr, _("unknown keysym '%s'\n"), s);

	return -1;
}
