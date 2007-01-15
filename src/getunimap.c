#include <stdio.h>
#include <errno.h>
#include <asm/ioctls.h>
#include <linux/kd.h>
#include "nls.h"

#ifndef USE_LIBC
/* There is such function in libc5 but it doesn't work for me [libc 5.4.13] */
#include "wctomb.c"
#define wctomb our_wctomb
#endif

extern int getfd();
extern char *malloc();

main(){
    char mb[]={0,0,0,0,0,0,0,0};
    unsigned mb_length;
    int fd, ct;
    struct unimapdesc ud;
    int i;

    setlocale(LC_ALL, "");
    bindtextdomain(PACKAGE, LOCALEDIR);
    textdomain(PACKAGE);

    fd = getfd();

    ud.entry_ct = ct = 0;
    ud.entries = 0;
    if (ioctl(fd, GIO_UNIMAP, &ud)) {
	if (errno != ENOMEM) {
	    perror("GIO_UNIMAP");
	    exit(1);
	}
	ct = ud.entry_ct;
	ud.entries = (struct unipair *) malloc(ct * sizeof(struct unipair));
	if (ioctl(fd, GIO_UNIMAP, &ud)) {
	    perror("GIO_UNIMAP (2)");
	    exit(1);
	}
	if (ct != ud.entry_ct)
	  fprintf(stderr, _("strange... ct changed from %d to %d\n"),
		  ct, ud.entry_ct);

	/* someone could change the unimap between our
	   first and second ioctl, so the above errors
	   are not impossible */
    }
    printf("# count=%d\n", ud.entry_ct);
    for(i=0; i<ud.entry_ct && i<ct; i++) {
	 mb_length=wctomb (mb, ud.entries[i].unicode);
	 mb[ (mb_length > 6) ? 0 : mb_length ] = 0 ;
	 printf("0x%03x\tU+%04x\t# %s \n", ud.entries[i].fontpos,
		ud.entries[i].unicode, mb);
    }
}
