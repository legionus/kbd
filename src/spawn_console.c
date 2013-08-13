/*
 * Tiny test program for the `spawn console' key
 * (should not use signal; should not use sleep)
 * aeb - 941025
 *
 * Note: this functionality will probably go away and become
 * part of init. For the time being, be very careful when
 * you use this - if you have this in /etc/rc.local you should
 * start getty, not openvt, or anybody will have a root shell
 * with a single keystroke.
 */
#include <signal.h>
#include <errno.h>
#include <linux/kd.h>
#include <stdio.h>
#include <stdlib.h>	/* system */
#include <fcntl.h>	/* open */
#include <sys/ioctl.h>	/* ioctl */
#include <unistd.h>	/* sleep */

#include "kbd.h"

static void
sighup(int n __attribute__ ((unused))) {
    if (system("openvt -s -l bash") == -1) {
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
    if (ioctl(fd, KDSIGACCEPT, (long) SIGHUP)) {
      perror("KDSIGACCEPT");
      exit(1);
    }
    while(1)
      sleep(3600);
}
