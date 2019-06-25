/*
 * setleds.c - aeb, 940130, 940909, 991008
 *
 * Call: setleds [-L] [-D] [-F] [{+|-}{num|caps|scroll}]*
 * will set or clear the indicated flags on the stdin tty,
 * and report the settings before and after.
 * In particular, setleds without arguments will only report.
 */
#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/kd.h>
#include <sys/ioctl.h>

#include "libcommon.h"

static void __attribute__((noreturn))
usage(void)
{
	fprintf(stderr, _(
	                    "Usage:\n"
	                    "	setleds [-v] [-L] [-D] [-F] [[+|-][ num | caps | scroll %s]]\n"
	                    "Thus,\n"
	                    "	setleds +caps -num\n"
	                    "will set CapsLock, clear NumLock and leave ScrollLock unchanged.\n"
	                    "The settings before and after the change (if any) are reported\n"
	                    "when the -v option is given or when no change is requested.\n"
	                    "Normally, setleds influences the vt flag settings\n"
	                    "(and these are usually reflected in the leds).\n"
	                    "With -L, setleds only sets the leds, and leaves the flags alone.\n"
	                    "With -D, setleds sets both the flags and the default flags, so\n"
	                    "that a subsequent reset will not change the flags.\n"),
#ifdef __sparc__
	        "| compose "
#else
	        ""
#endif
	        );
	exit(EXIT_FAILURE);
}

#define onoff(a) ((a) ? _("on ") : _("off"))

/* report the bits, in the order seen on the (my) keyboard */
#define LED_NLOCK 1
#define LED_CMPOSE 2
#define LED_SCRLCK 4
#define LED_CLOCK 8

static void
sunreport(int leds)
{
	printf("NumLock %s   Compose %s   ScrollLock %s   CapsLock %s\n",
	       onoff(leds & LED_NLOCK),
	       onoff(leds & LED_CMPOSE),
	       onoff(leds & LED_SCRLCK),
	       onoff(leds & LED_CLOCK));
}

static void
report(int leds)
{
	printf("NumLock %s   CapsLock %s   ScrollLock %s\n",
	       onoff(leds & LED_NUM),
	       onoff(leds & LED_CAP),
	       onoff(leds & LED_SCR));
}

struct led {
	const char *name;
	int bit;
	int sunbit;
} leds[] = {
	{ "scroll", LED_SCR, LED_SCRLCK },
	{ "num", LED_NUM, LED_NLOCK },
	{ "caps", LED_CAP, LED_CLOCK },
#ifdef __sparc__
	{ "compose", 0, LED_CMPOSE }
#endif
};

static void
getleds(char *cur_leds)
{
	if (ioctl(0, KDGETLED, cur_leds)) {
		kbd_error(EXIT_FAILURE, errno, _("Error reading current led setting. "
		                                 "Maybe stdin is not a VT?: "
		                                 "ioctl KDGETLED"));
	}
}

static int
setleds(char cur_leds)
{
	if (ioctl(0, KDSETLED, cur_leds)) {
		kbd_warning(errno, "ioctl KDSETLED");
		return -1;
	}
	return 0;
}

static void
getflags(char *flags)
{
	if (ioctl(0, KDGKBLED, flags)) {
		kbd_error(EXIT_FAILURE, errno, _("Error reading current flags setting. "
		                                 "Maybe you are not on the console?: "
		                                 "ioctl KDGKBLED"));
	}
}

static int sunkbdfd = -1;

#ifndef KIOCGLED
#define arg_state __attribute__((unused))
#else
#define arg_state
#endif

static void
sungetleds(arg_state char *cur_leds)
{
#ifdef KIOCGLED
	if (ioctl(sunkbdfd, KIOCGLED, cur_leds)) {
		kbd_error(EXIT_FAILURE, errno, _("Error reading current led setting from /dev/kbd: "
		                                 "ioctl KIOCGLED"));
	}
#else
	kbd_error(EXIT_FAILURE, 0, _("KIOCGLED unavailable?\n"));
#endif
}

#ifndef KIOCSLED
#define arg_state __attribute__((unused))
#else
#define arg_state
#endif

static void
sunsetleds(arg_state char *cur_leds)
{
#ifdef KIOCSLED
	if (ioctl(sunkbdfd, KIOCSLED, cur_leds)) {
		kbd_error(EXIT_FAILURE, errno, _("Error reading current led setting from /dev/kbd: "
		                                 "ioctl KIOCSLED"));
	}
#else
	kbd_error(EXIT_FAILURE, 0, _("KIOCSLED unavailable?\n"));
#endif
}

int main(int argc, char **argv)
{
	int optL = 0, optD = 0, optF = 0, verbose = 0;
	char oleds, nleds, oflags, nflags, odefflags, ndefflags;
	char nval, ndef, sign;
	char osunleds = 0, nsunleds, nsunval, nsundef;
	char *ap;
	struct led *lp;

	set_progname(argv[0]);
	setuplocale();

	if (argc == 2 && (!strcmp("-V", argv[1]) || !strcmp("--version", argv[1])))
		print_version_and_exit();

#ifdef __sparc__
	if ((sunkbdfd = open("/dev/kbd", O_RDONLY)) < 0) {
		kbd_error(EXIT_FAILURE, errno, "open /dev/kbd");
		/* exit(1); */
	}
#endif

	getflags(&oflags);
	getleds(&oleds);
	if (sunkbdfd >= 0)
		sungetleds(&osunleds);

	while (argc > 1) {
		if (!strcmp("-L", argv[1]))
			optL = 1;
		else if (!strcmp("-D", argv[1]))
			optD = 1;
		else if (!strcmp("-F", argv[1]))
			optF = 1;
		else if (!strcmp("-v", argv[1]))
			verbose = 1;
		else
			break;
		argc--;
		argv++;
	}

	odefflags = ndefflags = ((oflags >> 4) & 7);
	oflags = nflags = (oflags & 7);

	if (argc <= 1) {
		if (optL) {
			nleds = (char) 0xff;
			if (setleds(nleds)) {
				kbd_error(EXIT_FAILURE, 0, _("Error resetting ledmode\n"));
			}
		}

		/* If nothing to do, report, even if not verbose */
		if (!optD && !optL && !optF)
			optD = optL = optF = 1;
		if (optD) {
			printf(_("Current default flags:  "));
			report(odefflags);
		}
		if (optF) {
			printf(_("Current flags:          "));
			report(oflags & 07);
		}
		if (optL) {
			printf(_("Current leds:           "));
			if (sunkbdfd >= 0)
				sunreport(osunleds);
			else
				report(oleds);
		}
		exit(EXIT_SUCCESS);
	}

	if (!optL)
		optF = 1;
	nval         = 0;
	ndef         = 0;
	nsunval      = 0;
	nsundef      = 0;

	while (--argc) {
		ap   = *++argv;
		sign = 1; /* by default: set */
		if (*ap == '+')
			ap++;
		else if (*ap == '-') {
			sign = 0;
			ap++;
		}
		for (lp = leds; (unsigned)(lp - leds) < sizeof(leds) / sizeof(leds[0]); lp++) {
			if (!strcmp(ap, lp->name)) {
				if (sign) {
					nval |= lp->bit;
					nsunval |= lp->sunbit;
				}
				ndef |= lp->bit;
				nsundef |= lp->sunbit;
				goto nxtarg;
			}
		}
		fprintf(stderr, _("unrecognized argument: _%s_\n\n"), ap);
		usage();

	nxtarg:;
	}

	if (optD) {
		ndefflags = (odefflags & ~ndef) | nval;
		if (verbose) {
			printf(_("Old default flags:    "));
			report(odefflags);
			printf(_("New default flags:    "));
			report(ndefflags);
		}
	}
	if (optF) {
		nflags = ((oflags & ~ndef) | nval);
		if (verbose) {
			printf(_("Old flags:            "));
			report(oflags & 07);
			printf(_("New flags:            "));
			report(nflags & 07);
		}
	}
	if (optD || optF) {
		if (ioctl(0, KDSKBLED, (ndefflags << 4) | nflags)) {
			kbd_error(EXIT_FAILURE, errno, "ioctl KDSKBLED");
		}
	}
	if (optL) {
		if (sunkbdfd >= 0) {
			nsunleds = (osunleds & ~nsundef) | nsunval;
			if (verbose) {
				printf(_("Old leds:             "));
				sunreport(osunleds);
				printf(_("New leds:             "));
				sunreport(nsunleds);
			}
			sunsetleds(&nsunleds);
		} else {
			nleds = (oleds & ~ndef) | nval;
			if (verbose) {
				printf(_("Old leds:             "));
				report(oleds);
				printf(_("New leds:             "));
				report(nleds);
			}
			if (setleds(nleds))
				exit(EXIT_FAILURE);
		}
	}
	exit(EXIT_SUCCESS);
}
