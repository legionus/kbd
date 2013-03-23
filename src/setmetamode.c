/*
 * setmetamode.c - aeb, 940130
 *
 * Call: setmetamode { metabit | escprefix }
 * and report the setting before and after.
 * Without arguments setmetamode will only report.
 */

#include <stdio.h>
#include <fcntl.h>
#include <linux/kd.h>
#include <sys/ioctl.h>
#include "nls.h"
#include "version.h"

static void __attribute__ ((noreturn))
usage(void)
{
    fprintf(stderr, _(
"Usage:\n"
"	setmetamode [ metabit | meta | bit | escprefix | esc | prefix ]\n"
"Each vt has his own copy of this bit. Use\n"
"	setmetamode [arg] < /dev/ttyn\n"
"to change the settings of another vt.\n"
"The setting before and after the change are reported.\n"
));
    exit(1);
}

static void
report(int meta) {
    char *s;

    switch(meta) {
      case K_METABIT:
	s = _("Meta key sets high order bit\n");
	break;
      case K_ESCPREFIX:
	s = _("Meta key gives Esc prefix\n");
	break;
      default:
	s = _("Strange mode for Meta key?\n");
    }
    printf("%s", s);
}

struct meta {
    char *name;
    int val;
} metas[] = {
    { "metabit",   K_METABIT },
    { "meta",      K_METABIT },
    { "bit",       K_METABIT },
    { "escprefix", K_ESCPREFIX },
    { "esc",       K_ESCPREFIX },
    { "prefix",    K_ESCPREFIX }
};

#define SIZE(a) (sizeof(a)/sizeof(a[0]))

int
main(int argc, char **argv) {
    char ometa, nmeta;
    struct meta *mp;

    set_progname(argv[0]);

    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE_NAME, LOCALEDIR);
    textdomain(PACKAGE_NAME);

    if (argc == 2 && !strcmp(argv[1], "-V"))
	print_version_and_exit();

    if (ioctl(0, KDGKBMETA, &ometa)) {
	perror("KDGKBMETA");
	fprintf(stderr,
		_("Error reading current setting. Maybe stdin is not a VT?\n"));
	exit(1);
    }

    if (argc <= 1) {
	report(ometa);
	exit(0);
    }

    nmeta = 0;			/* make gcc happy */
    for (mp = metas; (unsigned) (mp-metas) < SIZE(metas); mp++) {
	if(!strcmp(argv[1], mp->name)) {
	    nmeta = mp->val;
	    goto fnd;
	}
    }
    fprintf(stderr, _("unrecognized argument: _%s_\n\n"), argv[1]);
    usage();

  fnd:
    printf(_("old state:    "));
    report(ometa);
    if (ioctl(0, KDSKBMETA, nmeta)) {
	perror("KDSKBMETA");
	exit(1);
    }
    printf(_("new state:    "));
    report(nmeta);
    exit(0);
}
