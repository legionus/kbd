// SPDX-License-Identifier: LGPL-2.1-or-later

#include "config.h"

#include <sys/mman.h>

#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <lzma.h>

#include <kbdfile.h>

#include "contextP.h"
#include "elf-note.h"

#define DL_SYMBOL_TABLE(M)	\
	M(lzma_stream_decoder)	\
	M(lzma_code)		\
	M(lzma_end)

DL_SYMBOL_TABLE(DECLARE_SYM)

static int dlopen_lzma(void)
{
	static void *dl;

	ELF_NOTE_DLOPEN("lzma",
			"Support for uncompressing xz-compressed files",
			ELF_NOTE_DLOPEN_PRIORITY_RECOMMENDED,
			"liblzma.so.5");

	return dlsym_many(&dl, "liblzma.so.5", DL_SYMBOL_TABLE(DLSYM_ARG) NULL);
}

FILE *kbdfile_decompressor_lzma(struct kbdfile *file)
{
	char errbuf[200];
	int retcode;
	FILE *outf = NULL;
	int infd = -1;
	int memfd = -1;

	lzma_stream strm = LZMA_STREAM_INIT;
	lzma_action action = LZMA_RUN;

	retcode = dlopen_lzma();
	if (retcode < 0) {
		ERR(file->ctx, "lzma: can't load and resolve symbols: %s",
		    kbd_strerror(-retcode, errbuf, sizeof(errbuf)));
		return NULL;
	}

	retcode = -1;

	if (sym_lzma_stream_decoder(&strm, UINT64_MAX, LZMA_CONCATENATED) != LZMA_OK)
		goto cleanup;

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

	uint8_t inpbuf[BUFSIZ];
	uint8_t outbuf[BUFSIZ * 4];
	int eof = 0;

	while (1) {
		if (!eof && strm.avail_in == 0) {
			ssize_t read_bytes = read(infd, inpbuf, sizeof(inpbuf));

			if (read_bytes < 0) {
				ERR(file->ctx, "unable to read xz-archive: %s",
				    kbd_strerror(errno, errbuf, sizeof(errbuf)));
				goto cleanup;
			} else if (read_bytes == 0) {
				eof = 1;
				action = LZMA_FINISH;
			} else {
				strm.next_in = inpbuf;
				strm.avail_in = (size_t) read_bytes;
			}
		}

		strm.next_out = outbuf;
		strm.avail_out = sizeof(outbuf);

		lzma_ret code_ret = sym_lzma_code(&strm, action);

		if (code_ret != LZMA_OK && code_ret != LZMA_STREAM_END) {
			ERR(file->ctx, "lzma: decode error (return code %d)", code_ret);
			goto cleanup;
		}

		size_t to_write = sizeof(outbuf) - strm.avail_out;
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

		if (code_ret == LZMA_STREAM_END) {
			retcode = 0;
			goto cleanup;
		}
	}

cleanup:
	if (infd >= 0)
		close(infd);

	sym_lzma_end(&strm);

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
