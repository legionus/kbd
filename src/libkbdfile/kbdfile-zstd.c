// SPDX-License-Identifier: LGPL-2.1-or-later

#include "config.h"

#include <sys/mman.h>

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <zstd.h>

#include <kbdfile.h>

#include "contextP.h"
#include "elf-note.h"

#define DL_SYMBOL_TABLE(M)		\
	M(ZSTD_createDStream)		\
	M(ZSTD_decompressStream)	\
	M(ZSTD_isError)			\
	M(ZSTD_getErrorName)		\
	M(ZSTD_freeDStream)

DL_SYMBOL_TABLE(DECLARE_SYM)

static int dlopen_note(void)
{
	static void *dl;

	ELF_NOTE_DLOPEN("zstd",
			"Support for uncompressing zstd-compressed files",
			ELF_NOTE_DLOPEN_PRIORITY_RECOMMENDED,
			"libzstd.so.1");

	return dlsym_many(&dl, "libzstd.so.1", DL_SYMBOL_TABLE(DLSYM_ARG) NULL);
}

FILE *kbdfile_decompressor_zstd(struct kbdfile *file)
{
	char errbuf[200];
	int retcode;
	FILE *outf = NULL;
	int infd = -1;
	int memfd = -1;

	ZSTD_DStream *dstream = NULL;

	retcode = dlopen_note();
	if (retcode < 0) {
		ERR(file->ctx, "zstd: can't load and resolve symbols: %s",
		    kbd_strerror(-retcode, errbuf, sizeof(errbuf)));
		return NULL;
	}

	retcode = -1;

	infd = open(file->pathname, O_RDONLY);
	if (infd < 0) {
		ERR(file->ctx, "unable to open xz-archive file: %s",
		    kbd_strerror(errno, errbuf, sizeof(errbuf)));
		goto cleanup;
	}

	memfd = memfd_create(file->pathname, MFD_CLOEXEC);
	if (memfd < 0) {
		ERR(file->ctx, "unable to open in-memory file: %s",
		    kbd_strerror(errno, errbuf, sizeof(errbuf)));
		goto cleanup;
	}

	dstream = sym_ZSTD_createDStream();
	if (!dstream) {
		ERR(file->ctx, "prepare zstd streaming decompressor failed");
		goto cleanup;
	}

	char inpbuf[BUFSIZ];
	char outbuf[BUFSIZ * 4];
	int eof = 0;

	ZSTD_inBuffer input   = { inpbuf, 0,              0 };
	ZSTD_outBuffer output = { outbuf, sizeof(outbuf), 0 };

	while (!eof || input.pos < input.size) {
		if (!eof && input.pos == input.size) {
			ssize_t r = read(infd, inpbuf, sizeof(inpbuf));

			if (r < 0) {
				ERR(file->ctx, "unable to read zstd-archive: %s",
				    kbd_strerror(errno, errbuf, sizeof(errbuf)));
				goto cleanup;
			} else if (r == 0) {
				eof = 1;
				input.src = inpbuf;
				input.size = 0;
				input.pos = 0;
			} else {
				input.src = inpbuf;
				input.size = (size_t) r;
				input.pos = 0;
			}
		}

		output.dst = outbuf;
		output.size = sizeof(outbuf);
		output.pos = 0;

		size_t res_decompress = sym_ZSTD_decompressStream(dstream, &output, &input);

		if (sym_ZSTD_isError(res_decompress)) {
			ERR(file->ctx, "zstd: unable to decompress stream: %s",
			    sym_ZSTD_getErrorName(res_decompress));
			goto cleanup;
		}

		size_t to_write = output.pos;
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

	if (eof && input.pos == input.size)
		retcode = 0;

cleanup:
	if (infd >= 0)
		close(infd);

	if (dstream)
		sym_ZSTD_freeDStream(dstream);

	if (retcode == 0) {
		lseek(memfd, 0, SEEK_SET);

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
