// SPDX-License-Identifier: LGPL-2.1-or-later

#include "config.h"

#include <sys/mman.h>

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <zlib.h>

#include <kbdfile.h>

#include "contextP.h"
#include "elf-note.h"

#define DL_SYMBOL_TABLE(M)	\
	M(gzclose)		\
	M(gzopen)		\
	M(gzerror)		\
	M(gzread)

DL_SYMBOL_TABLE(DECLARE_SYM)

static int dlopen_note(void)
{
	static void *dl;

	ELF_NOTE_DLOPEN("zlib",
			"Support for uncompressing zlib-compressed files",
			ELF_NOTE_DLOPEN_PRIORITY_RECOMMENDED,
			"libz.so.1");

	return dlsym_many(&dl, "libz.so.1", DL_SYMBOL_TABLE(DLSYM_ARG) NULL);
}

FILE *kbdfile_decompressor_zlib(struct kbdfile *file)
{
	char errbuf[200];
	int retcode;
	gzFile gzf = NULL;
	FILE *outf = NULL;
	int memfd = -1;

	retcode = dlopen_note();
	if (retcode < 0) {
		ERR(file->ctx, "zlib: can't load and resolve symbols: %s",
		    kbd_strerror(-retcode, errbuf, sizeof(errbuf)));
		return NULL;
	}

	retcode = -1;

	memfd = memfd_create(file->pathname, MFD_CLOEXEC);
	if (memfd < 0) {
		ERR(file->ctx, "unable to open in-memory file: %s",
		    kbd_strerror(errno, errbuf, sizeof(errbuf)));
		goto cleanup;
	}

	gzf = sym_gzopen(file->pathname, "rb");
	if (!gzf) {
		ERR(file->ctx, "zlib: unable to open archive: %s",
		    kbd_strerror(errno, errbuf, sizeof(errbuf)));
		goto cleanup;
	}

	while (1) {
		int read_bytes;
		char outbuf[BUFSIZ];

		read_bytes = sym_gzread(gzf, outbuf, sizeof(outbuf));

		if (read_bytes < 0) {
			int gzerr;
			ERR(file->ctx, "zlib: read error: %s",
			    sym_gzerror(gzf, &gzerr));
			goto cleanup;
		}
		if (read_bytes == 0) {
			retcode = 0;
			break;
		}

		size_t to_write = (size_t) read_bytes;
		size_t written = 0;

		while (written < to_write) {
			ssize_t w = write(memfd, outbuf + written, to_write - written);

			if (w < 0) {
				if (errno == EINTR)
					continue;

				ERR(file->ctx, "unable to write data: %s",
				    kbd_strerror(errno, errbuf, sizeof(errbuf)));

				goto cleanup;
			}
			written += (size_t) w;
		}
	}

cleanup:
	sym_gzclose(gzf);

	if (retcode == 0) {
		lseek(memfd, 0L, SEEK_SET);

		outf = fdopen(memfd, "r");
		if (!outf) {
			ERR(file->ctx, "unable to create file stream from file descriptor: %s",
			    kbd_strerror(errno, errbuf, sizeof(errbuf)));

			if (memfd >= 0)
				close(memfd);
		}
	} else if (memfd >= 0) {
		close(memfd);
	}

	return outf;
}
