#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include "getfd.h"
#include "xmalloc.h"
#include "nls.h"
#include "version.h"

#ifndef USE_LIBC
/* There is such function in libc5 but it doesn't work for me [libc 5.4.13] */
#include "wctomb.c"
#define wctomb our_wctomb
#endif

static int
ud_compar(const void *u1, const void *u2){
	unsigned short fp1 = ((struct unipair *) u1)->fontpos;
	unsigned short fp2 = ((struct unipair *) u2)->fontpos;
	return (int) fp1 - (int) fp2;
}

int
main(int argc, char **argv){
	int sortflag = 0;
	char mb[]={0,0,0,0,0,0,0,0};
	unsigned mb_length;
	int fd, ct;
	struct unimapdesc ud;
	int i;

	set_progname(argv[0]);

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE, LOCALEDIR);
	textdomain(PACKAGE);

	if (argc == 2 && !strcmp(argv[1], "-V"))
		print_version_and_exit();

	if (argc == 2 && !strcmp(argv[1], "-s")) {
		sortflag = 1;
		argc--;
	}
	if (argc != 1) {
		fprintf(stderr, _("Usage:\n\t%s [-s]\n"), progname);
		exit(1);
	}

	fd = getfd();

	ud.entry_ct = ct = 0;
	ud.entries = 0;
	if (ioctl(fd, GIO_UNIMAP, &ud)) {
		if (errno != ENOMEM) {
			perror("GIO_UNIMAP");
			exit(1);
		}
		ct = ud.entry_ct;
		ud.entries = xmalloc(ct * sizeof(struct unipair));
		if (ioctl(fd, GIO_UNIMAP, &ud)) {
			perror("GIO_UNIMAP (2)");
			exit(1);
		}
		if (ct != ud.entry_ct)
			fprintf(stderr,
				_("strange... ct changed from %d to %d\n"),
				ct, ud.entry_ct);

		/* someone could change the unimap between our
		   first and second ioctl, so the above errors
		   are not impossible */
	}

	if (sortflag) {
		printf("# sorted kernel unimap - count=%d\n", ud.entry_ct);
		/* sort and merge entries */
		qsort (ud.entries, ud.entry_ct, sizeof(ud.entries[0]),
		       ud_compar);
		for(i=0; i<ud.entry_ct && i<ct; i++) {
			int fp = ud.entries[i].fontpos;
			printf("0x%03x\tU+%04x", fp, ud.entries[i].unicode);
			while (i+1 < ud.entry_ct && i+1 < ct &&
			       ud.entries[i+1].fontpos == fp)
				printf(" U+%04x", ud.entries[++i].unicode);
			printf("\n");
		}
	} else {
		printf("# kernel unimap - count=%d\n", ud.entry_ct);
		for(i=0; i<ud.entry_ct && i<ct; i++) {
			mb_length=wctomb (mb, ud.entries[i].unicode);
			mb[ (mb_length > 6) ? 0 : mb_length ] = 0 ;
			printf("0x%03x\tU+%04x\t# %s \n",
			       ud.entries[i].fontpos,
			       ud.entries[i].unicode, mb);
		}
	}

	return 0;
}
