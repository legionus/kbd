/*
 * Tiny test program for the `spawn console' key
 * (should not use signal; should not use sleep)
 * aeb - 941025
 *
 */
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <errno.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <linux/kd.h>

#include "kbd.h"

static void
sighup(int n __attribute__ ((unused))) {
    if (system("openvt -s -l -- login -h spawn") == -1) {
	perror("system");
	exit(1);
    }
    signal(SIGHUP, sighup);
}

int
main(void) {
    int fd;

    fd = open("/dev/tty0", 0);
    if (fd < 0 && errno == ENOENT)
      fd = open("/dev/vc/0", 0);
    if (fd < 0)
      fd = 0;
    signal(SIGHUP, sighup);
    ioctl(fd, KDSIGACCEPT, (long) SIGHUP);
    while(1)
      sleep(3600);
}
