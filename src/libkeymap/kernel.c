/* kernel.c
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

#include "keymap.h"

#include "nls.h"
#include "contextP.h"

int
lk_kernel_keys(struct lk_ctx *ctx, int fd)
{
	int i, t;
	struct kbentry ke;

	for (t = 0; t < MAX_NR_KEYMAPS; t++) {
		for (i = 0; i < NR_KEYS; i++) {
			ke.kb_table = t;
			ke.kb_index = i;
			ke.kb_value = 0;

			if (ioctl(fd, KDGKBENT, (unsigned long) &ke)) {
				ERR(ctx, _("KDGKBENT: %s: error at index %d in table %d"),
					strerror(errno), i, t);
				return -1;
			}

			if (lk_add_key(ctx, t, i, ke.kb_value) < 0)
				return -1;
		}
	}

	if (lk_add_constants(ctx) < 0)
		return -1;

	return 0;
}

int
lk_kernel_funcs(struct lk_ctx *ctx, int fd)
{
	int i;
	struct kbsentry kbs;

	for (i = 0; i < MAX_NR_FUNC; i++) {
		kbs.kb_func = i;

		if (ioctl(fd, KDGKBSENT, (unsigned long) &kbs)) {
			ERR(ctx, _("KDGKBSENT: %s: Unable to get function key string"),
				strerror(errno));
			return -1;
		}

		if (!strlen((char *) kbs.kb_string))
			continue;

		if (lk_add_func(ctx, &kbs) < 0)
			return -1;
	}

	return 0;
}

int
lk_kernel_diacrs(struct lk_ctx *ctx, int fd)
{
#ifdef KDGKBDIACRUC
	int request = KDGKBDIACRUC;
	struct kbdiacrsuc kd;
	struct kbdiacruc *ar = kd.kbdiacruc;
#else
	int request = KDGKBDIACR;
	struct kbdiacrs kd;
	struct kbdiacr *ar = kd.kbdiacr;
#endif
	unsigned int i;
	struct lk_kbdiacr dcr;

	if (ioctl(fd, request, (unsigned long) &kd)) {
		ERR(ctx, _("KDGKBDIACR(UC): %s: Unable to get accent table"),
			strerror(errno));
		return -1;
	}

	for (i = 0; i < kd.kb_cnt; i++) {
		dcr.diacr  = (ar+i)->diacr;
		dcr.base   = (ar+i)->base;
		dcr.result = (ar+i)->result;

		if (lk_add_diacr(ctx, i, &dcr) < 0)
			return -1;
	}

	return 0;
}

int
lk_kernel_keymap(struct lk_ctx *ctx, int fd)
{
	if (lk_kernel_keys(ctx, fd)   < 0 ||
	    lk_kernel_funcs(ctx, fd)  < 0 ||
	    lk_kernel_diacrs(ctx, fd) < 0)
		return -1;
	return 0;
}
