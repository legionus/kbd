#ifndef _XMALLOC_H
#define _XMALLOC_H

/* Error-free versions of some libc routines */
extern void *xmalloc(size_t sz);
extern void *xrealloc(void *p, size_t sz);
extern char *xstrdup(char *p);
extern char *xstrndup(char *p, size_t n);
extern void *xfree(void *p);

#endif /* _XMALLOC_H */
