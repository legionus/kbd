#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/kd.h>
#include "getfd.h"

int
main(int argc, char **argv){
	int fd, indx, red, green, blue;
	unsigned char cmap[48];

	fd = getfd(NULL);
	if (ioctl(fd, GIO_CMAP, cmap))
		perror("GIO_CMAP");
	if (argc != 5) {
		fprintf(stderr, "call: setpalette index red green blue\n");
		exit(1);
	}
	indx = atoi(argv[1]);
	red = atoi(argv[2]);
	green = atoi(argv[3]);
	blue = atoi(argv[4]);
	if (indx < 0 || red < 0 || green < 0 || blue < 0 ||
	    indx > 15 || red > 255 || green > 255 || blue > 255) {
		fprintf(stderr, "indx must be in 0..15, colors in 0..255\n");
		exit(1);
	}
	cmap[3*indx] = red;
	cmap[3*indx+1] = green;
	cmap[3*indx+2] = blue;
	if (ioctl(fd, PIO_CMAP, cmap))
		perror("PIO_CMAP");
	return 0;
}
