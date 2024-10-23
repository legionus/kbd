/*
 * chvt.c - aeb - 940227 - Change virtual terminal
 */
#include "config.h"

#include <sys/types.h>
#include <sys/ioctl.h>
#include <linux/vt.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>
#include <errno.h>
#include <fcntl.h>
#include <getopt.h>
#include <sysexits.h>

#include "libcommon.h"

static void KBD_ATTR_NORETURN
usage(int rc, const struct kbd_help *options)
{
	fprintf(stderr, _("Usage: %s [option...] N\n"), program_invocation_short_name);

	print_options(options);
	print_report_bugs();

	exit(rc);
}

static void
sighandler(int sig KBD_ATTR_UNUSED, siginfo_t *si KBD_ATTR_UNUSED, void *uc KBD_ATTR_UNUSED)
{
	return;
}

int main(int argc, char *argv[])
{
	int c, fd, num;
	timer_t timerid;
	struct sigaction sa;
	struct sigevent sev;
	struct itimerspec its;
	const char *console = NULL;

	const char *const short_opts = "C:hV";
	const struct option long_opts[] = {
		{ "console", required_argument, NULL, 'C' },
		{ "help",    no_argument,       NULL, 'h' },
		{ "version", no_argument,       NULL, 'V' },
		{ NULL, 0, NULL, 0 }
	};
	const struct kbd_help opthelp[] = {
		{ "-C, --console=DEV", _("the console device to be used.") },
		{ "-h, --help",        _("print this usage message.") },
		{ "-V, --version",     _("print version number.")     },
		{ NULL, NULL }
	};

	setuplocale();

	while ((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
		switch (c) {
			case 'C':
				if (optarg == NULL || optarg[0] == '\0')
					usage(EX_USAGE, opthelp);
				console = optarg;
				break;
			case 'V':
				print_version_and_exit();
				break;
			case 'h':
				usage(EXIT_SUCCESS, opthelp);
				break;
			case '?':
				usage(EX_USAGE, opthelp);
				break;
		}
	}

	if (argc == optind) {
		kbd_warning(0, _("Not enough arguments."));
		usage(EX_USAGE, opthelp);
	}

	if ((fd = getfd(console)) < 0)
		kbd_error(EXIT_FAILURE, 0, _("Couldn't get a file descriptor referring to the console."));

	num = atoi(argv[optind]);

	sa.sa_flags     = SA_SIGINFO;
	sa.sa_sigaction = sighandler;
	sigemptyset(&sa.sa_mask);

	if (sigaction(SIGALRM, &sa, NULL) < 0)
		kbd_error(EXIT_FAILURE, errno, _("Unable to set signal handler"));

	sev.sigev_notify = SIGEV_SIGNAL;
	sev.sigev_signo = SIGALRM;
	sev.sigev_value.sival_ptr = &timerid;

	if (timer_create(CLOCK_REALTIME, &sev, &timerid) < 0)
		kbd_error(EXIT_FAILURE, errno, _("Unable to create timer"));

	its.it_value.tv_sec     = 1;
	its.it_value.tv_nsec    = 0;
	its.it_interval.tv_sec  = its.it_value.tv_sec;
	its.it_interval.tv_nsec = its.it_value.tv_nsec;

	if (timer_settime(timerid, 0, &its, NULL) < 0)
		kbd_error(EXIT_FAILURE, errno, _("Unable to set timer"));

	do {
		errno = 0;

		if (ioctl(fd, VT_ACTIVATE, num) < 0 && errno != EINTR)
			kbd_error(EXIT_FAILURE, errno,  _("Couldn't activate vt %d"), num);

		if (ioctl(fd, VT_WAITACTIVE, num) < 0 && errno != EINTR)
			kbd_error(EXIT_FAILURE, errno, "ioctl(%d,VT_WAITACTIVE)", num);

	} while (errno == EINTR);

	timer_delete(timerid);
	signal(SIGALRM, SIG_DFL);

	return EXIT_SUCCESS;
}
