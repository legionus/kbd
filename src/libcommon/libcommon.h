#ifndef _LIBCOMMON_H_
#define _LIBCOMMON_H_

#include <kbd/compiler_attributes.h>

#ifndef LOCALEDIR
#define LOCALEDIR "/usr/share/locale"
#endif

#ifdef HAVE_LOCALE_H
#include <locale.h>
#endif

#ifdef ENABLE_NLS
  #include <libintl.h>

  #define _(Text) gettext(Text)
  #define P_(singular, plural, number) ngettext(singular, plural, number)
#else
  #undef bindtextdomain
  #define bindtextdomain(Domain, Directory) /* empty */
  #undef textdomain
  #define textdomain(Domain) /* empty */
  #define _(Text) (Text)
  #define P_(singular, plural, number) (number == 1 ? singular : plural)
#endif

/* setup localization for a program */
#define setuplocale() do { \
    setlocale(LC_ALL, ""); \
    bindtextdomain(PACKAGE, LOCALEDIR); \
    textdomain(PACKAGE); \
} while (0)

struct kbd_help {
	const char *opts;
	const char *desc;
};

// getfd.c
int getfd(const char *fnam);

// version.c
extern const char *progname;

const char *get_progname(void);
void set_progname(const char *name);

void print_version_and_exit(void)
	KBD_ATTR_NORETURN;

void print_options(const struct kbd_help *options);
void print_report_bugs(void);

// error.c
void kbd_warning(const int errnum, const char *fmt, ...)
	KBD_ATTR_PRINTF(2, 3);

void
kbd_error(const int exitnum, const int errnum, const char *fmt, ...)
	KBD_ATTR_PRINTF(3, 4)
	KBD_ATTR_NORETURN;

#endif /* _LIBCOMMON_H_ */
