#ifndef LK_LOGGING_H
#define LK_LOGGING_H

#include <syslog.h>
#include <keymap/data.h>

void lk_log(struct keymap *kmap, int priority,
            const char *file, int line, const char *fn,
            const char *fmt, ...);

#define lk_log_cond(kmap, level, arg...) \
	do { \
		if (kmap->log_priority >= level) \
			lk_log(kmap, level, __FILE__, __LINE__, __func__, ## arg);\
	} while (0)

#define DBG(kmap, arg...)  lk_log_cond(kmap, LOG_DEBUG,   ## arg)
#define INFO(kmap, arg...) lk_log_cond(kmap, LOG_INFO,    ## arg)
#define WARN(kmap, arg...) lk_log_cond(kmap, LOG_WARNING, ## arg)
#define ERR(kmap, arg...)  lk_log_cond(kmap, LOG_ERR,     ## arg)

#endif /* LK_LOGGING_H */
