#ifdef __klibc__
#ifndef _KLIBC_COMPAT_H
#define _KLIBC_COMPAT_H

#include <malloc.h>

#define signal sysv_signal

extern void clearerr(FILE *stream);
extern void rewind(FILE *stream);
extern int feof(FILE *stream);
extern int ferror(FILE *stream);
extern int ungetc(int c, FILE *stream);

#endif
#endif /* __klibc__ */
