/*
 * cp1250 symbols, aeb, 2000-12-29
 * first half as usual; last quarter identical to latin-2 (iso 8859-2)
 *
 * No names have been introduced yet for the various quotation marks
 */
static sym cp1250_syms[] = {	/* 128-255 */
	{ 0x20ac, "euro"},	/* 0200 */
	{ 0xfffd, ""},
	{ 0x201a, ""},	/* SINGLE LOW-9 QUOTATION MARK */
	{ 0xfffd, ""},
	{ 0x201e, ""},	/* DOUBLE LOW-9 QUOTATION MARK */
	{ 0x2026, "ellipsis"},
	{ 0x2020, "dagger"},
	{ 0x2021, "doubledagger"},
	{ 0xfffd, ""},		/* 0210 */
	{ 0x2030, "permille"},
	{ 0x0160, "Scaron"},
	{ 0x2039, ""},	/* SINGLE LEFT-POINTING ANGLE QUOTATION MARK */
	{ 0x015a, "Sacute"},
	{ 0x0164, "Tcaron"},
	{ 0x017d, "Zcaron"},
	{ 0x0179, "Zacute"},
	{ 0xfffd, ""},		/* 0220 */
	{ 0x2018, ""},	/* LEFT SINGLE QUOTATION MARK */
	{ 0x2019, ""},	/* RIGHT SINGLE QUOTATION MARK */
	{ 0x201c, ""},	/* LEFT DOUBLE QUOTATION MARK */
	{ 0x201d, ""},	/* RIGHT DOUBLE QUOTATION MARK */
	{ 0x2022, "bullet"},
	{ 0x2013, "endash"},
	{ 0x2014, "emdash"},
	{ 0xfffd, ""},		/* 0230 */
	{ 0x2122, "trademark"},
	{ 0x0161, "scaron"},
	{ 0x203a, ""},	/* SINGLE RIGHT-POINTING ANGLE QUOTATION MARK */
	{ 0x015b, "sacute"},
	{ 0x0165, "tcaron"},
	{ 0x017e, "zcaron"},
	{ 0x017a, "zacute"},
	{ 0x00a0, ""},		/* 0240 */
	{ 0x02c7, "caron"},
	{ 0x02d8, "breve"},
	{ 0x0141, "Lstroke"},
	{ 0x00a4, ""},
	{ 0x0104, "Aogonek"},
	{ 0x00a6, ""},
	{ 0x00a7, ""},
	{ 0x00a8, ""},		/* 0250 */
	{ 0x00a9, ""},
	{ 0x015e, "Scedilla"},
	{ 0x00ab, ""},
	{ 0x00ac, ""},
	{ 0x00ad, ""},
	{ 0x00ae, ""},
	{ 0x017b, "Zabovedot"},
	{ 0x00b0, ""},		/* 0260 */
	{ 0x00b1, ""},
	{ 0x02db, "ogonek"},
	{ 0x0142, "lstroke"},
	{ 0x00b4, ""},
	{ 0x00b5, ""},
	{ 0x00b6, ""},
	{ 0x00b7, ""},
	{ 0x00b8, ""},		/* 0270 */
	{ 0x0105, "aogonek"},
	{ 0x015f, "scedilla"},
	{ 0x00bb, ""},
	{ 0x013d, "Lcaron"},
	{ 0x02dd, "doubleacute"},
	{ 0x013e, "lcaron"},
/* from here identical to latin-2 */
	{ 0x017c, "zabovedot"},
	{ 0x0154, "Racute"},	/* 0300 */
	{ 0x00c1, "Aacute"},
	{ 0x00c2, "Acircumflex"},
	{ 0x0102, "Abreve"},
	{ 0x00c4, "Adiaeresis"},
	{ 0x0139, "Lacute"},
	{ 0x0106, "Cacute"},
	{ 0x00c7, "Ccedilla"},
	{ 0x010c, "Ccaron"},	/* 0310 */
	{ 0x00c9, "Eacute"},
	{ 0x0118, "Eogonek"},
	{ 0x00cb, "Ediaeresis"},
	{ 0x011a, "Ecaron"},
	{ 0x00cd, "Iacute"},
	{ 0x00ce, "Icircumflex"},
	{ 0x010e, "Dcaron"},
	{ 0x0110, "Dstroke"},	/* 0320 */
	{ 0x0143, "Nacute"},
	{ 0x0147, "Ncaron"},
	{ 0x00d3, "Oacute"},
	{ 0x00d4, "Ocircumflex"},
	{ 0x0150, "Odoubleacute"},
	{ 0x00d6, "Odiaeresis"},
	{ 0x00d7, "multiply"},
	{ 0x0158, "Rcaron"},	/* 0330 */
	{ 0x016e, "Uring"},
	{ 0x00da, "Uacute"},
	{ 0x0170, "Udoubleacute"},
	{ 0x00dc, "Udiaeresis"},
	{ 0x00dd, "Yacute"},
	{ 0x0162, "Tcedilla"},
	{ 0x00df, "ssharp"},
	{ 0x0155, "racute"},	/* 0340 */
	{ 0x00e1, "aacute"},
	{ 0x00e2, "acircumflex"},
	{ 0x0103, "abreve"},
	{ 0x00e4, "adiaeresis"},
	{ 0x013a, "lacute"},
	{ 0x0107, "cacute"},
	{ 0x00e7, "ccedilla"},
	{ 0x010d, "ccaron"},	/* 0350 */
	{ 0x00e9, "eacute"},
	{ 0x0119, "eogonek"},
	{ 0x00eb, "ediaeresis"},
	{ 0x011b, "ecaron"},
	{ 0x00ed, "iacute"},
	{ 0x00ee, "icircumflex"},
	{ 0x010f, "dcaron"},
	{ 0x0111, "dstroke"},	/* 0360 */
	{ 0x0144, "nacute"},
	{ 0x0148, "ncaron"},
	{ 0x00f3, "oacute"},
	{ 0x00f4, "ocircumflex"},
	{ 0x0151, "odoubleacute"},
	{ 0x00f6, "odiaeresis"},
	{ 0x00f7, "division"},
	{ 0x0159, "rcaron"},	/* 0370 */
	{ 0x016f, "uring"},
	{ 0x00fa, "uacute"},
	{ 0x0171, "udoubleacute"},
	{ 0x00fc, "udiaeresis"},
	{ 0x00fd, "yacute"},
	{ 0x0163, "tcedilla"},
	{ 0x02d9, "abovedot"},
};
