
#include "../config.h"

#ifndef LOCALEDIR
#define LOCALEDIR "/usr/share/locale"
#endif

#ifdef HAVE_LOCALE_H
# include <locale.h>
#endif

#ifdef ENABLE_NLS
# include <libintl.h>
# define _(Text) gettext (Text)
# ifdef gettext_noop
#  define N_(String) gettext_noop (String)
# else
#  define N_(String) (String)
# endif
# define P_(singular, plural, number) ngettext(singular, plural, number)
#else
# undef bindtextdomain
# define bindtextdomain(Domain, Directory) /* empty */
# undef textdomain
# define textdomain(Domain) /* empty */
# define _(Text) (Text)
# define N_(Text) (Text)
# define P_(singular, plural, number) (number == 1 ? singular : plural)
#endif
