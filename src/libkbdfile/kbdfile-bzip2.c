// SPDX-License-Identifier: LGPL-2.1-or-later

#include "config.h"

#include <sys/mman.h>

#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <bzlib.h>

#include <kbdfile.h>

#include "contextP.h"
#include "elf-note.h"

#define DL_SYMBOL_TABLE(M)	\
	M(BZ2_bzopen)		\
	M(BZ2_bzclose)		\
	M(BZ2_bzread)		\
	M(BZ2_bzerror)

DL_SYMBOL_TABLE(DECLARE_SYM)

static int dlopen_note(void)
{
	static void *dl;

	ELF_NOTE_DLOPEN("bzip2",
			"Support for uncompressing bzip2-compressed files",
			ELF_NOTE_DLOPEN_PRIORITY_RECOMMENDED,
			"libbz2.so.1");

	return dlsym_many(&dl, "libbz2.so.1", DL_SYMBOL_TABLE(DLSYM_ARG) NULL);
}

FILE *kbdfile_decompressor_bzip2(struct kbdfile *file)
{
	char errbuf[200];
	int retcode;
	BZFILE *zf = NULL;
	FILE *outf = NULL;
	int memfd = -1;

	retcode = dlopen_note();
	if (retcode < 0) {
		ERR(file->ctx, "bzip2: can't load and resolve symbols: %s",
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

	zf = sym_BZ2_bzopen(file->pathname, "rb");
	if (!zf) {
		ERR(file->ctx, "bzip2: unable to open archive: %s",
		    kbd_strerror(errno, errbuf, sizeof(errbuf)));
		goto cleanup;
	}

	while (1) {
		int read_bytes;
		char outbuf[BUFSIZ];

		read_bytes = sym_BZ2_bzread(zf, outbuf, sizeof(outbuf));
		if (read_bytes < 0) {
			int zerrno;
			ERR(file->ctx, "bzip2: read error: %s",
			    sym_BZ2_bzerror(zf, &zerrno));
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
	if (zf)
		sym_BZ2_bzclose(zf);

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
