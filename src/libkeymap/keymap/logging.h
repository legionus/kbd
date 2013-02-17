#ifndef LK_LOGGING_H
#define LK_LOGGING_H

#define log_error(kmap, arg...) \
	kmap->log_error(__FILE__, __LINE__, __func__, ## arg)

#define log_verbose(kmap, level, arg...) \
	do { \
		if (kmap->verbose >= level) \
			kmap->log_message(__FILE__, __LINE__, __func__, ## arg); \
	} while(0)

#endif /* LK_LOGGING_H */