#include <stdio.h>
#include <errno.h>
#include "kbd.h"
#include "klibc_compat.h"

void clearerr(attr_unused FILE *stream) {
    errno = 0;
}

void rewind(FILE *stream) {
    (void)fseek(stream, 0L, SEEK_SET);
    clearerr(stream);
}

int feof(attr_unused FILE *stream) {
    return (errno == EOF);
}

int ferror(attr_unused FILE *stream) {
    return (errno != 0 && errno != EOF);
}

int ungetc(int c, FILE *stream) {
    if (fseek(stream, -1, SEEK_CUR) < 0)
	return EOF;
    else
	return c;
}
