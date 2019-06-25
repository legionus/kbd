#ifndef _LIBCOMMON_H_
#define _LIBCOMMON_H_

#ifndef __GNUC__
#define __attribute__(x) /*NOTHING*/
#endif

#ifndef LOCALEDIR
#define LOCALEDIR "/usr/share/locale"
#endif

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#ifdef ENABLE_NLS
  #include <libintl.h>

  #define _(Text) gettext(Text)

  #ifdef gettext_noop
    #define N_(String) gettext_noop(String)
  #else
    #define N_(String) (String)
  #endif

  #define P_(singular, plural, number) ngettext(singular, plural, number)
#else
  #undef bindtextdomain
  #define bindtextdomain(Domain, Directory) /* empty */
  #undef textdomain
  #define textdomain(Domain) /* empty */
  #define _(Text) (Text)
  #define N_(Text) (Text)
  #define P_(singular, plural, number) (number == 1 ? singular : plural)
#endif

/* setup localization for a program */
#define setuplocale() do { \
    setlocale(LC_ALL, ""); \
    bindtextdomain(PACKAGE, LOCALEDIR); \
    textdomain(PACKAGE); \
} while (0)

// getfd.c
int getfd(const char *fnam);

// version.c
const char *get_progname(void);
void set_progname(const char *name);

void
__attribute__((noreturn))
print_version_and_exit(void);

// error.c
void
__attribute__((format(printf, 2, 3)))
kbd_warning(const int errnum, const char *fmt, ...);

void
__attribute__((noreturn))
__attribute__((format(printf, 3, 4)))
kbd_error(const int exitnum, const int errnum, const char *fmt, ...);

// xmalloc.c
#include <sys/param.h>

void
__attribute__((noreturn))
nomem(void);

void *xmalloc(size_t sz);
void *xrealloc(void *p, size_t sz);
char *xstrdup(char *p);
char *xstrndup(char *p, size_t n);
void *xfree(void *p);

#endif /* _LIBCOMMON_H_ */
