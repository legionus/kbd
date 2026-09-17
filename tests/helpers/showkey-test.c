/* Exercise showkey without requiring an input device or a virtual console. */
#include <stdarg.h>
#include <sys/ioctl.h>
#include <linux/input.h>
#include <linux/kd.h>

static int test_ioctl(int fd, unsigned long request, ...);
int showkey_program_main(int argc, char **argv);

#define ioctl test_ioctl
#define main showkey_program_main
#include "../../src/showkey.c"
#undef main
#define fd console_fd
#include "../../src/libcommon/getfd.c"
#undef fd
#undef ioctl

static unsigned long test_keys[NBITS(KEY_MAX + 1)];
static int fail_device_id;
static int fail_kb_mode;
static int kb_mode_calls;

static int test_ioctl(int devfd, unsigned long request, ...)
{
	va_list ap;
	void *arg;

	/* PTYs stand in for VTs; all termios operations remain real. */
	if (request == KDSKBMODE) {
		kb_mode_calls++;
		if (fail_kb_mode) {
			errno = EIO;
			return -1;
		}
		return 0;
	}

	va_start(ap, request);
	arg = va_arg(ap, void *);
	va_end(ap);
	if (request == KDGKBMODE) {
		*(int *) arg = K_XLATE;
		return 0;
	}
	if (request == KDGKBTYPE) {
		*(char *) arg = KB_101;
		return 0;
	}
	if (request == TIOCGDEV && fail_device_id) {
		errno = ENOTTY;
		return -1;
	}
	if (devfd == -1 && request == EVIOCGBIT(EV_KEY, sizeof(test_keys))) {
		memcpy(arg, test_keys, sizeof(test_keys));
		return sizeof(test_keys);
	}
	return ioctl(devfd, request, arg);
}

#define CHECK(condition)                                                  \
	do {                                                              \
		if (!(condition)) {                                       \
			fprintf(stderr, "failed at line %d\n", __LINE__); \
			return EXIT_FAILURE;                              \
		}                                                         \
	} while (0)

static int test_events(void)
{
	char path_a[] = "keyboard-a";
	char path_b[] = "keyboard-b";

	struct evdev_device a = { .path = path_a };
	struct evdev_device b = { .path = path_b };
	struct input_event event = { .type = EV_SYN, .code = SYN_DROPPED };
	const unsigned int keys[] = { KEY_A, KEY_VOLUMEUP, KEY_OK, BTN_LEFT, BTN_SOUTH,
				      BTN_DPAD_UP, BTN_TRIGGER_HAPPY1 };

	CHECK(!evdev_is_keyboard(-1));

	for (size_t i = 0; i < sizeof(keys) / sizeof(keys[0]); i++) {
		memset(test_keys, 0, sizeof(test_keys));
		test_keys[keys[i] / EVDEV_BITS_PER_LONG] |= 1UL << (keys[i] % EVDEV_BITS_PER_LONG);

		CHECK(evdev_is_keyboard(-1) == (i < 3));
	}

	CHECK(!print_evdev_event(&a, &event, 1, 1));

	event.type = EV_KEY;
	event.code = KEY_A;
	event.value = 1;

	CHECK(!print_evdev_event(&a, &event, 1, 1));
	CHECK(print_evdev_event(&b, &event, 1, 1));

	event.type = EV_MSC;
	event.code = MSC_SCAN;
	event.value = 0x70004;

	CHECK(!print_evdev_event(&a, &event, 0, 1));
	CHECK(print_evdev_event(&b, &event, 0, 1));

	event.type = EV_SYN;
	event.code = SYN_REPORT;

	CHECK(!print_evdev_event(&a, &event, 0, 1));

	event.type = EV_MSC;
	event.code = MSC_SCAN;

	CHECK(print_evdev_event(&a, &event, 0, 1));

	event.type = EV_KEY;
	event.code = KEY_A;

	for (int value = 0; value <= 3; value++) {
		event.value = value;
		CHECK(print_evdev_event(&a, &event, 1, 1) == (value < 3));
	}

	return EXIT_SUCCESS;
}

int main(int argc, char **argv)
{
	struct evdev_device device;

	if (argc == 1)
		return test_events();

	if (!strcmp(argv[1], "cleanup-failure")) {
		struct termios changed;
		int reopened;

		CHECK(argc == 3);

		setup_cleanup();

		fd = open(argv[2], O_RDWR | O_NOCTTY);

		CHECK(fd >= 0);
		CHECK(tcgetattr(fd, &old) == 0);

		changed = old;
		changed.c_lflag &= ~((tcflag_t) (ICANON | ECHO));
		console_termios_active = 1;

		CHECK(tcsetattr(fd, TCSANOW, &changed) == 0);

		console_kb_active = 1;

		setup_input_terminal(fd);

		CHECK(input_terminal.active);

		fail_kb_mode = 1;

		clean_up();

		CHECK(kb_mode_calls == 1);
		CHECK(fd == -1 && input_terminal.fd == -1);

		reopened = open(argv[2], O_RDWR | O_NOCTTY);

		CHECK(reopened >= 0);

		clean_up();

		CHECK(kb_mode_calls == 1);
		CHECK(fcntl(reopened, F_GETFD) >= 0);

		close(reopened);
		return EXIT_SUCCESS;
	}

	if (!strcmp(argv[1], "evdev")) {
		CHECK(argc == 3);
		init_evdev_device(&device, atoi(argv[2]), "test-input");
		return read_evdev_devices(&device, 1, 1, 1, 0);
	}

	if (!strcmp(argv[1], "identity-failure")) {
		fail_device_id = 1;
		setup_input_terminal(STDIN_FILENO);
		CHECK(!input_terminal.active);
		return EXIT_SUCCESS;
	}

	CHECK(argc == 3);

	if (!strcmp(argv[1], "setup-failure"))
		fail_kb_mode = 1;

	run_console(argv[2], strcmp(argv[1], "scancodes") != 0, 1);

	clean_up();
	return EXIT_SUCCESS;
}
