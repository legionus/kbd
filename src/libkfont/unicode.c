#include <stdlib.h>
#include <errno.h>

#include "unicode.h"

int
kfont_addpair(struct unicode_list *up, unicode uc)
{
	struct unicode_list *ul = NULL;
	struct unicode_seq *us = NULL;

	if (!(ul = malloc(sizeof(*ul))))
		goto err;

	if (!(us = malloc(sizeof(*us))))
		goto err;

	us->uc = uc;
	us->prev = us;
	us->next = NULL;

	ul->seq = us;
	ul->prev = up->prev;
	ul->prev->next = ul;
	ul->next = NULL;

	up->prev = ul;

	return 0;
err:
	free(ul);
	free(us);

	return -ENOMEM;
}

int
kfont_addseq(struct unicode_list *up, unicode uc)
{
	struct unicode_list *ul = up->prev;
	struct unicode_seq *usl = ul->seq;
	struct unicode_seq *us = NULL;

	if (!(us = malloc(sizeof(*us))))
		return -ENOMEM;

	while (usl->next)
		usl = usl->next;

	us->uc = uc;
	us->prev = usl;
	us->next = NULL;

	usl->next = us;
	ul->seq->prev = us;

	return 0;
}

void
kfont_clear_uni_entry(struct unicode_list *up)
{
	up->next = NULL;
	up->seq  = NULL;
	up->prev = up;
}
