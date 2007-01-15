/*
 * fgconsole.c - aeb - 960123 - Print foreground console
 */
#include <sys/ioctl.h>
#include <linux/vt.h>
#include "nls.h"

int
main(){
    struct vt_stat vtstat;

    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);

    /* replace 0 by getfd() for a somewhat more robust version */
    if (ioctl(0, VT_GETSTATE, &vtstat)) {
        perror("fgconsole: VT_GETSTATE");
	exit(1);
    }
    printf("%d\n", vtstat.v_active);
    return 0;
}
