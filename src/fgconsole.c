/*
 * fgconsole.c - aeb - 960123 - Print foreground console
 */
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <getopt.h>
#include <linux/vt.h>
#include <linux/serial.h>
#include "getfd.h"
#include "nls.h"
#include "version.h"

static void __attribute__ ((noreturn))
usage(void)
{
	fprintf(stderr, _("%s version %s\n"
"\n"
"Usage: %s [options]\n"
"\n"
"Valid options are:\n"
"\n"
"	-h --help            display this help text\n"
"	-V --version         display program version\n"
"	-n --next-available  display number of next unallocated VT\n"),
			progname, PACKAGE_VERSION, progname);
	exit(1);
}

int
main(int argc, char **argv){
	struct vt_stat vtstat;
	int fd, vtno = -1, c, show_vt = 0;
	struct serial_struct sr;
	const struct option long_opts[] = {
	   { "help", no_argument, NULL, 'h' },
	   { "version", no_argument, NULL, 'V' },
	   { "next-available", no_argument, NULL, 'n' },
	   { NULL, 0, NULL, 0 } };

	setlocale(LC_ALL, "");
	bindtextdomain(PACKAGE_NAME, LOCALEDIR);
	textdomain(PACKAGE_NAME);

	set_progname(argv[0]);
	while (( c = getopt_long (argc, argv, "Vhn", long_opts, NULL)) != EOF) {
	  switch (c) {
	  case 'h':
	    usage();
	    exit (0);
	  case 'n':
	    show_vt = 1;
	    break;
	  case 'V':
	    print_version_and_exit();
	    break;
	  case '?':
	    usage();
	    exit(1);
	  }
	}
	
	fd = getfd(NULL);
	if (show_vt) {
	  if ((ioctl(fd, VT_OPENQRY, &vtno) < 0) || vtno == -1) {
	     perror (_("Couldn't read VTNO: "));
	     exit(2);
	  }
	  printf ("%d\n", vtno);
	  exit(0);
	}
	
	if (ioctl(fd, TIOCGSERIAL, &sr) == 0) {
	  printf ("serial\n");
	  exit (0);
	}
	
	if (ioctl(fd, VT_GETSTATE, &vtstat)) 
	  {
	    perror("fgconsole: VT_GETSTATE");
	    exit(1);
	  }
	printf("%d\n", vtstat.v_active);
	return 0;
}
