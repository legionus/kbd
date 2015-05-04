#ifndef _KBD_ERROR_H
#define _KBD_ERROR_H

void kbd_warning(const int errnum, const char *fmt, ...);
void kbd_error(const int exitnum, const int errnum, const char *fmt, ...);

#endif /* _KBD_ERROR_H */
