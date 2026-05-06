/*
 * vcslayout.c
 *
 * Proof-of-concept XKB layout state indicator for Linux VT.
 *
 * It binds two LED triggers (kbd-shiftllock and kbd-shiftrlock),
 * tracks keyboard activity to refresh their brightness values and writes
 * the inferred current layout in the upper-right corner of /dev/vcs.
 */
#define _GNU_SOURCE

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <limits.h>
#include <signal.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <linux/input.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

struct led_binding {
	char dir[PATH_MAX];
	char trigger[PATH_MAX];
	char brightness[PATH_MAX];
	char saved_trigger[128];
};

static struct led_binding left_led;
static struct led_binding right_led;
static int left_bound;
static int right_bound;
static char input_device[PATH_MAX];

static const char *layout_for_state(int count, int left, int right, char **layouts);

static void die(const char *fmt, ...)
{
	va_list ap;

	va_start(ap, fmt);
	vfprintf(stderr, fmt, ap);
	va_end(ap);
	fputc('\n', stderr);
	exit(EXIT_FAILURE);
}

static void read_first_line(const char *path, char *buf, size_t size)
{
	FILE *fp = fopen(path, "r");

	if (!fp)
		die("%s: %s", path, strerror(errno));

	if (!fgets(buf, (int) size, fp)) {
		fclose(fp);
		die("%s: unable to read", path);
	}

	fclose(fp);
	buf[strcspn(buf, "\r\n")] = '\0';
}

static void write_string(const char *path, const char *value)
{
	FILE *fp = fopen(path, "w");

	if (!fp)
		die("%s: %s", path, strerror(errno));

	if (fprintf(fp, "%s\n", value) < 0) {
		fclose(fp);
		die("%s: %s", path, strerror(errno));
	}

	fclose(fp);
}

static void path_join(char *buf, size_t size, const char *dir, const char *name)
{
	int ret;

	ret = snprintf(buf, size, "%s/%s", dir, name);
	if (ret < 0 || (size_t) ret >= size)
		die("path too long: %s/%s", dir, name);
}

static int read_brightness(const char *path)
{
	char buf[32];

	read_first_line(path, buf, sizeof(buf));
	return atoi(buf) != 0;
}

static void current_trigger(const char *path, char *buf, size_t size)
{
	FILE *fp = fopen(path, "r");
	char line[4096];
	char *start, *end;

	if (!fp)
		die("%s: %s", path, strerror(errno));

	if (!fgets(line, sizeof(line), fp)) {
		fclose(fp);
		die("%s: unable to read", path);
	}

	fclose(fp);

	start = strchr(line, '[');
	end = strchr(line, ']');
	if (!start || !end || end <= start + 1)
		die("%s: unable to parse current trigger", path);

	*end = '\0';
	snprintf(buf, size, "%s", start + 1);
}

static int trigger_supported(const char *path, const char *needle)
{
	FILE *fp = fopen(path, "r");
	char line[4096];
	int found = 0;

	if (!fp)
		return 0;

	if (fgets(line, sizeof(line), fp))
		found = strstr(line, needle) != NULL;

	fclose(fp);
	return found;
}

static void restore_leds(void)
{
	if (left_bound)
		write_string(left_led.trigger, left_led.saved_trigger);
	if (right_bound)
		write_string(right_led.trigger, right_led.saved_trigger);
}

static void on_signal(int signo)
{
	(void) signo;
	restore_leds();
	_exit(128);
}

static void setup_signal_handlers(void)
{
	struct sigaction sa;

	memset(&sa, 0, sizeof(sa));
	sa.sa_handler = on_signal;
	sigemptyset(&sa.sa_mask);

	sigaction(SIGINT, &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);
	sigaction(SIGHUP, &sa, NULL);
}

static void init_led_binding(struct led_binding *led, const char *dir)
{
	snprintf(led->dir, sizeof(led->dir), "%s", dir);
	path_join(led->trigger, sizeof(led->trigger), dir, "trigger");
	path_join(led->brightness, sizeof(led->brightness), dir, "brightness");
	current_trigger(led->trigger, led->saved_trigger, sizeof(led->saved_trigger));
}

static void autodetect_leds(void)
{
	DIR *dp;
	struct dirent *de;

	dp = opendir("/sys/class/leds");
	if (!dp)
		die("/sys/class/leds: %s", strerror(errno));

	while ((de = readdir(dp))) {
		char dir[PATH_MAX];
		char trigger[PATH_MAX];
		char current[128];

		if (de->d_name[0] == '.')
			continue;

		snprintf(dir, sizeof(dir), "/sys/class/leds/%s", de->d_name);
		path_join(trigger, sizeof(trigger), dir, "trigger");

		if (access(trigger, R_OK) != 0)
			continue;

		current_trigger(trigger, current, sizeof(current));
		if (strcmp(current, "kbd-shiftllock") == 0) {
			init_led_binding(&left_led, dir);
			left_bound = 1;
		} else if (strcmp(current, "kbd-shiftrlock") == 0) {
			init_led_binding(&right_led, dir);
			right_bound = 1;
		}
	}

	rewinddir(dp);

	while ((de = readdir(dp))) {
		char dir[PATH_MAX];
		char trigger[PATH_MAX];

		if (de->d_name[0] == '.')
			continue;

		snprintf(dir, sizeof(dir), "/sys/class/leds/%s", de->d_name);
		path_join(trigger, sizeof(trigger), dir, "trigger");

		if (access(trigger, R_OK) != 0)
			continue;

		if (!trigger_supported(trigger, "kbd-shiftllock") ||
		    !trigger_supported(trigger, "kbd-shiftrlock"))
			continue;

		if (!left_led.dir[0]) {
			init_led_binding(&left_led, dir);
			continue;
		}

		if (!right_led.dir[0] && strcmp(dir, left_led.dir) != 0) {
			init_led_binding(&right_led, dir);
			break;
		}
	}

	closedir(dp);

	if (!left_led.dir[0] || !right_led.dir[0])
		die("unable to find two LED devices supporting keyboard lock triggers");
}

static void bind_leds(void)
{
	write_string(left_led.trigger, "kbd-shiftllock");
	write_string(right_led.trigger, "kbd-shiftrlock");
	left_bound = 1;
	right_bound = 1;
}

static int has_suffix(const char *str, const char *suffix)
{
	size_t str_len = strlen(str);
	size_t suffix_len = strlen(suffix);

	return str_len >= suffix_len &&
	       strcmp(str + str_len - suffix_len, suffix) == 0;
}

static int try_open_keyboard_dir(const char *base)
{
	DIR *dp;
	struct dirent *de;

	dp = opendir(base);
	if (!dp)
		return -1;

	while ((de = readdir(dp))) {
		char path[PATH_MAX];
		int fd;

		if (de->d_name[0] == '.')
			continue;
		if (!has_suffix(de->d_name, "-kbd"))
			continue;

		path_join(path, sizeof(path), base, de->d_name);
		fd = open(path, O_RDONLY | O_CLOEXEC);
		if (fd < 0)
			continue;

		snprintf(input_device, sizeof(input_device), "%s", path);
		closedir(dp);
		return fd;
	}

	closedir(dp);
	return -1;
}

static int init_keyboard_watch(void)
{
	int fd;

	fd = try_open_keyboard_dir("/dev/input/by-path");
	if (fd >= 0)
		return fd;

	return try_open_keyboard_dir("/dev/input/by-id");
}

static int wait_keyboard_event(int fd)
{
	struct input_event events[16];
	struct pollfd pfd;
	ssize_t len;
	size_t i, count;

	pfd.fd = fd;
	pfd.events = POLLIN;

	for (;;) {
		if (poll(&pfd, 1, -1) < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}

		if (!(pfd.revents & POLLIN))
			continue;

		len = read(fd, events, sizeof(events));
		if (len < 0) {
			if (errno == EINTR)
				continue;
			return -1;
		}

		if (len < (ssize_t) sizeof(struct input_event))
			continue;

		count = (size_t) len / sizeof(struct input_event);
		for (i = 0; i < count; i++) {
			if (events[i].type == EV_KEY)
				return 0;
		}
	}
}

static void report_mode(int evdev_fd)
{
	fprintf(stderr, "left LED:        %s\n", left_led.dir);
	fprintf(stderr, "right LED:       %s\n", right_led.dir);
	if (evdev_fd >= 0) {
		fprintf(stderr, "keyboard device: %s\n", input_device);
		fprintf(stderr, "mode:            event-driven (evdev)\n");
	} else {
		fprintf(stderr, "mode:            polling (1s fallback)\n");
	}
}

static int vcs_columns(void)
{
	int fd;
	unsigned char hdr[2];

	fd = open("/dev/vcsa", O_RDONLY);
	if (fd < 0)
		fd = open("/dev/vcsa0", O_RDONLY);
	if (fd < 0)
		die("/dev/vcsa: %s", strerror(errno));

	if (read(fd, hdr, sizeof(hdr)) != (ssize_t) sizeof(hdr)) {
		close(fd);
		die("/dev/vcsa: unable to read geometry");
	}

	close(fd);
	return hdr[1];
}

static void write_label(const char *label)
{
	int fd, cols;
	off_t pos;
	size_t len = strlen(label);

	fd = open("/dev/vcs", O_WRONLY);
	if (fd < 0)
		fd = open("/dev/vcs0", O_WRONLY);
	if (fd < 0)
		die("/dev/vcs: %s", strerror(errno));

	cols = vcs_columns();
	if ((int) len > cols)
		die("label too wide for console");

	pos = (off_t) (cols - (int) len);
	if (lseek(fd, pos, SEEK_SET) < 0) {
		close(fd);
		die("/dev/vcs: %s", strerror(errno));
	}

	if (write(fd, label, len) != (ssize_t) len) {
		close(fd);
		die("/dev/vcs: %s", strerror(errno));
	}

	close(fd);
}

static void update_label(int count, char **layouts, char *last_label, size_t size)
{
	const char *layout;
	char label[64];
	int left = read_brightness(left_led.brightness);
	int right = read_brightness(right_led.brightness);

	layout = layout_for_state(count, left, right, layouts);
	snprintf(label, sizeof(label), "[%s]", layout);

	if (strcmp(label, last_label) != 0) {
		write_label(label);
		snprintf(last_label, size, "%s", label);
	}
}

static const char *layout_for_state(int count, int left, int right, char **layouts)
{
	switch (count) {
	case 1:
		return layouts[0];
	case 2:
		return (left == right) ? layouts[0] : layouts[1];
	case 3:
		if (left && !right)
			return layouts[1];
		if (!left && right)
			return layouts[2];
		return layouts[0];
	case 4:
		if (!left && !right)
			return layouts[0];
		if (left && !right)
			return layouts[1];
		if (left && right)
			return layouts[2];
		return layouts[3];
	default:
		return "?";
	}
}

int main(int argc, char **argv)
{
	char *layouts_arg, *layouts[4], *tmp, *tok;
	char last_label[64] = "";
	int count = 0, evdev_fd;

	if (argc != 2)
		die("usage: %s us,ru[,fr[,de]]", argv[0]);

	layouts_arg = strdup(argv[1]);
	if (!layouts_arg)
		die("out of memory");

	tmp = layouts_arg;
	while ((tok = strsep(&tmp, ",")) != NULL) {
		if (*tok == '\0')
			die("empty layout label");
		if (count >= 4)
			die("expected 1 to 4 layout labels");
		layouts[count++] = tok;
	}

	if (count < 1)
		die("expected 1 to 4 layout labels");

	atexit(restore_leds);
	setup_signal_handlers();
	autodetect_leds();
	bind_leds();
	update_label(count, layouts, last_label, sizeof(last_label));
	evdev_fd = init_keyboard_watch();
	report_mode(evdev_fd);

	for (;;) {
		if (evdev_fd >= 0) {
			if (wait_keyboard_event(evdev_fd) < 0) {
				close(evdev_fd);
				evdev_fd = -1;
				report_mode(evdev_fd);
				continue;
			}
			update_label(count, layouts, last_label, sizeof(last_label));
			continue;
		}

		sleep(1);
		update_label(count, layouts, last_label, sizeof(last_label));
	}
}
