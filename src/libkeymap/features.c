#include "config.h"

#include <errno.h>
#include <linux/kd.h>
#include <linux/keyboard.h>
#include <string.h>

#include "contextP.h"

#define KT_CSI_TYPE 15

static const char *const feature_names[LK_FEATURE_COUNT] = {
	[LK_FEATURE_KT_CSI] = "KT_CSI",
};

static int
find_unused_keymap(struct lk_ctx *ctx, int console_fd, struct kbentry *entry)
{
	entry->kb_index = 0;

	for (int table = 1; table < MAX_NR_KEYMAPS; table++) {
		entry->kb_table = (unsigned char)table;
		if (lk_ioctl_ptr(ctx, console_fd, KDGKBENT, entry))
			return -1;
		if (entry->kb_value == K_NOSUCHMAP)
			return 0;
	}

	errno = ENOSPC;
	return -1;
}

static enum lk_feature_status
check_kt_csi(struct lk_ctx *ctx, int console_fd)
{
	struct kbentry probe;
	int kbd_mode;
	int cleanup_errno;
	int probe_errno;

	if (find_unused_keymap(ctx, console_fd, &probe))
		return LK_FEATURE_UNAVAILABLE;

	if (lk_ioctl_ptr(ctx, console_fd, KDGKBMODE, &kbd_mode))
		return LK_FEATURE_UNAVAILABLE;

	/*
	 * Old kernels accept unknown action types in Unicode mode. Switch to
	 * K_XLATE so KDSKBENT validates KT_CSI against the handler table.
	 */
	if (kbd_mode != K_XLATE &&
	    lk_ioctl_int(ctx, console_fd, KDSKBMODE, K_XLATE))
		return LK_FEATURE_UNAVAILABLE;

	probe.kb_value = K(KT_CSI_TYPE, 0);
	if (lk_ioctl_ptr(ctx, console_fd, KDSKBENT, &probe))
		probe_errno = errno;
	else
		probe_errno = 0;

	/* Always discard the temporary map before returning to the caller. */
	probe.kb_value = K_NOSUCHMAP;
	if (lk_ioctl_ptr(ctx, console_fd, KDSKBENT, &probe))
		cleanup_errno = errno;
	else
		cleanup_errno = 0;

	if (kbd_mode != K_XLATE &&
	    lk_ioctl_int(ctx, console_fd, KDSKBMODE, kbd_mode))
		return LK_FEATURE_UNAVAILABLE;

	if (cleanup_errno) {
		errno = cleanup_errno;
		return LK_FEATURE_UNAVAILABLE;
	}

	if (!probe_errno)
		return LK_FEATURE_SUPPORTED;
	if (probe_errno == EINVAL)
		return LK_FEATURE_UNSUPPORTED;

	return LK_FEATURE_UNAVAILABLE;
}

int
lk_check_features(struct lk_ctx *ctx, int console_fd)
{
	if (!ctx)
		return -1;

	ctx->features[LK_FEATURE_KT_CSI] = check_kt_csi(ctx, console_fd);

	return ctx->features[LK_FEATURE_KT_CSI] == LK_FEATURE_UNAVAILABLE ? -1 : 0;
}

enum lk_feature_status
lk_get_feature_status(const struct lk_ctx *ctx, enum lk_feature feature)
{
	if (!ctx || feature < 0 || feature >= LK_FEATURE_COUNT)
		return LK_FEATURE_UNAVAILABLE;

	return ctx->features[feature];
}

enum lk_feature
lk_feature_from_name(const char *name)
{
	if (!name)
		return LK_FEATURE_COUNT;

	for (int feature = 0; feature < LK_FEATURE_COUNT; feature++) {
		if (!strcmp(name, feature_names[feature]))
			return feature;
	}

	return LK_FEATURE_COUNT;
}

const char *
lk_feature_name(enum lk_feature feature)
{
	if (feature < 0 || feature >= LK_FEATURE_COUNT)
		return NULL;

	return feature_names[feature];
}
