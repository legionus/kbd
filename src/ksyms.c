#include <linux/keyboard.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "ksyms.h"
#include "nls.h"

/* Keysyms whose KTYP is KT_LATIN or KT_LETTER and whose KVAL is 0..127. */

static const char *iso646_syms[] = {
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

	/* set_charset() fills in charset dependent strings here. */
	/* start with the latin1 defaults */
	"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
	"", "", "", "", "", "", "", "", "", "", "", "", "", "", "", "",
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
	"ydiaeresis",
};

/* Keysyms whose KTYP is KT_LATIN or KT_LETTER and whose KVAL is 128..255,
   and their Unicode equivalent. */

static sym latin1_syms[] = {
	{ 0x00a0, "nobreakspace" },
	{ 0x00a1, "exclamdown" },
	{ 0x00a2, "cent" },
	{ 0x00a3, "sterling" },
	{ 0x00a4, "currency" },
	{ 0x00a5, "yen" },
	{ 0x00a6, "brokenbar" },
	{ 0x00a7, "section" },
	{ 0x00a8, "diaeresis" },
	{ 0x00a9, "copyright" },
	{ 0x00aa, "ordfeminine" },
	{ 0x00ab, "guillemotleft" },
	{ 0x00ac, "notsign" },
	{ 0x00ad, "hyphen" },
	{ 0x00ae, "registered" },
	{ 0x00af, "macron" },
	{ 0x00b0, "degree" },
	{ 0x00b1, "plusminus" },
	{ 0x00b2, "twosuperior" },
	{ 0x00b3, "threesuperior" },
	{ 0x00b4, "acute" },
	{ 0x00b5, "mu" },
	{ 0x00b6, "paragraph" },
	{ 0x00b7, "periodcentered" },
	{ 0x00b8, "cedilla" },
	{ 0x00b9, "onesuperior" },
	{ 0x00ba, "masculine" },
	{ 0x00bb, "guillemotright" },
	{ 0x00bc, "onequarter" },
	{ 0x00bd, "onehalf" },
	{ 0x00be, "threequarters" },
	{ 0x00bf, "questiondown" },
	{ 0x00c0, "Agrave" },
	{ 0x00c1, "Aacute" },
	{ 0x00c2, "Acircumflex" },
	{ 0x00c3, "Atilde" },
	{ 0x00c4, "Adiaeresis" },
	{ 0x00c5, "Aring" },
	{ 0x00c6, "AE" },
	{ 0x00c7, "Ccedilla" },
	{ 0x00c8, "Egrave" },
	{ 0x00c9, "Eacute" },
	{ 0x00ca, "Ecircumflex" },
	{ 0x00cb, "Ediaeresis" },
	{ 0x00cc, "Igrave" },
	{ 0x00cd, "Iacute" },
	{ 0x00ce, "Icircumflex" },
	{ 0x00cf, "Idiaeresis" },
	{ 0x00d0, "ETH" },
	{ 0x00d1, "Ntilde" },
	{ 0x00d2, "Ograve" },
	{ 0x00d3, "Oacute" },
	{ 0x00d4, "Ocircumflex" },
	{ 0x00d5, "Otilde" },
	{ 0x00d6, "Odiaeresis" },
	{ 0x00d7, "multiply" },
	{ 0x00d8, "Ooblique" },
	{ 0x00d9, "Ugrave" },
	{ 0x00da, "Uacute" },
	{ 0x00db, "Ucircumflex" },
	{ 0x00dc, "Udiaeresis" },
	{ 0x00dd, "Yacute" },
	{ 0x00de, "THORN" },
	{ 0x00df, "ssharp" },
	{ 0x00e0, "agrave" },
	{ 0x00e1, "aacute" },
	{ 0x00e2, "acircumflex" },
	{ 0x00e3, "atilde" },
	{ 0x00e4, "adiaeresis" },
	{ 0x00e5, "aring" },
	{ 0x00e6, "ae" },
	{ 0x00e7, "ccedilla" },
	{ 0x00e8, "egrave" },
	{ 0x00e9, "eacute" },
	{ 0x00ea, "ecircumflex" },
	{ 0x00eb, "ediaeresis" },
	{ 0x00ec, "igrave" },
	{ 0x00ed, "iacute" },
	{ 0x00ee, "icircumflex" },
	{ 0x00ef, "idiaeresis" },
	{ 0x00f0, "eth" },
	{ 0x00f1, "ntilde" },
	{ 0x00f2, "ograve" },
	{ 0x00f3, "oacute" },
	{ 0x00f4, "ocircumflex" },
	{ 0x00f5, "otilde" },
	{ 0x00f6, "odiaeresis" },
	{ 0x00f7, "division" },
	{ 0x00f8, "oslash" },
	{ 0x00f9, "ugrave" },
	{ 0x00fa, "uacute" },
	{ 0x00fb, "ucircumflex" },
	{ 0x00fc, "udiaeresis" },
	{ 0x00fd, "yacute" },
	{ 0x00fe, "thorn" },
	{ 0x00ff, "ydiaeresis" }
};

static sym latin2_syms[] = {
	{ 0x00a0, "" },		/* 0240 */
	{ 0x0104, "Aogonek" },
	{ 0x02d8, "breve" },
	{ 0x0141, "Lstroke" },
	{ 0x00a4, "" },
	{ 0x013d, "Lcaron" },
	{ 0x015a, "Sacute" },
	{ 0x00a7, "" },
	{ 0x00a8, "" },
	{ 0x0160, "Scaron" },
	{ 0x015e, "Scedilla" },
	{ 0x0164, "Tcaron" },
	{ 0x0179, "Zacute" },
	{ 0x00ad, "" },
	{ 0x017d, "Zcaron" },
	{ 0x017b, "Zabovedot" },
	{ 0x00b0, "" },
	{ 0x0105, "aogonek" },
	{ 0x02db, "ogonek" },
	{ 0x0142, "lstroke" },
	{ 0x00b4, "" },
	{ 0x013e, "lcaron" },
	{ 0x015b, "sacute" },
	{ 0x02c7, "caron" },
	{ 0x00b8, "" },
	{ 0x0161, "scaron" },
	{ 0x015f, "scedilla" },
	{ 0x0165, "tcaron" },
	{ 0x017a, "zacute" },
	{ 0x02dd, "doubleacute" },
	{ 0x017e, "zcaron" },
	{ 0x017c, "zabovedot" },
	{ 0x0154, "Racute" },	/* 0300 */
	{ 0x00c1, "" },
	{ 0x00c2, "" },
	{ 0x0102, "Abreve" },
	{ 0x00c4, "" },
	{ 0x0139, "Lacute" },
	{ 0x0106, "Cacute" },
	{ 0x00c7, "" },
	{ 0x010c, "Ccaron" },
	{ 0x00c9, "" },
	{ 0x0118, "Eogonek" },
	{ 0x00cb, "" },
	{ 0x011a, "Ecaron" },
	{ 0x00cd, "" },
	{ 0x00ce, "" },
	{ 0x010e, "Dcaron" },
	{ 0x0110, "Dstroke" },
	{ 0x0143, "Nacute" },
	{ 0x0147, "Ncaron" },
	{ 0x00d3, "" },
	{ 0x00d4, "" },
	{ 0x0150, "Odoubleacute" },
	{ 0x00d6, "" },
	{ 0x00d7, "" },
	{ 0x0158, "Rcaron" },
	{ 0x016e, "Uring" },
	{ 0x00da, "" },
	{ 0x0170, "Udoubleacute" },
	{ 0x00dc, "" },
	{ 0x00dd, "" },
	{ 0x0162, "Tcedilla" },
	{ 0x00df, "" },
	{ 0x0155, "racute" },
	{ 0x00e1, "" },
	{ 0x00e2, "" },
	{ 0x0103, "abreve" },
	{ 0x00e4, "" },
	{ 0x013a, "lacute" },
	{ 0x0107, "cacute" },
	{ 0x00e7, "" },
	{ 0x010d, "ccaron" },
	{ 0x00e9, "" },
	{ 0x0119, "eogonek" },
	{ 0x00eb, "" },
	{ 0x011b, "ecaron" },
	{ 0x00ed, "" },
	{ 0x00ee, "" },
	{ 0x010f, "dcaron" },
	{ 0x0111, "dstroke" },
	{ 0x0144, "nacute" },
	{ 0x0148, "ncaron" },
	{ 0x00f3, "" },
	{ 0x00f4, "" },
	{ 0x0151, "odoubleacute" },
	{ 0x00f6, "" },
	{ 0x00f7, "" },
	{ 0x0159, "rcaron" },
	{ 0x016f, "uring" },
	{ 0x00fa, "" },
	{ 0x0171, "udoubleacute" },
	{ 0x00fc, "" },
	{ 0x00fd, "" },
	{ 0x0163, "tcedilla" },
	{ 0x02d9, "abovedot" }
};

static sym mazovia_syms[] = {
	/* as specified by Wlodek Bzyl <matwb@univ.gda.pl> */
	{ 0x0080, "" },
	{ 0x0081, "" },
	{ 0x0082, "" },
	{ 0x0083, "" },
	{ 0x0084, "" },
	{ 0x0085, "" },
	{ 0x0105, "aogonek" },
	{ 0x0087, "" },
	{ 0x0088, "" },
	{ 0x0089, "" },
	{ 0x008a, "" },
	{ 0x008b, "" },
	{ 0x008c, "" },
	{ 0x0107, "cacute" },
	{ 0x008e, "" },
	{ 0x0104, "Aogonek" },
	{ 0x0118, "Eogonek" },
	{ 0x0119, "eogonek" },
	{ 0x0142, "lstroke" },
	{ 0x0093, "" },
	{ 0x0094, "" },
	{ 0x0106, "Cacute" },
	{ 0x0096, "" },
	{ 0x0097, "" },
	{ 0x015a, "Sacute" },
	{ 0x0099, "" },
	{ 0x009a, "" },
	{ 0x009b, "" },
	{ 0x0141, "Lstroke" },
	{ 0x009d, "" },
	{ 0x015b, "sacute" },
	{ 0x009f, "" },
	{ 0x0179, "Zacute" },
	{ 0x017b, "Zabovedot" },
	{ 0x00f3, "oacute" },
	{ 0x00d3, "Oacute" },
	{ 0x0144, "nacute" },
	{ 0x0143, "Nacute" },
	{ 0x017a, "zacute" },
	{ 0x017c, "zabovedot" },
	{ 0x00a8, "" },
	{ 0x00a9, "" },
	{ 0x00aa, "" },
	{ 0x00ab, "" },
	{ 0x00ac, "" },
	{ 0x00ad, "" },
	{ 0x00ab, "guillemotleft" },
	{ 0x00bb, "guillemotright" },
	{ 0x00b0, "" },
	{ 0x00b1, "" },
	{ 0x00b2, "" },
	{ 0x00b3, "" },
	{ 0x00b4, "" },
	{ 0x00b5, "" },
	{ 0x00b6, "" },
	{ 0x00b7, "" },
	{ 0x00b8, "" },
	{ 0x00b9, "" },
	{ 0x00ba, "" },
	{ 0x00bb, "" },
	{ 0x00bc, "" },
	{ 0x00bd, "" },
	{ 0x00be, "" },
	{ 0x00bf, "" },
	{ 0x00c0, "" },
	{ 0x00c1, "" },
	{ 0x00c2, "" },
	{ 0x00c3, "" },
	{ 0x00c4, "" },
	{ 0x00c5, "" },
	{ 0x00c6, "" },
	{ 0x00c7, "" },
	{ 0x00c8, "" },
	{ 0x00c9, "" },
	{ 0x00ca, "" },
	{ 0x00cb, "" },
	{ 0x00cc, "" },
	{ 0x00cd, "" },
	{ 0x00ce, "" },
	{ 0x00cf, "" },
	{ 0x00d0, "" },
	{ 0x00d1, "" },
	{ 0x00d2, "" },
	{ 0x00d3, "" },
	{ 0x00d4, "" },
	{ 0x00d5, "" },
	{ 0x00d6, "" },
	{ 0x00d7, "" },
	{ 0x00d8, "" },
	{ 0x00d9, "" },
	{ 0x00da, "" },
	{ 0x00db, "" },
	{ 0x00dc, "" },
	{ 0x00dd, "" },
	{ 0x00de, "" },
	{ 0x00df, "" },
	{ 0x00e0, "" },
	{ 0x00e1, "" },
	{ 0x00e2, "" },
	{ 0x00e3, "" },
	{ 0x00e4, "" },
	{ 0x00e5, "" },
	{ 0x00e6, "" },
	{ 0x00e7, "" },
	{ 0x00e8, "" },
	{ 0x00e9, "" },
	{ 0x00ea, "" },
	{ 0x00eb, "" },
	{ 0x00ec, "" },
	{ 0x00ed, "" },
	{ 0x00ee, "" },
	{ 0x00ef, "" },
	{ 0x00f0, "" },
	{ 0x00f1, "" },
	{ 0x00f2, "" },
	{ 0x00f3, "" },
	{ 0x00f4, "" },
	{ 0x00f5, "" },
	{ 0x00f6, "" },
	{ 0x00f7, "" },
	{ 0x00f8, "" },
	{ 0x00f9, "" },
	{ 0x00fa, "" },
	{ 0x00fb, "" },
	{ 0x00fc, "" },
	{ 0x00fd, "" },
	{ 0x00fe, "" },
	{ 0x201e, "quotedblbase" }
};

static sym latin3_syms[] = {
	{ 0x00a0, "" },
	{ 0x0126, "Hstroke" },
	{ 0x02d8, "" },
	{ 0x00a3, "" },
	{ 0x00a4, "" },
	{ 0xfffd, "" },
	{ 0x0124, "Hcircumflex" },
	{ 0x00a7, "" },
	{ 0x00a8, "" },
	{ 0x0130, "Iabovedot" },
	{ 0x015e, "" },
	{ 0x011e, "Gbreve" },
	{ 0x0134, "Jcircumflex" },
	{ 0x00ad, "" },
	{ 0xfffd, "" },
	{ 0x017b, "" },
	{ 0x00b0, "" },
	{ 0x0127, "hstroke" },
	{ 0x00b2, "" },
	{ 0x00b3, "" },
	{ 0x00b4, "" },
	{ 0x00b5, "" },
	{ 0x0125, "hcircumflex" },
	{ 0x00b7, "" },
	{ 0x00b8, "" },
	{ 0x0131, "idotless" },
	{ 0x015f, "" },
	{ 0x011f, "gbreve" },
	{ 0x0135, "jcircumflex" },
	{ 0x00bd, "" },
	{ 0xfffd, "" },
	{ 0x017c, "" },
	{ 0x00c0, "" },
	{ 0x00c1, "" },
	{ 0x00c2, "" },
	{ 0xfffd, "" },
	{ 0x00c4, "" },
	{ 0x010a, "Cabovedot" },
	{ 0x0108, "Ccircumflex" },
	{ 0x00c7, "" },
	{ 0x00c8, "" },
	{ 0x00c9, "" },
	{ 0x00ca, "" },
	{ 0x00cb, "" },
	{ 0x00cc, "" },
	{ 0x00cd, "" },
	{ 0x00ce, "" },
	{ 0x00cf, "" },
	{ 0xfffd, "" },
	{ 0x00d1, "" },
	{ 0x00d2, "" },
	{ 0x00d3, "" },
	{ 0x00d4, "" },
	{ 0x0120, "Gabovedot" },
	{ 0x00d6, "" },
	{ 0x00d7, "" },
	{ 0x011c, "Gcircumflex" },
	{ 0x00d9, "" },
	{ 0x00da, "" },
	{ 0x00db, "" },
	{ 0x00dc, "" },
	{ 0x016c, "Ubreve" },
	{ 0x015c, "Scircumflex" },
	{ 0x00df, "" },
	{ 0x00e0, "" },
	{ 0x00e1, "" },
	{ 0x00e2, "" },
	{ 0xfffd, "" },
	{ 0x00e4, "" },
	{ 0x010b, "cabovedot" },
	{ 0x0109, "ccircumflex" },
	{ 0x00e7, "" },
	{ 0x00e8, "" },
	{ 0x00e9, "" },
	{ 0x00ea, "" },
	{ 0x00eb, "" },
	{ 0x00ec, "" },
	{ 0x00ed, "" },
	{ 0x00ee, "" },
	{ 0x00ef, "" },
	{ 0xfffd, "" },
	{ 0x00f1, "" },
	{ 0x00f2, "" },
	{ 0x00f3, "" },
	{ 0x00f4, "" },
	{ 0x0121, "gabovedot" },
	{ 0x00f6, "" },
	{ 0x00f7, "" },
	{ 0x011d, "gcircumflex" },
	{ 0x00f9, "" },
	{ 0x00fa, "" },
	{ 0x00fb, "" },
	{ 0x00fc, "" },
	{ 0x016d, "ubreve" },
	{ 0x015d, "scircumflex" },
	{ 0x02d9, "" }
};

static sym latin4_syms[] = {
	{ 0x00a0, "" },
	{ 0x0104, "" },
	{ 0x0138, "kra" },
	{ 0x0156, "Rcedilla" },
	{ 0x00a4, "" },
	{ 0x0128, "Itilde" },
	{ 0x013b, "Lcedilla" },
	{ 0x00a7, "" },
	{ 0x00a8, "" },
	{ 0x0160, "" },
	{ 0x0112, "Emacron" },
	{ 0x0122, "Gcedilla" },
	{ 0x0166, "Tslash" },
	{ 0x00ad, "" },
	{ 0x017d, "" },
	{ 0x00af, "" },
	{ 0x00b0, "" },
	{ 0x0105, "" },
	{ 0x02db, "" },
	{ 0x0157, "rcedilla" },
	{ 0x00b4, "" },
	{ 0x0129, "itilde" },
	{ 0x013c, "lcedilla" },
	{ 0x02c7, "" },
	{ 0x00b8, "" },
	{ 0x0161, "" },
	{ 0x0113, "emacron" },
	{ 0x0123, "gcedilla" },
	{ 0x0167, "tslash" },
	{ 0x014a, "ENG" },
	{ 0x017e, "" },
	{ 0x014b, "eng" },
	{ 0x0100, "Amacron" },
	{ 0x00c1, "" },
	{ 0x00c2, "" },
	{ 0x00c3, "" },
	{ 0x00c4, "" },
	{ 0x00c5, "" },
	{ 0x00c6, "" },
	{ 0x012e, "Iogonek" },
	{ 0x010c, "" },
	{ 0x00c9, "" },
	{ 0x0118, "" },
	{ 0x00cb, "" },
	{ 0x0116, "Eabovedot" },
	{ 0x00cd, "" },
	{ 0x00ce, "" },
	{ 0x012a, "Imacron" },
	{ 0x0110, "" },
	{ 0x0145, "Ncedilla" },
	{ 0x014c, "Omacron" },
	{ 0x0136, "Kcedilla" },
	{ 0x00d4, "" },
	{ 0x00d5, "" },
	{ 0x00d6, "" },
	{ 0x00d7, "" },
	{ 0x00d8, "" },
	{ 0x0172, "Uogonek" },
	{ 0x00da, "" },
	{ 0x00db, "" },
	{ 0x00dc, "" },
	{ 0x0168, "Utilde" },
	{ 0x016a, "Umacron" },
	{ 0x00df, "" },
	{ 0x0101, "amacron" },
	{ 0x00e1, "" },
	{ 0x00e2, "" },
	{ 0x00e3, "" },
	{ 0x00e4, "" },
	{ 0x00e5, "" },
	{ 0x00e6, "" },
	{ 0x012f, "iogonek" },
	{ 0x010d, "" },
	{ 0x00e9, "" },
	{ 0x0119, "" },
	{ 0x00eb, "" },
	{ 0x0117, "eabovedot" },
	{ 0x00ed, "" },
	{ 0x00ee, "" },
	{ 0x012b, "imacron" },
	{ 0x0111, "" },
	{ 0x0146, "ncedilla" },
	{ 0x014d, "omacron" },
	{ 0x0137, "kcedilla" },
	{ 0x00f4, "" },
	{ 0x00f5, "" },
	{ 0x00f6, "" },
	{ 0x00f7, "" },
	{ 0x00f8, "" },
	{ 0x0173, "uogonek" },
	{ 0x00fa, "" },
	{ 0x00fb, "" },
	{ 0x00fc, "" },
	{ 0x0169, "utilde" },
	{ 0x016b, "umacron" },
	{ 0x02d9, "" }
};

static sym iso_8859_5_syms[] = { /* 160-255 */
	{ 0x00a0, "nobreakspace" },
	{ 0x0401, "cyrillic_capital_letter_io" },
	{ 0x0402, "serbocroatian_cyrillic_capital_letter_dje" },
	{ 0x0403, "macedonian_cyrillic_capital_letter_gje" },
	{ 0x0404, "ukrainian_cyrillic_capital_letter_ie" },
	{ 0x0405, "macedonian_cyrillic_capital_letter_dze" },
	{ 0x0406, "ukrainian_cyrillic_capital_letter_i" },
	{ 0x0407, "ukrainian_cyrillic_capital_letter_yi" },
	{ 0x0408, "cyrillic_capital_letter_je" }, 			/* 0250 */
	{ 0x0409, "cyrillic_capital_letter_lje" },
	{ 0x040a, "cyrillic_capital_letter_nje" },
	{ 0x040b, "serbocroatian_cyrillic_capital_letter_chje" },
	{ 0x040c, "macedonian_cyrillic_capital_letter_kje" },
	{ 0x00ad, "hyphen" },
	{ 0x040e, "bielorussian_cyrillic_capital_letter_short_u" },
	{ 0x040f, "cyrillic_capital_letter_dze" },
	{ 0x0410, "cyrillic_capital_letter_a" }, 			/* 0260 */
	{ 0x0411, "cyrillic_capital_letter_be" },
	{ 0x0412, "cyrillic_capital_letter_ve" },
	{ 0x0413, "cyrillic_capital_letter_ghe" },
	{ 0x0414, "cyrillic_capital_letter_de" },
	{ 0x0415, "cyrillic_capital_letter_ie" },
	{ 0x0416, "cyrillic_capital_letter_zhe" },
	{ 0x0417, "cyrillic_capital_letter_ze" },
	{ 0x0418, "cyrillic_capital_letter_i" }, 			/* 0270 */
	{ 0x0419, "cyrillic_capital_letter_short_i" },
	{ 0x041a, "cyrillic_capital_letter_ka" },
	{ 0x041b, "cyrillic_capital_letter_el" },
	{ 0x041c, "cyrillic_capital_letter_em" },
	{ 0x041d, "cyrillic_capital_letter_en" },
	{ 0x041e, "cyrillic_capital_letter_o" },
	{ 0x041f, "cyrillic_capital_letter_pe" },
	{ 0x0420, "cyrillic_capital_letter_er" }, 			/* 0300 */
	{ 0x0421, "cyrillic_capital_letter_es" },
	{ 0x0422, "cyrillic_capital_letter_te" },
	{ 0x0423, "cyrillic_capital_letter_u" },
	{ 0x0424, "cyrillic_capital_letter_ef" },
	{ 0x0425, "cyrillic_capital_letter_ha" },
	{ 0x0426, "cyrillic_capital_letter_tse" },
	{ 0x0427, "cyrillic_capital_letter_che" },
	{ 0x0428, "cyrillic_capital_letter_sha" }, 			/* 0310 */
	{ 0x0429, "cyrillic_capital_letter_shcha" },
	{ 0x042a, "cyrillic_capital_hard_sign" },
	{ 0x042b, "cyrillic_capital_letter_yeru" },
	{ 0x042c, "cyrillic_capital_soft_sign" },
	{ 0x042d, "cyrillic_capital_letter_e" },
	{ 0x042e, "cyrillic_capital_letter_yu" },
	{ 0x042f, "cyrillic_capital_letter_ya" },
	{ 0x0430, "cyrillic_small_letter_a" },				/* 0320 */
	{ 0x0431, "cyrillic_small_letter_be" },
	{ 0x0432, "cyrillic_small_letter_ve" },
	{ 0x0433, "cyrillic_small_letter_ghe" },
	{ 0x0434, "cyrillic_small_letter_de" },
	{ 0x0435, "cyrillic_small_letter_ie" },
	{ 0x0436, "cyrillic_small_letter_zhe" },
	{ 0x0437, "cyrillic_small_letter_ze" },
	{ 0x0438, "cyrillic_small_letter_i" },				/* 0330 */
	{ 0x0439, "cyrillic_small_letter_short_i" },
	{ 0x043a, "cyrillic_small_letter_ka" },
	{ 0x043b, "cyrillic_small_letter_el" },
	{ 0x043c, "cyrillic_small_letter_em" },
	{ 0x043d, "cyrillic_small_letter_en" },
	{ 0x043e, "cyrillic_small_letter_o" },
	{ 0x043f, "cyrillic_small_letter_pe" },
	{ 0x0440, "cyrillic_small_letter_er" },				/* 0340 */
	{ 0x0441, "cyrillic_small_letter_es" },
	{ 0x0442, "cyrillic_small_letter_te" },
	{ 0x0443, "cyrillic_small_letter_u" },
	{ 0x0444, "cyrillic_small_letter_ef" },
	{ 0x0445, "cyrillic_small_letter_ha" },
	{ 0x0446, "cyrillic_small_letter_tse" },
	{ 0x0447, "cyrillic_small_letter_che" },
	{ 0x0448, "cyrillic_small_letter_sha" }, 			/* 0350 */
	{ 0x0449, "cyrillic_small_letter_shcha" },
	{ 0x044a, "cyrillic_small_hard_sign" },
	{ 0x044b, "cyrillic_small_letter_yeru" },
	{ 0x044c, "cyrillic_small_soft_sign" },
	{ 0x044d, "cyrillic_small_letter_e" },
	{ 0x044e, "cyrillic_small_letter_yu" },
	{ 0x044f, "cyrillic_small_letter_ya" },
	{ 0x2116, "number_acronym" },					/* 0360 */
	{ 0x0451, "cyrillic_small_letter_io" },
	{ 0x0452, "serbocroatian_cyrillic_small_letter_dje" },
	{ 0x0453, "macedonian_cyrillic_small_letter_gje" },
	{ 0x0454, "ukrainian_cyrillic_small_letter_ie" },
	{ 0x0455, "macedonian_cyrillic_small_letter_dze" },
	{ 0x0456, "ukrainian_cyrillic_small_letter_i" },
	{ 0x0457, "ukrainian_cyrillic_small_letter_yi" },
	{ 0x0458, "cyrillic_small_letter_je" },				/* 0370 */
	{ 0x0459, "cyrillic_small_letter_lje" },
	{ 0x045a, "cyrillic_small_letter_nje" },
	{ 0x045b, "serbocroatian_cyrillic_small_letter_chje" },
	{ 0x045c, "macedonian_cyrillic_small_letter_kje" },
	{ 0x00a7, "section" },
	{ 0x045e, "bielorussian_cyrillic_small_letter_short_u" }, 	/* printing error in ECMA-113 */
	{ 0x045f, "cyrillic_small_letter_dze" }
};

static sym iso_8859_7_syms[] = { /* 160-255 */
	{ 0x00a0, "" },
	{ 0x02bd, "leftquote" },
	{ 0x02bc, "rightquote" },
	{ 0x00a3, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0x00a6, "" },
	{ 0x00a7, "" },
	{ 0x00a8, "" },
	{ 0x00a9, "" },
	{ 0xfffd, "" },
	{ 0x00ab, "" },
	{ 0x00ac, "" },
	{ 0x00ad, "" },
	{ 0xfffd, "" },
	{ 0x2015, "" },
	{ 0x00b0, "" },
	{ 0x00b1, "" },
	{ 0x00b2, "" },
	{ 0x00b3, "" },
	{ 0x0384, "accent" },
	{ 0x0385, "diaeresisaccent" },
	{ 0x0386, "Alphaaccent" },
	{ 0x00b7, "" },
	{ 0x0388, "Epsilonaccent" },
	{ 0x0389, "Etaaccent" },
	{ 0x038a, "Iotaaccent" },
	{ 0x00bb, "guillemotright" },
	{ 0x038c, "Omicronaccent" },
	{ 0x00bd, "onehalf" },
	{ 0x038e, "Upsilonaccent" },
	{ 0x038f, "Omegaaccent" },
	{ 0x0390, "iotadiaeresisaccent" },
	{ 0x0391, "Alpha" },
	{ 0x0392, "Beta" },
	{ 0x0393, "Gamma" },
	{ 0x0394, "Delta" },
	{ 0x0395, "Epsilon" },
	{ 0x0396, "Zeta" },
	{ 0x0397, "Eta" },
	{ 0x0398, "Theta" },
	{ 0x0399, "Iota" },
	{ 0x039a, "Kappa" },
	{ 0x039b, "Lamda" /*sic*/ },
	{ 0x039c, "Mu" },
	{ 0x039d, "Nu" },
	{ 0x039e, "Ksi" },
	{ 0x039f, "Omicron" },
	{ 0x03a0, "Pi" },
	{ 0x03a1, "Rho" },
	{ 0xfffd, "" },
	{ 0x03a3, "Sigma" },
	{ 0x03a4, "Tau" },
	{ 0x03a5, "Upsilon" },
	{ 0x03a6, "Phi" },
	{ 0x03a7, "Khi" },
	{ 0x03a8, "Psi" },
	{ 0x03a9, "Omega" },
	{ 0x03aa, "Iotadiaeresis" },
	{ 0x03ab, "Upsilondiaeresis" },
	{ 0x03ac, "alphaaccent" },
	{ 0x03ad, "epsilonaccent" },
	{ 0x03ae, "etaaccent" },
	{ 0x03af, "iotaaccent" },
	{ 0x03b0, "upsilondiaeresisaccent" },
	{ 0x03b1, "alpha" },
	{ 0x03b2, "beta" },
	{ 0x03b3, "gamma" },
	{ 0x03b4, "delta" },
	{ 0x03b5, "epsilon" },
	{ 0x03b6, "zeta" },
	{ 0x03b7, "eta" },
	{ 0x03b8, "theta" },
	{ 0x03b9, "iota" },
	{ 0x03ba, "kappa" },
	{ 0x03bb, "lamda" /*sic*/ },
	{ 0x03bc, "mu" },
	{ 0x03bd, "nu" },
	{ 0x03be, "ksi" },
	{ 0x03bf, "omicron" },
	{ 0x03c0, "pi" },
	{ 0x03c1, "rho" },
	{ 0x03c2, "terminalsigma" },
	{ 0x03c3, "sigma" },
	{ 0x03c4, "tau" },
	{ 0x03c5, "upsilon" },
	{ 0x03c6, "phi" },
	{ 0x03c7, "khi" },
	{ 0x03c8, "psi" },
	{ 0x03c9, "omega" },
	{ 0x03ca, "iotadiaeresis" },
	{ 0x03cb, "upsilondiaeresis" },
	{ 0x03cc, "omicronaccent" },
	{ 0x03cd, "upsilonaccent" },
	{ 0x03ce, "omegaaccent" },
	{ 0xfffd, "" }
};

static sym iso_8859_8_syms[] = {
	{ 0x00a0, "" },
	{ 0xfffd, "" },
	{ 0x00a2, "" },
	{ 0x00a3, "" },
	{ 0x00a4, "" },
	{ 0x00a5, "" },
	{ 0x00a6, "" },
	{ 0x00a7, "" },
	{ 0x00a8, "" },
	{ 0x00a9, "" },
	{ 0x00d7, "multiply" },
	{ 0x00ab, "" },
	{ 0x00ac, "" },
	{ 0x00ad, "" },
	{ 0x00ae, "" },
	{ 0x203e, "overscore" },
	{ 0x00b0, "" },
	{ 0x00b1, "" },
	{ 0x00b2, "" },
	{ 0x00b3, "" },
	{ 0x00b4, "" },
	{ 0x00b5, "" },
	{ 0x00b6, "" },
	{ 0x00b7, "" },
	{ 0x00b8, "" },
	{ 0x00b9, "" },
	{ 0x00f7, "division" },
	{ 0x00bb, "" },
	{ 0x00bc, "" },
	{ 0x00bd, "" },
	{ 0x00be, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0x2017, "doubleunderscore" },
	{ 0x05d0, "alef" },
	{ 0x05d1, "bet" },
	{ 0x05d2, "gimel" },
	{ 0x05d3, "dalet" },
	{ 0x05d4, "he" },
	{ 0x05d5, "vav" },
	{ 0x05d6, "zayin" },
	{ 0x05d7, "het" },
	{ 0x05d8, "tet" },
	{ 0x05d9, "yod" },
	{ 0x05da, "finalkaf" },
	{ 0x05db, "kaf" },
	{ 0x05dc, "lamed" },
	{ 0x05dd, "finalmem" },
	{ 0x05de, "mem" },
	{ 0x05df, "finalnun" },
	{ 0x05e0, "nun" },
	{ 0x05e1, "samekh" },
	{ 0x05e2, "ayin" },
	{ 0x05e3, "finalpe" },
	{ 0x05e4, "pe" },
	{ 0x05e5, "finaltsadi" },
	{ 0x05e6, "tsadi" },
	{ 0x05e7, "qof" },
	{ 0x05e8, "resh" },
	{ 0x05e9, "shin" },
	{ 0x05ea, "tav" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" },
	{ 0xfffd, "" }
};

static sym iso_8859_9_syms[] = { /* latin-5 */
	/* Identical to latin-1, but with the 6 symbols
	   ETH, eth, THORN, thorn, Yacute, yacute replaced by
	   Gbreve, gbreve, Scedilla, scedilla, Idotabove, dotlessi */
	{ 0x011e, "Gbreve" },
	{ 0x00d1, "" },
	{ 0x00d2, "" },
	{ 0x00d3, "" },
	{ 0x00d4, "" },
	{ 0x00d5, "" },
	{ 0x00d6, "" },
	{ 0x00d7, "" },
	{ 0x00d8, "" },
	{ 0x00d9, "" },
	{ 0x00da, "" },
	{ 0x00db, "" },
	{ 0x00dc, "" },
	{ 0x0130, "Idotabove" },
	{ 0x015e, "Scedilla" },
	{ 0x00df, "" },
	{ 0x00e0, "" },
	{ 0x00e1, "" },
	{ 0x00e2, "" },
	{ 0x00e3, "" },
	{ 0x00e4, "" },
	{ 0x00e5, "" },
	{ 0x00e6, "" },
	{ 0x00e7, "" },
	{ 0x00e8, "" },
	{ 0x00e9, "" },
	{ 0x00ea, "" },
	{ 0x00eb, "" },
	{ 0x00ec, "" },
	{ 0x00ed, "" },
	{ 0x00ee, "" },
	{ 0x00ef, "" },
	{ 0x011f, "gbreve" },
	{ 0x00f1, "" },
	{ 0x00f2, "" },
	{ 0x00f3, "" },
	{ 0x00f4, "" },
	{ 0x00f5, "" },
	{ 0x00f6, "" },
	{ 0x00f7, "" },
	{ 0x00f8, "" },
	{ 0x00f9, "" },
	{ 0x00fa, "" },
	{ 0x00fb, "" },
	{ 0x00fc, "" },
	{ 0x0131, "dotlessi" },
	{ 0x015f, "scedilla" },
	{ 0x00ff, "" }
};

#include "koi8.syms.h"
#include "cp1250.syms.h"
#include "thai.syms.h"
#include "ethiopic.syms.h"
#include "sami.syms.h"

static sym iso_8859_15_syms[] = {
	/* latin-1 with 8 changes */
	{ 0x00a0, "" },
	{ 0x00a1, "" },
	{ 0x00a2, "" },
	{ 0x00a3, "" },
	{ 0x20ac, "euro" },
	{ 0x00a5, "" },
	{ 0x0160, "Scaron" },
	{ 0x00a7, "" },
	{ 0x0161, "scaron" },
	{ 0x00a9, "" },
	{ 0x00aa, "" },
	{ 0x00ab, "" },
	{ 0x00ac, "" },
	{ 0x00ad, "" },
	{ 0x00ae, "" },
	{ 0x00af, "" },
	{ 0x00b0, "" },
	{ 0x00b1, "" },
	{ 0x00b2, "" },
	{ 0x00b3, "" },
	{ 0x017d, "Zcaron" },
	{ 0x00b5, "" },
	{ 0x00b6, "" },
	{ 0x00b7, "" },
	{ 0x017e, "zcaron" },
	{ 0x00b9, "" },
	{ 0x00ba, "" },
	{ 0x00bb, "" },
	{ 0x0152, "OE" },
	{ 0x0153, "oe" },
	{ 0x0178, "Ydiaeresis" },
	{ 0x00bf, "" },
	{ 0x00c0, "" },
	{ 0x00c1, "" },
	{ 0x00c2, "" },
	{ 0x00c3, "" },
	{ 0x00c4, "" },
	{ 0x00c5, "" },
	{ 0x00c6, "" },
	{ 0x00c7, "" },
	{ 0x00c8, "" },
	{ 0x00c9, "" },
	{ 0x00ca, "" },
	{ 0x00cb, "" },
	{ 0x00cc, "" },
	{ 0x00cd, "" },
	{ 0x00ce, "" },
	{ 0x00cf, "" },
	{ 0x00d0, "" },
	{ 0x00d1, "" },
	{ 0x00d2, "" },
	{ 0x00d3, "" },
	{ 0x00d4, "" },
	{ 0x00d5, "" },
	{ 0x00d6, "" },
	{ 0x00d7, "" },
	{ 0x00d8, "" },
	{ 0x00d9, "" },
	{ 0x00da, "" },
	{ 0x00db, "" },
	{ 0x00dc, "" },
	{ 0x00dd, "" },
	{ 0x00de, "" },
	{ 0x00df, "" },
	{ 0x00e0, "" },
	{ 0x00e1, "" },
	{ 0x00e2, "" },
	{ 0x00e3, "" },
	{ 0x00e4, "" },
	{ 0x00e5, "" },
	{ 0x00e6, "" },
	{ 0x00e7, "" },
	{ 0x00e8, "" },
	{ 0x00e9, "" },
	{ 0x00ea, "" },
	{ 0x00eb, "" },
	{ 0x00ec, "" },
	{ 0x00ed, "" },
	{ 0x00ee, "" },
	{ 0x00ef, "" },
	{ 0x00f0, "" },
	{ 0x00f1, "" },
	{ 0x00f2, "" },
	{ 0x00f3, "" },
	{ 0x00f4, "" },
	{ 0x00f5, "" },
	{ 0x00f6, "" },
	{ 0x00f7, "" },
	{ 0x00f8, "" },
	{ 0x00f9, "" },
	{ 0x00fa, "" },
	{ 0x00fb, "" },
	{ 0x00fc, "" },
	{ 0x00fd, "" },
	{ 0x00fe, "" },
	{ 0x00ff, "" }
};

/* Keysyms whose KTYP is KT_FN. */

static const char *fn_syms[] = {
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

/* Keysyms whose KTYP is KT_SPEC. */

static const char *spec_syms[] = {
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

/* Keysyms whose KTYP is KT_PAD. */

static const char *pad_syms[] = {
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

/* Keysyms whose KTYP is KT_DEAD. */

static const char *dead_syms[] = {
	"dead_grave",
	"dead_acute",
	"dead_circumflex",
	"dead_tilde",
	"dead_diaeresis",
	"dead_cedilla"
};

/* Keysyms whose KTYP is KT_CONS. */

static const char *cons_syms[] = {
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

/* Keysyms whose KTYP is KT_CUR. */

static const char *cur_syms[] = {
	"Down",
	"Left",
	"Right",
	"Up"
};

/* Keysyms whose KTYP is KT_SHIFT. */

static const char *shift_syms[] = {
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

/* Keysyms whose KTYP is KT_ASCII. */

static const char *ascii_syms[] = {
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

/* Keysyms whose KTYP is KT_LOCK. */

static const char *lock_syms[] = {
	"Shift_Lock",
	"AltGr_Lock",
	"Control_Lock",
	"Alt_Lock",
	"ShiftL_Lock",
	"ShiftR_Lock",
	"CtrlL_Lock",
	"CtrlR_Lock",
	"CapsShift_Lock"
};

/* Keysyms whose KTYP is KT_SLOCK. */

static const char *sticky_syms[] = {
	"SShift",
	"SAltGr",
	"SControl",
	"SAlt",
	"SShiftL",
	"SShiftR",
	"SCtrlL",
	"SCtrlR",
	"SCapsShift"
};

/* Keysyms whose KTYP is KT_BRL. */

static const char *brl_syms[] = {
	"Brl_blank",
	"Brl_dot1",
	"Brl_dot2",
	"Brl_dot3",
	"Brl_dot4",
	"Brl_dot5",
	"Brl_dot6",
	"Brl_dot7",
	"Brl_dot8",
	"Brl_dot9",
	"Brl_dot10"
};

#define E(x) { x, sizeof(x) / sizeof(x[0]) }

syms_entry syms[] = {
	E(iso646_syms),		/* KT_LATIN */
	E(fn_syms),		/* KT_FN */
	E(spec_syms),		/* KT_SPEC */
	E(pad_syms),		/* KT_PAD */
	E(dead_syms),		/* KT_DEAD */
	E(cons_syms),		/* KT_CONS */
	E(cur_syms),		/* KT_CUR */
	E(shift_syms),		/* KT_SHIFT */
	{ 0, 0 },		/* KT_META */
	E(ascii_syms),		/* KT_ASCII */
	E(lock_syms),		/* KT_LOCK */
	{ 0, 0 },		/* KT_LETTER */
	E(sticky_syms),		/* KT_SLOCK */
	{ 0, 0 },		/*  */
	E(brl_syms)		/* KT_BRL */
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
/* turkish */
	{ "Idotabove",          "Iabovedot" },
	{ "dotlessi",           "idotless" },
/* cyrillic */
	{ "no-break_space",     "nobreakspace" },
	{ "paragraph_sign",     "section" },
	{ "soft_hyphen",        "hyphen" },
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
	{ "cyrillic_small_letter_short_ii", "cyrillic_small_letter_short_i" },
/* iso-8859-7 */
	{ "rightanglequote",    "guillemotright" }
};

const unsigned int syms_size = sizeof(syms) / sizeof(syms_entry);
const unsigned int syn_size = sizeof(synonyms) / sizeof(synonyms[0]);

struct cs {
    const char *charset;
    sym *charnames;
    int start;
} charsets[] = {
    { "", NULL, 256 },
    { "iso-8859-1",	latin1_syms, 160 },
    { "iso-8859-2",	latin2_syms, 160 },
    { "iso-8859-3",	latin3_syms, 160 },
    { "iso-8859-4",	latin4_syms, 160 },
    { "iso-8859-5",	iso_8859_5_syms, 160 },
    { "iso-8859-7",	iso_8859_7_syms, 160 },
    { "iso-8859-8",	iso_8859_8_syms, 160 },
    { "iso-8859-9",	iso_8859_9_syms, 208 },
    { "iso-8859-10",	latin6_syms, 160 },
    { "iso-8859-15",	iso_8859_15_syms, 160 },
    { "mazovia",	mazovia_syms, 128 },
    { "cp-1250",	cp1250_syms, 128 },
    { "koi8-r",		koi8_syms, 128 },
    { "koi8-u",		koi8_syms, 128 },
    { "tis-620",	tis_620_syms, 160 },		/* thai */
    { "iso-10646-18",	iso_10646_18_syms, 159 },	/* ethiopic */
    { "iso-ir-197",	iso_ir_197_syms, 160 },		/* sami */
    { "iso-ir-209",	iso_ir_209_syms, 160 },		/* sami */
    /* When you add a new charset with a long (> 15 chars) name,
     * please update the chosen_charset definition below. */
};

/* Functions for both dumpkeys and loadkeys. */

int prefer_unicode = 0;
static char chosen_charset[16] = "";

void
list_charsets(FILE *f) {
	int lth,ct;
	unsigned int i, j;
	char *mm[] = { "iso-8859-", "koi8-" };

	for (j=0; j<sizeof(mm)/sizeof(mm[0]); j++) {
		if(j)
			fprintf(f, ",");
		fprintf(f, "%s{", mm[j]);
		ct = 0;
		lth = strlen(mm[j]);
		for(i=1; i < sizeof(charsets)/sizeof(charsets[0]); i++) {
			if(!strncmp(charsets[i].charset, mm[j], lth)) {
				if(ct++)
					fprintf(f, ",");
				fprintf(f, "%s", charsets[i].charset+lth);
			}
		}
		fprintf(f, "}");
	}
	for(i=1; i < sizeof(charsets)/sizeof(charsets[0]); i++) {
		for (j=0; j<sizeof(mm)/sizeof(mm[0]); j++) {
			lth = strlen(mm[j]);
			if(!strncmp(charsets[i].charset, mm[j], lth))
				goto nxti;
		}
		fprintf(f, ",%s", charsets[i].charset);
	nxti:;
	}
	fprintf(f, "\n");
}

int
set_charset(const char *charset) {
	sym *p;
	unsigned int i;

	for (i = 1; i < sizeof(charsets)/sizeof(charsets[0]); i++) {
		if (!strcasecmp(charsets[i].charset, charset)) {
			charsets[0].charset = charsets[i].charset;
			charsets[0].charnames = charsets[i].charnames;
			charsets[0].start = charsets[i].start;
			p = charsets[i].charnames;
			for (i = charsets[i].start; i < 256; i++,p++) {
				if(p->name[0])
					syms[0].table[i] = p->name;
			}
			strcpy(chosen_charset, charset);

			/* Unicode: The first 256 code points were made
			   identical to the content of ISO 8859-1 */
			if (prefer_unicode && !strcasecmp(charset, "iso-8859-1"))
				prefer_unicode = 0;

			return 0;
		}
	}
	fprintf (stderr, _("unknown charset %s - ignoring charset request\n"),
		 charset);
	return 1;
}

const char *
codetoksym(int code) {
	unsigned int i;
	int j;
	sym *p;

	if (code < 0)
		return NULL;

	if (code < 0x1000) {	/* "traditional" keysym */
		if (code < 0x80)
			return iso646_syms[code];
		if (KTYP(code) == KT_META)
			return NULL;
		if (KTYP(code) == KT_LETTER)
			code = K(KT_LATIN, KVAL(code));
		if (KTYP(code) > KT_LATIN)
			return syms[KTYP(code)].table[KVAL(code)];

		for (i = 0; i < sizeof(charsets)/sizeof(charsets[0]); i++) {
			p = charsets[i].charnames;
			if (!p)
				continue;
			p += KVAL(code) - charsets[i].start;
			if (p->name[0])
				return p->name;
		}
	}

	else {			/* Unicode keysym */
		code ^= 0xf000;

		if (code < 0x80)
			return iso646_syms[code];

		for (i = 0; i < sizeof(charsets)/sizeof(charsets[0]); i++) {
			p = charsets[i].charnames;
			if (!p)
				continue;
			for (j = charsets[i].start; j < 256; j++, p++) {
				if (p->uni == code && p->name[0])
					return p->name;
			}
		}
	}

	return NULL;
}

/* Functions for loadkeys. */

int
ksymtocode(const char *s, int direction) {
	unsigned int i;
	int j, jmax;
	int keycode;
	sym *p;

	if (direction == TO_AUTO)
		direction = prefer_unicode ? TO_UNICODE : TO_8BIT;

	if (!strncmp(s, "Meta_", 5)) {
		keycode = ksymtocode(s+5, TO_8BIT);
		if (KTYP(keycode) == KT_LATIN)
			return K(KT_META, KVAL(keycode));

		/* Avoid error messages for Meta_acute with UTF-8 */
		else if(direction == TO_UNICODE)
		        return (0);

		/* fall through to error printf */
	}

	for (i = 0; i < syms_size; i++) {
		jmax = ((i == 0 && direction == TO_UNICODE) ? 128 : syms[i].size);
		for (j = 0; j < jmax; j++)
			if (!strcmp(s,syms[i].table[j]))
				return K(i, j);
	}

	for (i = 0; i < syn_size; i++)
		if (!strcmp(s, synonyms[i].synonym))
			return ksymtocode(synonyms[i].official_name, direction);

	if (direction == TO_UNICODE) {
		for (i = 0; i < sizeof(charsets)/sizeof(charsets[0]); i++) {
			p = charsets[i].charnames;
			for (j = charsets[i].start; j < 256; j++, p++)
				if (!strcmp(s,p->name))
					return (p->uni ^ 0xf000);
		}
	} else /* if (!chosen_charset[0]) */ {
		/* note: some keymaps use latin1 but with euro,
		   so set_charset() would fail */
		/* note: some keymaps with charset line still use
		   symbols from more than one character set,
		   so we cannot have the  `if (!chosen_charset[0])'  here */

		for (i = 0; i < 256 - 160; i++)
			if (!strcmp(s, latin1_syms[i].name)) {
				fprintf(stderr,
					_("assuming iso-8859-1 %s\n"), s);
				return K(KT_LATIN, 160 + i);
			}

		for (i = 0; i < 256 - 160; i++)
			if (!strcmp(s, iso_8859_15_syms[i].name)) {
				fprintf(stderr,
					_("assuming iso-8859-15 %s\n"), s);
				return K(KT_LATIN, 160 + i);
			}

		for (i = 0; i < 256 - 160; i++)
			if (!strcmp(s, latin2_syms[i].name)) {
				fprintf(stderr,
					_("assuming iso-8859-2 %s\n"), s);
				return K(KT_LATIN, 160 + i);
			}

		for (i = 0; i < 256 - 160; i++)
			if (!strcmp(s, latin3_syms[i].name)) {
				fprintf(stderr,
					_("assuming iso-8859-3 %s\n"), s);
				return K(KT_LATIN, 160 + i);
			}

		for (i = 0; i < 256 - 160; i++)
			if (!strcmp(s, latin4_syms[i].name)) {
				fprintf(stderr,
					_("assuming iso-8859-4 %s\n"), s);
				return K(KT_LATIN, 160 + i);
			}
	}

	fprintf(stderr, _("unknown keysym '%s'\n"), s);

	return CODE_FOR_UNKNOWN_KSYM;
}

int
convert_code(int code, int direction)
{
	const char *ksym;
	int unicode_forced = (direction == TO_UNICODE);
	int input_is_unicode = (code >= 0x1000);
	int result;

	if (direction == TO_AUTO)
		direction = prefer_unicode ? TO_UNICODE : TO_8BIT;

	if (KTYP(code) == KT_META)
		return code;
	else if (!input_is_unicode && code < 0x80)
		/* basic ASCII is fine in every situation */
		return code;
	else if (input_is_unicode && (code ^ 0xf000) < 0x80)
		/* so is Unicode "Basic Latin" */
		return code ^ 0xf000;
	else if ((input_is_unicode && direction == TO_UNICODE) ||
		 (!input_is_unicode && direction == TO_8BIT))
		/* no conversion necessary */
		result = code;
	else {
		/* depending on direction, this will give us either an 8-bit
		 * K(KTYP, KVAL) or a Unicode keysym xor 0xf000 */
		ksym = codetoksym(code);
		if (ksym)
			result = ksymtocode(ksym, direction);
		else
			result = code;
	}

	/* if direction was TO_UNICODE from the beginning, we return the true
	 * Unicode value (without the 0xf000 mask) */
	if (unicode_forced && result >= 0x1000)
		return result ^ 0xf000;
	else
		return result;
}

int
add_capslock(int code)
{
	if (KTYP(code) == KT_LATIN && (!prefer_unicode || code < 0x80))
		return K(KT_LETTER, KVAL(code));
	else if ((code ^ 0xf000) < 0x100)
		/* Unicode Latin-1 Supplement */
		/* a bit dirty to use KT_LETTER here, but it should work */
		return K(KT_LETTER, code ^ 0xf000);
	else
		return convert_code(code, TO_AUTO);
}
