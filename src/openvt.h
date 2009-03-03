#ifndef _OPENVT_H
#define _OPENVT_H

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <fcntl.h>
#include <dirent.h>
#include <pwd.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/vt.h>
#include <sys/types.h>
#include <sys/wait.h>

#include "getfd.h"

#ifndef NAME_MAX
#define NAME_MAX 255
#endif

void usage(int);
char *authenticate_user(int);

/*
 * There must be a universal way to find these!
 */

#define TRUE (1)
#define FALSE (0)


/*
 * Where your VTs are hidden
 */
#ifdef __linux__
#define VTNAME "/dev/tty%d"
#define VTNAME2 "/dev/vc/%d"
#endif

#ifdef ESIX_5_3_2_D
#define	VTBASE		"/dev/vt%02d"
#endif

#endif /* _OPENVT_H */
