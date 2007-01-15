/*
 * Tiny test program for the `spawn console' key
 * (should not use signal; should not use sleep)
 * aeb - 941025
 *
 * Note: this functionality will probably go away and become
 * part of init. For the time being, be very careful when
 * you use this - if you have this in /etc/rc.local you should
 * start getty, not open, or anybody will have a root shell
 * with a single keystroke.
 */
#include <signal.h>
#include <linux/kd.h>

void
sighup(){
    system("open -s -l bash");
    signal(SIGHUP, sighup);
}

main(){
    int fd;

    fd = open("/dev/console", 0);
    if (fd < 0)
      fd = 0;
    signal(SIGHUP, sighup);
    ioctl(fd, KDSIGACCEPT, (long) SIGHUP);
    while(1)
      sleep(3600);
}
