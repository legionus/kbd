#include "config.h"

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <errno.h>
#include <unistd.h>
#include <limits.h>
#include <dirent.h>
#include <poll.h>
#include <time.h>
#include <getopt.h>
#include <fcntl.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <sysexits.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include <linux/keyboard.h>
#include <linux/input.h>

#include "libcommon.h"

#define EVDEV_DIR		"/dev/input"
#define MAX_EVDEV_DEVICES	256
#define EVDEV_BITS_PER_LONG	(sizeof(unsigned long) * CHAR_BIT)
#define NBITS(x)		((((x) - 1) / EVDEV_BITS_PER_LONG) + 1)
#define TEST_BIT(x, a)		((a)[(x) / EVDEV_BITS_PER_LONG] & (1UL << ((x) % EVDEV_BITS_PER_LONG)))

struct evdev_device {
	int fd;
	char *path;
	int dropped;
};

struct input_terminal {
	int fd;
	int active;
	struct termios old;
};

static int fd = -1;
static int oldkbmode;
static struct termios old;
static volatile sig_atomic_t console_termios_active;
static volatile sig_atomic_t console_kb_active;
static struct input_terminal input_terminal = { .fd = -1 };

/*
 * version 0.81 of showkey would restore kbmode unconditionally to XLATE,
 * thus making the console unusable when it was called under X.
 */
static void
get_mode(void)
{
	const char *m;

	if (ioctl(fd, KDGKBMODE, &oldkbmode))
		kbd_error(EXIT_FAILURE, errno, _("Unable to read keyboard mode"));

	switch (oldkbmode) {
		case K_RAW:
			m = "RAW";
			break;
		case K_XLATE:
			m = "XLATE";
			break;
		case K_MEDIUMRAW:
			m = "MEDIUMRAW";
			break;
		case K_UNICODE:
			m = "UNICODE";
			break;
		default:
			m = _("?UNKNOWN?");
			break;
	}
	printf(_("kb mode was %s\n"), m);
	if (oldkbmode != K_XLATE) {
		printf(_("[ if you are trying this under X, it might not work\n"
			 "since the X server is also reading /dev/console ]\n"));
	}
	printf("\n");
}

static int
restore_terminal(struct input_terminal *terminal)
{
	int error = 0;

	if (terminal->active && tcsetattr(terminal->fd, TCSAFLUSH, &terminal->old) < 0)
		error = errno;

	if (terminal->fd >= 0)
		close(terminal->fd);

	terminal->fd = -1;
	terminal->active = 0;

	return error;
}

static void
restore_input_terminal(void)
{
	int error;
	sigset_t blocked, previous;

	sigfillset(&blocked);
	sigprocmask(SIG_BLOCK, &blocked, &previous);

	error = restore_terminal(&input_terminal);
	if (error)
		kbd_warning(error, "tcsetattr controlling terminal");

	sigprocmask(SIG_SETMASK, &previous, NULL);
}

static void
setup_input_terminal(int input_fd)
{
	struct termios new;
	unsigned int input_device;
	unsigned int terminal_device;

	input_terminal.fd = open("/dev/tty", O_RDWR | O_NONBLOCK | O_CLOEXEC);

	if (input_terminal.fd < 0)
		return;

	if (input_fd >= 0) {
		/* Never drain a console unless it is known to be a different tty. */
		if (ioctl(input_fd, TIOCGDEV, &input_device) < 0 ||
		    ioctl(input_terminal.fd, TIOCGDEV, &terminal_device) < 0 ||
		    input_device == terminal_device) {
			close(input_terminal.fd);
			input_terminal.fd = -1;
			return;
		}
	}

	if (tcgetattr(input_terminal.fd, &input_terminal.old) < 0) {
		close(input_terminal.fd);
		input_terminal.fd = -1;
		return;
	}

	new = input_terminal.old;
	new.c_lflag &= ~((tcflag_t) (ICANON | ECHO | ECHONL | ISIG | IEXTEN));
	new.c_iflag &= ~((tcflag_t) (IXON | IXOFF | IXANY));
	new.c_cc[VMIN] = 0;
	new.c_cc[VTIME] = 0;
	input_terminal.active = 1;

	if (tcsetattr(input_terminal.fd, TCSAFLUSH, &new) < 0) {
		kbd_warning(errno, "tcsetattr");

		close(input_terminal.fd);

		input_terminal.fd = -1;
		input_terminal.active = 0;
	}
}

static void
drain_input_terminal(short revents)
{
	unsigned char buf[256];

	/* Bound the work so continuous tty input cannot starve device input. */
	if (revents & POLLIN) {
		if (read(input_terminal.fd, buf, sizeof(buf)) < 0 && errno != EAGAIN && errno != EINTR) {
			restore_input_terminal();
			return;
		}
	}
	if (revents & (POLLERR | POLLHUP | POLLNVAL))
		restore_input_terminal();
}

static void
clean_up_state(int quiet)
{
	sigset_t blocked, previous;
	int kb_error, tty_error, input_error;

	kb_error = tty_error = input_error = 0;

	/* Do not let a signal interrupt restoration or close an fd twice. */
	sigfillset(&blocked);
	sigprocmask(SIG_BLOCK, &blocked, &previous);

	if (console_kb_active && ioctl(fd, KDSKBMODE, oldkbmode) < 0)
		kb_error = errno;

	console_kb_active = 0;

	if (console_termios_active && tcsetattr(fd, TCSAFLUSH, &old) < 0)
		tty_error = errno;

	console_termios_active = 0;

	if (fd >= 0) {
		close(fd);
		fd = -1;
	}

	input_error = restore_terminal(&input_terminal);

	if (!quiet) {
		if (kb_error)
			kbd_warning(kb_error, "ioctl KDSKBMODE");
		if (tty_error)
			kbd_warning(tty_error, "tcsetattr console");
		if (input_error)
			kbd_warning(input_error, "tcsetattr controlling terminal");
	}

	sigprocmask(SIG_SETMASK, &previous, NULL);
}

static void
clean_up(void)
{
	clean_up_state(0);
}

static void KBD_ATTR_NORETURN
die(int sig)
{
	clean_up_state(1);
	_exit(sig == SIGALRM ? EXIT_SUCCESS : 128 + sig);
}

static void
setup_cleanup(void)
{
	const int signals[] = {
		SIGHUP, SIGINT, SIGQUIT, SIGILL, SIGTRAP, SIGABRT, SIGFPE,
		SIGUSR1, SIGSEGV, SIGUSR2, SIGPIPE, SIGTERM,
#ifdef SIGSTKFLT
		SIGSTKFLT,
#endif
		SIGCHLD, SIGCONT, SIGTSTP, SIGTTIN, SIGTTOU, SIGALRM
	};
	struct sigaction action = { .sa_handler = die };

	/* Signal exits use _exit(), so event output must not remain buffered. */
	setvbuf(stdout, NULL, _IONBF, 0);

	atexit(clean_up);
	sigfillset(&action.sa_mask);

	for (size_t i = 0; i < sizeof(signals) / sizeof(signals[0]); i++) {
		if (sigaction(signals[i], &action, NULL) < 0)
			kbd_error(EXIT_FAILURE, errno, "sigaction");
	}
}

static int
is_event_name(const char *name)
{
	const char *p;

	if (strncmp(name, "event", 5) != 0 || !name[5])
		return 0;

	for (p = name + 5; *p; p++) {
		if (*p < '0' || *p > '9')
			return 0;
	}
	return 1;
}

static int
is_evdev(int evfd)
{
	int version;
	return ioctl(evfd, EVIOCGVERSION, &version) == 0;
}

static int
evdev_supports_mode(int evfd, int show_keycodes)
{
	unsigned long evbits[NBITS(EV_MAX + 1)] = { 0 };
	unsigned long mscbits[NBITS(MSC_MAX + 1)] = { 0 };

	if (ioctl(evfd, EVIOCGBIT(0, sizeof(evbits)), evbits) < 0)
		return 0;

	if (show_keycodes)
		return !!TEST_BIT(EV_KEY, evbits);

	if (!TEST_BIT(EV_MSC, evbits) || ioctl(evfd, EVIOCGBIT(EV_MSC, sizeof(mscbits)), mscbits) < 0)
		return 0;

	return !!TEST_BIT(MSC_SCAN, mscbits);
}

static int
evdev_is_keyboard(int evfd)
{
	unsigned long keys[NBITS(KEY_MAX + 1)] = { 0 };

	if (ioctl(evfd, EVIOCGBIT(EV_KEY, sizeof(keys)), keys) < 0)
		return 0;

	/* Include separate media-key interfaces, but not button-only devices. */
	for (unsigned int key = KEY_ESC; key <= KEY_MAX; key++) {
		if ((key >= BTN_MISC && key < KEY_OK) ||
		    (key >= BTN_DPAD_UP && key <= BTN_DPAD_RIGHT) ||
		    key >= BTN_TRIGGER_HAPPY)
			continue;
		if (TEST_BIT(key, keys))
			return 1;
	}
	return 0;
}

static void
init_evdev_device(struct evdev_device *device, int evfd, const char *path)
{
	device->dropped = 0;
	device->fd = evfd;
	device->path = strdup(path);

	if (!device->path) {
		close(evfd);
		kbd_error(EXIT_FAILURE, errno, _("unable to allocate memory"));
	}
}

static void
add_evdev_device(struct evdev_device *devices, size_t *ndevices,
		 const char *path, int show_keycodes)
{
	int evfd;

	if (*ndevices >= MAX_EVDEV_DEVICES) {
		kbd_warning(0, _("too many evdev devices; ignoring %s"), path);
		return;
	}

	evfd = open(path, O_RDONLY | O_NONBLOCK | O_CLOEXEC);
	if (evfd < 0) {
		kbd_warning(errno, _("unable to open %s"), path);
		return;
	}

	if (!is_evdev(evfd) || !evdev_is_keyboard(evfd) || !evdev_supports_mode(evfd, show_keycodes)) {
		close(evfd);
		return;
	}

	init_evdev_device(&devices[*ndevices], evfd, path);
	(*ndevices)++;
}

static size_t
find_evdev_devices(struct evdev_device *devices, int show_keycodes)
{
	struct dirent *entry;
	DIR *dir;
	size_t ndevices = 0;
	char path[PATH_MAX];
	int len;

	dir = opendir(EVDEV_DIR);
	if (!dir)
		kbd_error(EXIT_FAILURE, errno, _("unable to open %s"), EVDEV_DIR);

	while (1) {
		errno = 0;
		entry = readdir(dir);
		if (!entry) {
			if (errno)
				kbd_warning(errno, _("unable to read %s"), EVDEV_DIR);
			break;
		}

		if (!is_event_name(entry->d_name))
			continue;

		len = snprintf(path, sizeof(path), "%s/%s", EVDEV_DIR, entry->d_name);

		if (len >= 0 && (size_t) len < sizeof(path))
			add_evdev_device(devices, &ndevices, path, show_keycodes);
	}
	closedir(dir);

	return ndevices;
}

static int64_t
monotonic_msec(void)
{
	struct timespec ts;

	if (clock_gettime(CLOCK_MONOTONIC, &ts) < 0)
		kbd_error(EXIT_FAILURE, errno, "clock_gettime");

	return (int64_t) ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

static int
print_evdev_event(struct evdev_device *device,
		  const struct input_event *event, int show_keycodes,
		  int show_source)
{
	const char *state;

	if (event->type == EV_SYN && event->code == SYN_DROPPED) {
		kbd_warning(0, _("%s: events lost; ignoring input until the next SYN_REPORT"), device->path);
		device->dropped = 1;
		return 0;
	}
	if (device->dropped) {
		if (event->type == EV_SYN && event->code == SYN_REPORT)
			device->dropped = 0;
		return 0;
	}

	if (!show_keycodes) {
		if (event->type != EV_MSC || event->code != MSC_SCAN)
			return 0;

		if (show_source)
			printf("%s: ", device->path);

		printf(_("scan code 0x%08x\n"), (uint32_t) event->value);

		return 1;
	}

	if (event->type != EV_KEY)
		return 0;

	switch (event->value) {
		case 0:
			state = _("release");
			break;
		case 1:
			state = _("press");
			break;
		case 2:
			state = _("repeat");
			break;
		default:
			return 0;
	}

	if (show_source)
		printf("%s: ", device->path);

	printf(_("keycode %3u %s\n"), event->code, state);

	return 1;
}

static void
close_evdev_device(struct evdev_device *device)
{
	if (device->fd >= 0) {
		close(device->fd);
		device->fd = -1;
	}
	free(device->path);
	device->path = NULL;
}

static int
read_evdev_devices(struct evdev_device *devices, size_t ndevices,
		   int show_keycodes, int timeout, int show_source)
{
	struct pollfd pollfds[MAX_EVDEV_DEVICES + 1];
	int64_t deadline;
	nfds_t npollfds = ndevices;
	size_t active = ndevices;

	for (size_t i = 0; i < ndevices; i++) {
		pollfds[i].fd = devices[i].fd;
		pollfds[i].events = POLLIN;
	}

	setup_cleanup();

	setup_input_terminal(-1);

	if (input_terminal.active) {
		pollfds[npollfds].fd = input_terminal.fd;
		pollfds[npollfds].events = POLLIN;
		npollfds++;
	}

	printf(_("press any key (program terminates %ds after last keypress)...\n"), timeout);

	deadline = monotonic_msec() + (int64_t) timeout * 1000;

	while (active) {
		int wait, rc;
		int64_t remaining = deadline - monotonic_msec();

		if (remaining <= 0)
			break;

		wait = remaining > INT_MAX ? INT_MAX : (int) remaining;

		rc = poll(pollfds, npollfds, wait);

		if (rc < 0) {
			if (errno == EINTR)
				continue;
			kbd_error(EXIT_FAILURE, errno, "poll");
		}
		if (!rc)
			break;

		for (size_t i = 0; i < ndevices; i++) {
			if (pollfds[i].fd < 0 || !pollfds[i].revents)
				continue;

			if (pollfds[i].revents & POLLIN) {
				struct input_event events[32];
				ssize_t nread = read(pollfds[i].fd, events, sizeof(events));

				if (nread > 0) {
					size_t nevents = (size_t) nread / sizeof(events[0]);

					for (size_t j = 0; j < nevents; j++) {
						if (print_evdev_event(&devices[i], &events[j], show_keycodes, show_source))
							deadline = monotonic_msec() + (int64_t) timeout * 1000;
					}
				} else if (nread == 0 || (nread < 0 && errno != EAGAIN && errno != EINTR)) {
					if (nread < 0)
						kbd_warning(errno, _("unable to read %s"), devices[i].path);

					close_evdev_device(&devices[i]);

					pollfds[i].fd = -1;
					active--;
				}
			}

			if (pollfds[i].fd >= 0 && (pollfds[i].revents & (POLLERR | POLLHUP | POLLNVAL))) {
				close_evdev_device(&devices[i]);
				pollfds[i].fd = -1;
				active--;
			}
		}

		if (input_terminal.active) {
			drain_input_terminal(pollfds[ndevices].revents);
			pollfds[ndevices].fd = input_terminal.fd;
		}
	}

	for (size_t i = 0; i < ndevices; i++)
		close_evdev_device(&devices[i]);

	restore_input_terminal();

	return EXIT_SUCCESS;
}

static void KBD_ATTR_NORETURN
usage(int rc, const struct kbd_help *options)
{
	fprintf(stderr, _("Usage: %s [option...]\n"), program_invocation_short_name);

	print_options(options);
	print_report_bugs();

	exit(rc);
}

static int
run_evdev(const char *device, int show_keycodes, int timeout)
{
	struct evdev_device devices[MAX_EVDEV_DEVICES] = { 0 };
	int evfd;

	if (!strcmp(device, "all")) {
		size_t ndevices = find_evdev_devices(devices, show_keycodes);
		if (!ndevices)
			kbd_error(EXIT_FAILURE, 0, _("no suitable evdev devices found"));

		return read_evdev_devices(devices, ndevices, show_keycodes, timeout, 1);
	}

	evfd = open(device, O_RDONLY | O_NONBLOCK | O_CLOEXEC);

	if (evfd < 0)
		kbd_error(EXIT_FAILURE, errno, _("unable to open %s"), device);

	if (!is_evdev(evfd)) {
		close(evfd);
		return -1;
	}

	if (!evdev_supports_mode(evfd, show_keycodes)) {
		close(evfd);

		if (show_keycodes)
			kbd_error(EXIT_FAILURE, 0, _("%s does not report key events"), device);
		else
			kbd_error(EXIT_FAILURE, 0, _("%s does not report MSC_SCAN events"), device);
	}

	init_evdev_device(&devices[0], evfd, device);

	return read_evdev_devices(devices, 1, show_keycodes, timeout, 0);
}

static void
run_ascii(void)
{
	struct termios new = { 0 };
	unsigned char ch;

	fd = STDIN_FILENO;

	if (tcgetattr(fd, &old) == -1)
		kbd_warning(errno, "tcgetattr");
	if (tcgetattr(fd, &new) == -1)
		kbd_warning(errno, "tcgetattr");

	new.c_lflag &= ~((tcflag_t) (ICANON | ISIG));
	new.c_lflag |= (ECHO | ECHOCTL);
	new.c_iflag = 0;
	new.c_cc[VMIN] = 1;
	new.c_cc[VTIME] = 0;

	if (tcsetattr(fd, TCSAFLUSH, &new) == -1)
		kbd_warning(errno, "tcgetattr");
	printf(_("\nPress any keys - "
		 "Ctrl-D will terminate this program\n\n"));

	while (read(fd, &ch, 1) == 1) {
		printf(" \t%3d 0%03o 0x%02x\n", ch, ch, ch);
		if (ch == 04)
			break;
	}

	if (tcsetattr(fd, 0, &old) == -1)
		kbd_warning(errno, "tcsetattr");
}

static ssize_t
read_console(unsigned char *buf, size_t size)
{
	while (input_terminal.active) {
		struct pollfd pollfds[] = {
			{ .fd = fd,                .events = POLLIN },
			{ .fd = input_terminal.fd, .events = POLLIN },
		};

		if (poll(pollfds, 2, -1) < 0) {
			if (errno == EINTR)
				continue;

			kbd_warning(errno, "poll");

			clean_up();
			exit(EXIT_FAILURE);
		}

		drain_input_terminal(pollfds[1].revents);

		if (pollfds[0].revents)
			break;
	}
	return read(fd, buf, size);
}

static void
run_console(const char *device, int show_keycodes, int timeout)
{
	struct termios new = { 0 };
	unsigned char buf[18];
	ssize_t n;
	int i;

	setup_cleanup();
	if ((fd = getfd(device)) < 0)
		kbd_error(EXIT_FAILURE, 0, _("Couldn't get a file descriptor referring to the console."));

	get_mode();
	if (tcgetattr(fd, &old) == -1)
		kbd_error(EXIT_FAILURE, errno, "tcgetattr");
	new = old;

	new.c_lflag &= ~((tcflag_t) (ICANON | ECHO | ISIG));
	new.c_iflag = 0;
	new.c_cc[VMIN] = sizeof(buf);
	new.c_cc[VTIME] = 1; /* 0.1 sec intercharacter timeout */

	console_termios_active = 1;
	if (tcsetattr(fd, TCSAFLUSH, &new) == -1)
		kbd_error(EXIT_FAILURE, errno, "tcsetattr");
	console_kb_active = 1;
	if (ioctl(fd, KDSKBMODE, show_keycodes ? K_MEDIUMRAW : K_RAW)) {
		kbd_error(EXIT_FAILURE, errno, "ioctl KDSKBMODE");
	}

	setup_input_terminal(fd);

	printf(_("press any key (program terminates %ds after last keypress)...\n"), timeout);

	/* show scancodes */
	if (!show_keycodes) {
		while (1) {
			alarm((unsigned int) timeout);
			n = read_console(buf, sizeof(buf));
			for (i = 0; i < n; i++)
				printf("0x%02x ", buf[i]);
			printf("\n");
		}
		return;
	}

	/* show keycodes - 2.6 allows 3-byte reports */
	while (1) {
		alarm((unsigned int) timeout);
		n = read_console(buf, sizeof(buf));
		i = 0;
		while (i < n) {
			int kc;
			const char *s;

			s = (buf[i] & 0x80) ? _("release") : _("press");

			if (i + 2 < n && (buf[i] & 0x7f) == 0 && (buf[i + 1] & 0x80) != 0 && (buf[i + 2] & 0x80) != 0) {
				kc = ((buf[i + 1] & 0x7f) << 7) |
				     (buf[i + 2] & 0x7f);
				i += 3;
			} else {
				kc = (buf[i] & 0x7f);
				i++;
			}
			printf(_("keycode %3d %s\n"), kc, s);
		}
	}
}

int main(int argc, char *argv[])
{
	const char *short_opts = "haskVt:d:";
	const struct option long_opts[] = {
		{ "help",      no_argument,       NULL, 'h' },
		{ "ascii",     no_argument,       NULL, 'a' },
		{ "scancodes", no_argument,       NULL, 's' },
		{ "keycodes",  no_argument,       NULL, 'k' },
		{ "timeout",   required_argument, NULL, 't' },
		{ "device",    required_argument, NULL, 'd' },
		{ "version",   no_argument,       NULL, 'V' },
		{ NULL,        0,                 NULL, 0   }
	};
	int c;
	const char *device = NULL;
	int show_keycodes = 1;
	int print_ascii = 0;
	int timeout = 10;

	setuplocale();

	const struct kbd_help opthelp[] = {
		{ "-a, --ascii",         _("display the decimal/octal/hex values of the keys.")    },
		{ "-s, --scancodes",     _("display only the raw scan-codes.")                     },
		{ "-k, --keycodes",      _("display only the interpreted keycodes (default).")     },
		{ "-t, --timeout",       _("set timeout, default 10")                              },
		{ "-d, --device=DEVICE", _("read from DEVICE; use 'all' for all evdev keyboards.") },
		{ "-h, --help",          _("print this usage message.")                            },
		{ "-V, --version",       _("print version number.")                                },
		{ NULL,                  NULL						      }
	};

	while ((c = getopt_long(argc, argv, short_opts, long_opts, NULL)) != -1) {
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
				break;
			case 'h':
				usage(EXIT_SUCCESS, opthelp);
				break;
			case 't':
				timeout = atoi(optarg);
				/*  in anycase if conversion wrong */
				if (timeout < 1)
					timeout = 10;
				break;
			case 'd':
				device = optarg;
				break;
			case '?':
				usage(EX_USAGE, opthelp);
				break;
		}
	}

	if (optind < argc)
		usage(EX_USAGE, opthelp);

	if (print_ascii) {
		if (device)
			kbd_error(EX_USAGE, 0, _("--device cannot be used with --ascii"));

		/* no mode and signal and timer stuff - just read stdin */
		run_ascii();
		return EXIT_SUCCESS;
	}

	if (device) {
		int rc = run_evdev(device, show_keycodes, timeout);

		if (rc >= 0)
			return rc;
	}

	run_console(device, show_keycodes, timeout);
	clean_up();

	return EXIT_SUCCESS;
}
