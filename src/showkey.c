#include <stdio.h>
#include <unistd.h>
#include <getopt.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <linux/keyboard.h>
#include "getfd.h"
#include "nls.h"
#include "version.h"

int tmp;	/* for debugging */

int fd;
int oldkbmode;
struct termios old;

/*
 * version 0.81 of showkey would restore kbmode unconditially to XLATE,
 * thus making the console unusable when it was called under X.
 */
static void
get_mode(void) {
        char *m;

	if (ioctl(fd, KDGKBMODE, &oldkbmode)) {
		perror("KDGKBMODE");
		exit(1);
	}
	switch(oldkbmode) {
	  case K_RAW:
	    m = "RAW"; break;
	  case K_XLATE:
	    m = "XLATE"; break;
	  case K_MEDIUMRAW:
	    m = "MEDIUMRAW"; break;
	  case K_UNICODE:
	    m = "UNICODE"; break;
	  default:
	    m = _("?UNKNOWN?"); break;
	}
	printf(_("kb mode was %s\n"), m);
	if (oldkbmode != K_XLATE) {
	    printf(_("[ if you are trying this under X, it might not work\n"
		     "since the X server is also reading /dev/console ]\n"));
	}
	printf("\n");
}

static void
clean_up(void) {
	if (ioctl(fd, KDSKBMODE, oldkbmode)) {
		perror("KDSKBMODE");
		exit(1);
	}
	if (tcsetattr(fd, 0, &old) == -1)
		perror("tcsetattr");
	close(fd);
}

static void __attribute__ ((noreturn))
die(int x) {
	printf(_("caught signal %d, cleaning up...\n"), x);
	clean_up();
	exit(1);
}

static void __attribute__ ((noreturn))
watch_dog(int x __attribute__ ((unused))) {
	clean_up();
	exit(0);
}

static void __attribute__ ((noreturn))
usage(void) {
	fprintf(stderr, _(
"showkey version %s\n\n"
"usage: showkey [options...]\n"
"\n"
"valid options are:\n"
"\n"
"	-h --help	display this help text\n"
"	-a --ascii	display the decimal/octal/hex values of the keys\n"
"	-s --scancodes	display only the raw scan-codes\n"
"	-k --keycodes	display only the interpreted keycodes (default)\n"
), PACKAGE_VERSION);
	exit(1);
}

int
main (int argc, char *argv[]) {
	const char *short_opts = "haskV";
	const struct option long_opts[] = {
		{ "help",	no_argument, NULL, 'h' },
		{ "ascii",	no_argument, NULL, 'a' },
		{ "scancodes",	no_argument, NULL, 's' },
		{ "keycodes",	no_argument, NULL, 'k' },
		{ "version",	no_argument, NULL, 'V' },
		{ NULL, 0, NULL, 0 }
	};
	int c;
	int show_keycodes = 1;
	int print_ascii = 0;

	struct termios new;
	unsigned char buf[18];	/* divisible by 3 */
	int i, n;

	set_progname(argv[0]);

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_NAME, LOCALEDIR);
	textdomain(PACKAGE_NAME);

	while ((c = getopt_long(argc, argv,
				short_opts, long_opts, NULL)) != -1) {
		switch (c) {
		case 's':
			show_keycodes = 0;
			break;
		case 'k':
			show_keycodes = 1;
			break;
		case 'a':
			print_ascii = 1;
			break;
		case 'V':
			print_version_and_exit();
		case 'h':
		case '?':
			usage();
		}
	}

	if (optind < argc)
		usage();

	if (print_ascii) {
		/* no mode and signal and timer stuff - just read stdin */
	        fd = 0;

		if (tcgetattr(fd, &old) == -1)
			perror("tcgetattr");
		if (tcgetattr(fd, &new) == -1)
			perror("tcgetattr");

		new.c_lflag &= ~ (ICANON | ISIG);
		new.c_lflag |= (ECHO | ECHOCTL);
		new.c_iflag = 0;
		new.c_cc[VMIN] = 1;
		new.c_cc[VTIME] = 0;

		if (tcsetattr(fd, TCSAFLUSH, &new) == -1)
			perror("tcgetattr");
		printf(_("\nPress any keys - "
		         "Ctrl-D will terminate this program\n\n"));

		while (1) {
			n = read(fd, buf, 1);
			if (n == 1)
				printf(" \t%3d 0%03o 0x%02x\n",
				       buf[0], buf[0], buf[0]);
			if (n != 1 || buf[0] == 04)
				break;
		}

		if (tcsetattr(fd, 0, &old) == -1)
			perror("tcsetattr");
		exit(0);
	}


	fd = getfd(NULL);

	/* the program terminates when there is no input for 10 secs */
	signal(SIGALRM, watch_dog);

	/*
	  if we receive a signal, we want to exit nicely, in
	  order not to leave the keyboard in an unusable mode
	*/
	signal(SIGHUP, die);
	signal(SIGINT, die);
	signal(SIGQUIT, die);
	signal(SIGILL, die);
	signal(SIGTRAP, die);
	signal(SIGABRT, die);
	signal(SIGIOT, die);
	signal(SIGFPE, die);
	signal(SIGKILL, die);
	signal(SIGUSR1, die);
	signal(SIGSEGV, die);
	signal(SIGUSR2, die);
	signal(SIGPIPE, die);
	signal(SIGTERM, die);
#ifdef SIGSTKFLT
	signal(SIGSTKFLT, die);
#endif
	signal(SIGCHLD, die);
	signal(SIGCONT, die);
	signal(SIGSTOP, die);
	signal(SIGTSTP, die);
	signal(SIGTTIN, die);
	signal(SIGTTOU, die);

	get_mode();
	if (tcgetattr(fd, &old) == -1)
		perror("tcgetattr");
	if (tcgetattr(fd, &new) == -1)
		perror("tcgetattr");

	new.c_lflag &= ~ (ICANON | ECHO | ISIG);
	new.c_iflag = 0;
	new.c_cc[VMIN] = sizeof(buf);
	new.c_cc[VTIME] = 1;	/* 0.1 sec intercharacter timeout */

	if (tcsetattr(fd, TCSAFLUSH, &new) == -1)
		perror("tcsetattr");
	if (ioctl(fd, KDSKBMODE, show_keycodes ? K_MEDIUMRAW : K_RAW)) {
		perror("KDSKBMODE");
		exit(1);
	}

	printf(_("press any key (program terminates 10s after last keypress)...\n"));

	/* show scancodes */
	if (!show_keycodes) {
		while (1) {
			alarm(10);
			n = read(fd, buf, sizeof(buf));
			for (i = 0; i < n; i++)
				printf("0x%02x ", buf[i]);
			printf("\n");
		}
		clean_up();
		exit(0);
	}

	/* show keycodes - 2.6 allows 3-byte reports */
	while (1) {
		alarm(10);
		n = read(fd, buf, sizeof(buf));
		i = 0;
		while (i < n) {
			int kc;
			char *s;

			s = (buf[i] & 0x80) ? _("release") : _("press");

			if (i+2 < n && (buf[i] & 0x7f) == 0
				&& (buf[i+1] & 0x80) != 0
				&& (buf[i+2] & 0x80) != 0) {
				kc = ((buf[i+1] & 0x7f) << 7) |
					(buf[i+2] & 0x7f);
				i += 3;
			} else {
				kc = (buf[i] & 0x7f);
				i++;
			}
			printf(_("keycode %3d %s\n"), kc, s);
		}
	}

	clean_up();
	exit(0);
}
