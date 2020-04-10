#ifndef _UNICODE_H_
#define _UNICODE_H_

#include <stdint.h>

typedef int32_t unicode;

struct unicode_seq {
	struct unicode_seq *next;
	struct unicode_seq *prev;
	unicode uc;
};

struct unicode_list {
	struct unicode_list *next;
	struct unicode_list *prev;
	struct unicode_seq *seq;
};

int addpair(struct unicode_list *up, unicode uc);
int addseq(struct unicode_list *up, unicode uc);
void clear_uni_entry(struct unicode_list *up);

#endif /* _UNICODE_H_ */
