#include "config.h"

#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <linux/keyboard.h>
#include <unistd.h>

#include "keymap.h"

#include "contextP.h"
#include "ksyms.h"

static int set_keymap_table(struct lk_ctx *ctx, int fd, int k_table)
{
	struct kbentry ke;
	int k_index, value, count, fail;

	count = 0;

	for (k_index = 0; k_index < NR_KEYS; k_index++) {
		if (!lk_key_exists(ctx, k_table, k_index))
			continue;

		value = lk_get_key(ctx, k_table, k_index);

		if (value < 0 || value > USHRT_MAX) {
			WARN(ctx, _("can not bind key %d to value %d because it is too large"),
					k_index, value);
			continue;
		}

		ke.kb_table = (unsigned char) k_table;
		ke.kb_index = (unsigned char) k_index;
		ke.kb_value = (unsigned short) value;

		fail = ioctl(fd, KDSKBENT, &ke);

		if (fail) {
			if (errno == EPERM) {
				ERR(ctx, _("Keymap %d: Permission denied"),
						k_table);
				return -1;
			}
			ERR(ctx, "%s", strerror(errno));

			/*
			 * Such an error can be returned if the tty has been
			 * hungup while loadkeys is running.
			 */
			if (errno == EIO)
				return -1;
		} else {
			count++;
		}

		INFO(ctx, _("keycode %d, table %d = %d%s"), k_index, k_table, ke.kb_value,
				fail ? _("    FAILED") : "");

		if (fail)
			WARN(ctx, _("failed to bind key %d to value %d"),
					k_index, ke.kb_value);
	}

	return count;
}

static int clear_keymap_table(struct lk_ctx *ctx, int fd, int k_table)
{
	struct kbentry ke;

	/* deallocate keymap */
	ke.kb_table = (unsigned char) k_table;
	ke.kb_index = 0;
	ke.kb_value = K_NOSUCHMAP;

	DBG(ctx, _("deallocate keymap %d"), k_table);

	if (!ioctl(fd, KDSKBENT, &ke))
		return 0;

	if (errno != EINVAL) {
		ERR(ctx, _("KDSKBENT: %s: could not deallocate keymap %d"),
				strerror(errno), k_table);
		return -1;
	}

	/*
	 * probably an old kernel. clear keymap by hand.
	 */
	for (int k_index = 0; k_index < NR_KEYS; k_index++) {
		ke.kb_table = (unsigned char) k_table;
		ke.kb_index = (unsigned char) k_index;
		ke.kb_value = K_HOLE;

		if (ioctl(fd, KDSKBENT, &ke)) {
			if (errno == EINVAL && k_table >= 16)
				break; /* old kernel */

			ERR(ctx, _("KDSKBENT: %s: cannot deallocate or clear keymap"),
					strerror(errno));
			return -1;
		}
	}

	return 0;
}

static int set_keymap(struct lk_ctx *ctx, int fd, int kbd_mode)
{
	int count = 0;

	if (ctx->flags & LK_FLAG_UNICODE_MODE) {
		/* temporarily switch to K_UNICODE while defining keys */
		if (ioctl(fd, KDSKBMODE, K_UNICODE)) {
			ERR(ctx, _("KDSKBMODE: %s: could not switch to Unicode mode"),
					strerror(errno));
			return -1;
		}
	}

	for (int k_table = 0; k_table < MAX_NR_KEYMAPS; k_table++) {
		if (lk_map_exists(ctx, k_table)) {
			int ct = set_keymap_table(ctx, fd, k_table);

			if (ct < 0) {
				count = -1;
				break;
			}
			count += ct;

		} else if (ctx->keywords & LK_KEYWORD_KEYMAPS) {
			if (clear_keymap_table(ctx, fd, k_table) < 0) {
				count = -1;
				break;
			}
		}
	}

	if ((ctx->flags & LK_FLAG_UNICODE_MODE) && ioctl(fd, KDSKBMODE, kbd_mode)) {
		ERR(ctx, _("KDSKBMODE: %sr could not return to original keyboard mode"),
				strerror(errno));
		return -1;
	}

	return count;
}

static char *
ostr(struct lk_ctx *ctx, char *s)
{
	size_t lth = strlen(s);
	char *ns0  = malloc(4 * lth + 1);
	char *ns   = ns0;

	if (ns == NULL) {
		ERR(ctx, _("out of memory"));
		return NULL;
	}

	while (*s) {
		switch (*s) {
			case '\n':
				*ns++ = '\\';
				*ns++ = 'n';
				break;
			case '\033':
				*ns++ = '\\';
				*ns++ = '0';
				*ns++ = '3';
				*ns++ = '3';
				break;
			default:
				*ns++ = *s;
		}
		s++;
	}
	*ns = 0;
	return ns0;
}

static int
deffuncs(struct lk_ctx *ctx, int fd)
{
	unsigned int i;
	int ct = 0;
	char *ptr, *s;
	struct kbsentry kbs;

	for (i = 0; i < MAX_NR_FUNC; i++) {
		kbs.kb_func = (unsigned char) i;

		ptr = lk_array_get_ptr(ctx->func_table, i);

		if (ptr) {
			strlcpy((char *)kbs.kb_string, ptr, sizeof(kbs.kb_string));

			if (ioctl(fd, KDSKBSENT, &kbs)) {
				s = ostr(ctx, (char *)kbs.kb_string);
				if (s == NULL)
					return -1;
				ERR(ctx, _("failed to bind string '%s' to function %s"),
				    s, get_sym(ctx, KT_FN, kbs.kb_func));
				free(s);
			} else {
				ct++;
			}
		} else if (ctx->flags & LK_FLAG_CLEAR_STRINGS) {
			kbs.kb_string[0] = 0;

			if (ioctl(fd, KDSKBSENT, &kbs)) {
				ERR(ctx, _("failed to clear string %s"),
				    get_sym(ctx, KT_FN, kbs.kb_func));
			} else {
				ct++;
			}
		}
	}
	return ct;
}

static int
defdiacs(struct lk_ctx *ctx, int fd)
{
	unsigned int i, j, count;
	struct lk_kbdiacr *ptr;

	if (ctx->accent_table->count > MAX_DIACR) {
		count = MAX_DIACR;
		ERR(ctx, _("too many compose definitions"));
	} else {
		count = (unsigned int) ctx->accent_table->count;
	}
#ifdef KDSKBDIACRUC
	if (ctx->flags & LK_FLAG_PREFER_UNICODE) {
		struct kbdiacrsuc kdu;

		kdu.kb_cnt = count;

		for (i = 0, j = 0; i < ctx->accent_table->total && j < count; i++) {
			ptr = lk_array_get_ptr(ctx->accent_table, i);
			if (!ptr)
				continue;

			kdu.kbdiacruc[j].diacr  = ptr->diacr;
			kdu.kbdiacruc[j].base   = ptr->base;
			kdu.kbdiacruc[j].result = ptr->result;
			j++;
		}

		if (ioctl(fd, KDSKBDIACRUC, &kdu)) {
			ERR(ctx, "KDSKBDIACRUC: %s", strerror(errno));
			return -1;
		}
	} else
#endif
	{
		struct kbdiacrs kd;

		kd.kb_cnt = count;

		for (i = 0, j = 0; i < ctx->accent_table->total && j < count; i++) {
			ptr = lk_array_get_ptr(ctx->accent_table, i);
			if (!ptr)
				continue;

			if (ptr->diacr > UCHAR_MAX ||
			    ptr->base > UCHAR_MAX ||
			    ptr->result > UCHAR_MAX) {
				ERR(ctx, _("unable to load compose definitions because some of them are too large"));
				return -1;
			}

			kd.kbdiacr[j].diacr  = (unsigned char) ptr->diacr;
			kd.kbdiacr[j].base   = (unsigned char) ptr->base;
			kd.kbdiacr[j].result = (unsigned char) ptr->result;
			j++;
		}

		if (ioctl(fd, KDSKBDIACR, &kd)) {
			ERR(ctx, "KDSKBDIACR: %s", strerror(errno));
			return -1;
		}
	}

	return (int) count;
}

int lk_load_keymap(struct lk_ctx *ctx, int fd, int kbd_mode)
{
	int keyct, funcct, diacct;

	if (lk_add_constants(ctx) < 0)
		return -1;

	if ((keyct = set_keymap(ctx, fd, kbd_mode)) < 0 || (funcct = deffuncs(ctx, fd)) < 0)
		return -1;

	INFO(ctx, P_("\nChanged %d key", "\nChanged %d keys", (unsigned int) keyct), keyct);
	INFO(ctx, P_("Changed %d string", "Changed %d strings", (unsigned int) funcct), funcct);

	if (ctx->accent_table->count > 0 || ctx->flags & LK_FLAG_CLEAR_COMPOSE) {
		diacct = defdiacs(ctx, fd);

		if (diacct < 0)
			return -1;

		INFO(ctx, P_("Loaded %d compose definition",
		             "Loaded %d compose definitions", (unsigned int) diacct),
		     diacct);

	} else {
		INFO(ctx, _("(No change in compose definitions)"));
	}

	return 0;
}
