// SPDX-License-Identifier: LGPL-2.0-or-later
/**
 * @file kbdfile.h
 * @brief Functions for search, open and close a file objects.
 */
#ifndef _KBD_LIBKBDFILE_KBDFILE_H_
#define _KBD_LIBKBDFILE_KBDFILE_H_

#include <stdio.h>
#include <stdarg.h>
#include <sys/param.h>

#include <kbd/compiler_attributes.h>

typedef void (*kbdfile_logger_t)(void *, int, const char *, int, const char *, const char *, va_list)
	KBD_ATTR_PRINTF(6, 0);

struct kbdfile_ctx;

struct kbdfile_ctx *kbdfile_context_new(void);
void *kbdfile_context_free(struct kbdfile_ctx *ctx);

kbdfile_logger_t kbdfile_get_log_fn(struct kbdfile_ctx *ctx);
int kbdfile_set_log_fn(struct kbdfile_ctx *ctx, kbdfile_logger_t log_fn, const void *data);

void *kbdfile_get_log_data(struct kbdfile_ctx *ctx);
int kbdfile_set_log_data(struct kbdfile_ctx *ctx, const void *data);

int kbdfile_get_log_priority(struct kbdfile_ctx *ctx);
int kbdfile_set_log_priority(struct kbdfile_ctx *ctx, int priority);

struct kbdfile;

struct kbdfile *kbdfile_new(struct kbdfile_ctx *ctx);
void kbdfile_free(struct kbdfile *fp);

struct kbdfile *kbdfile_open(struct kbdfile_ctx *ctx, const char *filename);
void kbdfile_close(struct kbdfile *fp);

int kbdfile_find(const char *fnam, const char *const *dirpath, const char *const *suffixes, struct kbdfile *fp);

char *kbdfile_get_pathname(struct kbdfile *fp);
int kbdfile_set_pathname(struct kbdfile *fp, const char *pathname);

FILE *kbdfile_get_file(struct kbdfile *fp);
int kbdfile_set_file(struct kbdfile *fp, FILE *x);

int kbdfile_is_compressed(struct kbdfile *fp);

#include <syslog.h>

void
kbdfile_log(struct kbdfile_ctx *ctx, int priority,
	    const char *file, int line, const char *fn,
	    const char *fmt, ...)
	KBD_ATTR_PRINTF(6, 7);

#endif /* _KBD_LIBKBDFILE_KBDFILE_H_ */
