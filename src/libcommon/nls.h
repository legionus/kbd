#ifndef _KBD_NLS_H_
#define _KBD_NLS_H_

#ifndef LOCALEDIR
#define LOCALEDIR "/usr/share/locale"
#endif

#ifdef HAVE_LOCALE_H
  #include <locale.h>
#else
  #undef setlocale
  #define setlocale(Category, Locale) /* empty */
#endif

#ifdef ENABLE_NLS
  #include <libintl.h>
  /*
   * For NLS support in the public shared libraries we have to specify text
   * domain name to be independent on the main program. For this purpose define
   * KBD_TEXTDOMAIN_EXPLICIT before you include nls.h to your shared library
   * code.
   */
  #ifdef KBD_TEXTDOMAIN_EXPLICIT
    #define _(Text) dgettext (KBD_TEXTDOMAIN_EXPLICIT, Text)
  #else
    #define _(Text) gettext (Text)
  #endif

  #ifdef gettext_noop
    #define N_(String) gettext_noop (String)
  #else
    #define N_(String) (String)
  #endif

  #define P_(Singular, Plural, number) ngettext (Singular, Plural, number)
#else
  #undef bindtextdomain
  #define bindtextdomain(Domain, Directory) /* empty */
  #undef textdomain
  #define textdomain(Domain) /* empty */
  #define _(Text) (Text)
  #define N_(Text) (Text)
  #define P_(Singular, Plural, number) ((number) == 1 ? (Singular) : (Plural))
#endif /* ENABLE_NLS */

/* setup localization for a program */
#define setuplocale() do { \
	setlocale(LC_ALL, ""); \
	bindtextdomain(PACKAGE, LOCALEDIR); \
	textdomain(PACKAGE); \
} while (0)

#endif /* _KBD_NLS_H_ */
