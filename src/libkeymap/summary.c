/* summary.c
 *
 * This file is part of kbd project.
 * Copyright (C) 2012-2013  Alexey Gladkov <gladkov.alexey@gmail.com>
 *
 * This file is covered by the GNU General Public License,
 * which should be included with kbd as the file COPYING.
 */
#include <string.h>
#include <errno.h>

#include <sys/ioctl.h>

#include "ksyms.h"
#include "modifiers.h"

#include "nls.h"
#include "keymap.h"

static char
valid_type(int fd, int t)
{
	struct kbentry ke;

	ke.kb_index = 0;
	ke.kb_table = 0;
	ke.kb_value = K(t, 0);

	return (ioctl(fd, KDSKBENT, (unsigned long) &ke) == 0);
}

static u_char
maximum_val(int fd, int t)
{
	struct kbentry ke, ke0;
	int i;

	ke.kb_index = 0;
	ke.kb_table = 0;
	ke.kb_value = K_HOLE;
	ke0 = ke;

	ioctl(fd, KDGKBENT, (unsigned long) &ke0);

	for (i = 0; i < 256; i++) {
		ke.kb_value = K(t, i);
		if (ioctl(fd, KDSKBENT, (unsigned long) &ke))
			break;
	}
	ke.kb_value = K_HOLE;
	ioctl(fd, KDSKBENT, (unsigned long) &ke0);

	return i - 1;
}

#define NR_TYPES 15

void
lk_dump_summary(struct keymap *kmap, FILE *fd, int console)
{
	int i, allocct = 0;

	for (i = 0; i < MAX_NR_KEYMAPS; i++) {
		if (lk_get_key(kmap, i, 0) == K_ALLOCATED)
			allocct++;
	}

	fprintf(fd, _("keycode range supported by kernel:           1 - %d\n"),
		NR_KEYS - 1);
	fprintf(fd, _("max number of actions bindable to a key:         %d\n"),
		MAX_NR_KEYMAPS);
	fprintf(fd, _("number of keymaps in actual use:                 %u\n"),
		(unsigned int) kmap->keymap->count);

	if (allocct)
		fprintf(fd, _("of which %d dynamically allocated\n"), allocct);

	fprintf(fd, _("ranges of action codes supported by kernel:\n"));

	for (i = 0; i < NR_TYPES && valid_type(console, i); i++)
		fprintf(fd, "	0x%04x - 0x%04x\n",
			K(i, 0), K(i, maximum_val(console, i)));

	fprintf(fd, _("number of function keys supported by kernel: %d\n"),
		MAX_NR_FUNC);
	fprintf(fd, _("max nr of compose definitions: %d\n"),
		MAX_DIACR);
	fprintf(fd, _("nr of compose definitions in actual use: %d\n"),
		kmap->accent_table_size);
}

void
lk_dump_symbols(FILE *fd)
{
	unsigned int t;
	modifier_t *mod;
	int v;
	const char *p;

	for (t = 0; t < syms_size; t++) {
	    if (syms[t].size) {
		for (v = 0; v < syms[t].size; v++) {
			if ((p = syms[t].table[v])[0])
				fprintf(fd, "0x%04x\t%s\n", K(t, v), p);
		}
	    } else if (t == KT_META) {
		for (v = 0; v < syms[0].size && v < 128; v++) {
			if ((p = syms[0].table[v])[0])
				fprintf(fd, "0x%04x\tMeta_%s\n", K(t, v), p);
		}
	    }
	}

	fprintf(fd, _("\nThe following synonyms are recognized:\n\n"));

	for (t = 0; t < syn_size; t++) {
		fprintf(fd, _("%-15s for %s\n"),
			synonyms[t].synonym, synonyms[t].official_name);
	}

	fprintf(fd, _("\nRecognized modifier names and their column numbers:\n"));

	mod = (modifier_t *) modifiers;
	while (mod->name) {
		fprintf(fd, "%s\t\t%3d\n", mod->name, 1 << mod->bit);
		mod++;
	}
}
