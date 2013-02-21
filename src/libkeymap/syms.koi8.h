/* For koi8-r, see rfc1489.txt */
/* For koi8-u, see rfc2319.txt
   it is identical to koi8-r with 8 exceptions */
/* Since koi8-r did not have letters on these 8 places,
   we can use the same table for both. */
/* TODO: check unicode values for these 8 positions */

static sym
const koi8_syms[] = { /* 128-255 */
	{ 0x2500, "" },                        /* 0200 */
	{ 0x2502, "" },
	{ 0x250c, "" },
	{ 0x2510, "" },
	{ 0x2514, "" },
	{ 0x2518, "" },
	{ 0x251c, "" },
	{ 0x2524, "" },
	{ 0x252c, "" },                        /* 0210 */
	{ 0x2534, "" },
	{ 0x253c, "" },
	{ 0x2580, "" },
	{ 0x2584, "" },
	{ 0x2588, "" },
	{ 0x258c, "" },
	{ 0x2590, "" },
	{ 0x2591, "" },                        /* 0220 */
	{ 0x2592, "" },
	{ 0x2593, "" },
	{ 0x2320, "" },
	{ 0x25a0, "" },
	{ 0x2219, "" },
	{ 0x221a, "" },
	{ 0x2248, "" },
	{ 0x2264, "" },                        /* 0230 */
	{ 0x2265, "" },
	{ 0x00a0, "" },
	{ 0x2321, "" },
	{ 0x00b0, "" },
	{ 0x00b2, "" },
	{ 0x00b7, "" },
	{ 0x00f7, "" },
	{ 0x2550, "" },                        /* 0240 */
	{ 0x2551, "" },
	{ 0x2552, "" },
	{ 0x0451, "cyrillic_small_letter_io" },
	{ 0x2553, "ukrainian_cyrillic_small_letter_ie" },  /* koi8-u #164 */
	{ 0x2554, "" },
	{ 0x2555, "ukrainian_cyrillic_small_letter_i" },   /* koi8-u #166 */
	{ 0x2556, "ukrainian_cyrillic_small_letter_yi" },  /* koi8-u #167 */
	{ 0x2557, "" },                        /* 0250 */
	{ 0x2558, "" },
	{ 0x2559, "" },
	{ 0x255a, "" },
	{ 0x255b, "" },
	{ 0x255c, "ukrainian_cyrillic_small_letter_ghe_with_upturn" }, /* koi8-u #173 */
	{ 0x255d, "" },
	{ 0x255e, "" },
	{ 0x255f, "" },                        /* 0260 */
	{ 0x2560, "" },
	{ 0x2561, "" },
	{ 0x0401, "cyrillic_capital_letter_io" },
	{ 0x2562, "ukrainian_cyrillic_capital_letter_ie" },/* koi8-u #180 */
	{ 0x2563, "" },
	{ 0x2564, "ukrainian_cyrillic_capital_letter_i" }, /* koi8-u #182 */
	{ 0x2565, "ukrainian_cyrillic_capital_letter_yi" },/* koi8-u #183 */
	{ 0x2566, "" },                        /* 0270 */
	{ 0x2567, "" },
	{ 0x2568, "" },
	{ 0x2569, "" },
	{ 0x256a, "" },
	{ 0x256b, "ukrainian_cyrillic_capital_letter_ghe_with_upturn" },/* koi8-u #189 */
	{ 0x256c, "" },
	{ 0x00a9, "copyright" },
	{ 0x044e, "cyrillic_small_letter_yu" },                        /* 0300 */
	{ 0x0430, "cyrillic_small_letter_a" },
	{ 0x0431, "cyrillic_small_letter_be" },
	{ 0x0446, "cyrillic_small_letter_tse" },
	{ 0x0434, "cyrillic_small_letter_de" },
	{ 0x0435, "cyrillic_small_letter_ie" },
	{ 0x0444, "cyrillic_small_letter_ef" },
	{ 0x0433, "cyrillic_small_letter_ghe" },
	{ 0x0445, "cyrillic_small_letter_ha" },                        /* 0310 */
	{ 0x0438, "cyrillic_small_letter_i" },
	{ 0x0439, "cyrillic_small_letter_short_i" },
	{ 0x043a, "cyrillic_small_letter_ka" },
	{ 0x043b, "cyrillic_small_letter_el" },
	{ 0x043c, "cyrillic_small_letter_em" },
	{ 0x043d, "cyrillic_small_letter_en" },
	{ 0x043e, "cyrillic_small_letter_o" },
	{ 0x043f, "cyrillic_small_letter_pe" },                                /* 0320 */
	{ 0x044f, "cyrillic_small_letter_ya" },
	{ 0x0440, "cyrillic_small_letter_er" },
	{ 0x0441, "cyrillic_small_letter_es" },
	{ 0x0442, "cyrillic_small_letter_te" },
	{ 0x0443, "cyrillic_small_letter_u" },
	{ 0x0436, "cyrillic_small_letter_zhe" },
	{ 0x0432, "cyrillic_small_letter_ve" },
	{ 0x044c, "cyrillic_small_soft_sign" },                                /* 0330 */
	{ 0x044b, "cyrillic_small_letter_yeru" },
	{ 0x0437, "cyrillic_small_letter_ze" },
	{ 0x0448, "cyrillic_small_letter_sha" },
	{ 0x044d, "cyrillic_small_letter_e" },
	{ 0x0449, "cyrillic_small_letter_shcha" },
	{ 0x0447, "cyrillic_small_letter_che" },
	{ 0x044a, "cyrillic_small_hard_sign" },
	{ 0x042e, "cyrillic_capital_letter_yu" },                      /* 0340 */
	{ 0x0410, "cyrillic_capital_letter_a" },
	{ 0x0411, "cyrillic_capital_letter_be" },
	{ 0x0426, "cyrillic_capital_letter_tse" },
	{ 0x0414, "cyrillic_capital_letter_de" },
	{ 0x0415, "cyrillic_capital_letter_ie" },
	{ 0x0424, "cyrillic_capital_letter_ef" },
	{ 0x0413, "cyrillic_capital_letter_ghe" },
	{ 0x0425, "cyrillic_capital_letter_ha" },                      /* 0350 */
	{ 0x0418, "cyrillic_capital_letter_i" },
	{ 0x0419, "cyrillic_capital_letter_short_i" },
	{ 0x041a, "cyrillic_capital_letter_ka" },
	{ 0x041b, "cyrillic_capital_letter_el" },
	{ 0x041c, "cyrillic_capital_letter_em" },
	{ 0x041d, "cyrillic_capital_letter_en" },
	{ 0x041e, "cyrillic_capital_letter_o" },
	{ 0x041f, "cyrillic_capital_letter_pe" },                      /* 0360 */
	{ 0x042f, "cyrillic_capital_letter_ya" },
	{ 0x0420, "cyrillic_capital_letter_er" },
	{ 0x0421, "cyrillic_capital_letter_es" },
	{ 0x0422, "cyrillic_capital_letter_te" },
	{ 0x0423, "cyrillic_capital_letter_u" },
	{ 0x0416, "cyrillic_capital_letter_zhe" },
	{ 0x0412, "cyrillic_capital_letter_ve" },
	{ 0x042c, "cyrillic_capital_soft_sign" },                      /* 0370 */
	{ 0x042b, "cyrillic_capital_letter_yeru" },
	{ 0x0417, "cyrillic_capital_letter_ze" },
	{ 0x0428, "cyrillic_capital_letter_sha" },
	{ 0x042d, "cyrillic_capital_letter_e" },
	{ 0x0429, "cyrillic_capital_letter_shcha" },
	{ 0x0427, "cyrillic_capital_letter_che" },
	{ 0x042a, "cyrillic_capital_hard_sign" }
};
