#define _GNU_SOURCE

#include <linux/kd.h>

#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>
#include <dlfcn.h>

#ifdef __GLIBC__
typedef unsigned long ioctl_request_t;
#else
typedef int ioctl_request_t;
#endif

static int (*real_ioctl)(int fd, ioctl_request_t, ...) = NULL;
static int outfd = 1;

struct translate_names {
	unsigned long id;
	const char *name;
};

static struct translate_names kd_request[] = {
	{ GIO_FONT, "GIO_FONT" },
	{ PIO_FONT, "PIO_FONT" },
	{ GIO_FONTX, "GIO_FONTX" },
	{ PIO_FONTX, "PIO_FONTX" },
	{ PIO_FONTRESET, "PIO_FONTRESET" },
	{ GIO_CMAP, "GIO_CMAP" },
	{ PIO_CMAP, "PIO_CMAP" },
	{ KDGETLED, "KDGETLED" },
	{ KDSETLED, "KDSETLED" },
	{ KDGKBLED, "KDGKBLED" },
	{ KDSKBLED, "KDSKBLED" },
	{ KDGKBTYPE, "KDGKBTYPE" },
	{ KDGETMODE, "KDGETMODE" },
	{ KDSETMODE, "KDSETMODE" },
	{ GIO_SCRNMAP, "GIO_SCRNMAP" },
	{ PIO_SCRNMAP, "PIO_SCRNMAP" },
	{ GIO_UNISCRNMAP, "GIO_UNISCRNMAP" },
	{ PIO_UNISCRNMAP, "PIO_UNISCRNMAP" },
	{ GIO_UNIMAP, "GIO_UNIMAP" },
	{ PIO_UNIMAP, "PIO_UNIMAP" },
	{ PIO_UNIMAPCLR, "PIO_UNIMAPCLR" },
	{ KDFONTOP, "KDFONTOP" },
	{ KDGKBMODE, "KDGKBMODE" },
	{ KDSKBMODE, "KDSKBMODE" },
	{ KDGKBMETA, "KDGKBMETA" },
	{ KDGKBENT, "KDGKBENT" },
	{ KDSKBENT, "KDSKBENT" },
	{ KDGKBSENT, "KDGKBSENT" },
	{ KDSKBSENT, "KDSKBSENT" },
	{ KDGKBDIACR, "KDGKBDIACR" },
	{ KDSKBDIACR, "KDSKBDIACR" },
	{ KDGKBDIACRUC, "KDGKBDIACRUC" },
	{ KDSKBDIACRUC, "KDSKBDIACRUC" },
	{ 0, NULL }
};

static struct translate_names kd_mode[] = {
	{ KD_TEXT, "KD_TEXT" },
	{ KD_GRAPHICS, "KD_GRAPHICS" },
	{ KD_TEXT0, "KD_TEXT0" },
	{ KD_TEXT1, "KD_TEXT1" },
	{ 0, NULL }
};

static struct translate_names kb_mode[] = {
	{ K_RAW, "K_RAW" },
	{ K_XLATE, "K_XLATE" },
	{ K_MEDIUMRAW, "K_MEDIUMRAW" },
	{ K_UNICODE, "K_UNICODE" },
	{ K_OFF, "K_OFF" },
	{ 0, NULL }
};

static struct translate_names kb_meta[] = {
	{ K_METABIT, "K_METABIT" },
	{ K_ESCPREFIX, "K_ESCPREFIX" },
	{ 0, NULL }
};

static struct translate_names kd_font_op[] = {
	{ KD_FONT_OP_SET, "KD_FONT_OP_SET" },
	{ KD_FONT_OP_GET, "KD_FONT_OP_GET" },
	{ KD_FONT_OP_SET_DEFAULT, "KD_FONT_OP_SET_DEFAULT" },
	{ KD_FONT_OP_COPY, "KD_FONT_OP_COPY" },
	{ 0, NULL }
};

/*
 *  Font switching
 *
 *  Currently we only support fonts up to 32 pixels wide, at a maximum height
 *  of 32 pixels. Userspace fontdata is stored with 32 bytes (shorts/ints, 
 *  depending on width) reserved for each character which is kinda wasty, but 
 *  this is done in order to maintain compatibility with the EGA/VGA fonts. It 
 *  is up to the actual low-level console-driver convert data into its favorite
 *  format (maybe we should add a `fontoffset' field to the `display'
 *  structure so we won't have to convert the fontdata all the time.
 *  /Jes
 */

#define max_font_size 65536

static char *
translate_ntoa(struct translate_names names[], unsigned long type, char *buf, size_t len)
{
	int i = 0;
	while (names[i].name) {
		if (names[i].id == type) {
			snprintf(buf, len, "%s", names[i].name);
			return buf;
		}
		i++;
	}
	snprintf(buf, len, "0x%04lX", type);
	return buf;
}

static void
print_chars(const char *prefix, const unsigned char *s, size_t len, size_t split)
{
	dprintf(outfd, "%s", prefix);

	for (size_t i = 0; i < len; i++) {
		dprintf(outfd, "0x%02X", s[i]);

		if (i+1 >= len) {
			continue;
		}

		dprintf(outfd, " ");

		if (i % split == (split - 1)) {
			dprintf(outfd, "\n");
			dprintf(outfd, "%s", prefix);
		}
	}
}

static void
print_unimapinit(struct unimapinit *ui)
{
	dprintf(outfd, "\tstruct unimapinit *arg {\n");
	dprintf(outfd, "\t\tunsigned short advised_hashsize = %u\n", ui->advised_hashsize);
	dprintf(outfd, "\t\tunsigned short advised_hashstep = %u\n", ui->advised_hashstep);
	dprintf(outfd, "\t\tunsigned short advised_hashlevel = %u\n", ui->advised_hashlevel);
	dprintf(outfd, "\t};\n");
}

static void
print_unimapdesc(struct unimapdesc *ud)
{
	size_t i = 0;

	dprintf(outfd, "\tstruct unimapdesc *arg {\n");
	dprintf(outfd, "\t\tunsigned short entry_ct = %u\n", ud->entry_ct);

	if (ud->entries) {
		dprintf(outfd, "\t\tstruct unipair *entries = {\n");
		for (i = 0; i < ud->entry_ct; i++) {
			dprintf(outfd, "\t\t\t{\n");
			dprintf(outfd, "\t\t\t\tunsigned short unicode = 0x%04x\n", ud->entries[i].unicode);
			dprintf(outfd, "\t\t\t\tunsigned short fontpos = %u\n", ud->entries[i].fontpos);
			dprintf(outfd, "\t\t\t}\n");
		}
		dprintf(outfd, "\t\t}\n");
	} else {
		dprintf(outfd, "\t\tstruct unipair *entries = NULL\n");
	}
	dprintf(outfd, "\t};\n");
}

static void
print_consolefontdesc(struct consolefontdesc *cfd)
{
	size_t width = 8;
	size_t size = (width + 7) / 8 * 32 * cfd->charcount;

	dprintf(outfd, "\tstruct consolefontdesc * {\n");
	dprintf(outfd, "\t\tunsigned short charcount = %u\n", cfd->charcount);
	dprintf(outfd, "\t\tunsigned short charheight = %u\n", cfd->charheight);
	dprintf(outfd, "\t\tchar *chardata = {\n");
	print_chars("\t\t\t", (unsigned char *) cfd->chardata, size, 8);
	dprintf(outfd, "\n");
	dprintf(outfd, "\t\t}\n");
	dprintf(outfd, "\t};");
}

static void
print_console_font_op(struct console_font_op *op)
{
	size_t size;
	char buf[4096];

	if (!op->data) {
		fprintf(stderr, "bad data\n");
		exit(1);
	}

	if (op->charcount > 512) {
		fprintf(stderr, "font has too many chars\n");
		exit(1);
	}

	if (op->width <= 0 || op->width > 32 || op->height > 32) {
		fprintf(stderr, "wrong font parameters\n");
		exit(1);
	}

	size = (op->width + 7) / 8 * 32 * op->charcount;

	if (size > max_font_size) {
		fprintf(stderr, "font too big\n");
		exit(1);
	}

	dprintf(outfd, "\tstruct console_font_op *arg {\n");
	dprintf(outfd, "\t\tunsigned int op = %s\n", translate_ntoa(kd_font_op, op->op, buf, sizeof(buf)));
	dprintf(outfd, "\t\tunsigned int flags = %u\n", op->flags);
	dprintf(outfd, "\t\tunsigned int width = %u\n", op->width);
	dprintf(outfd, "\t\tunsigned int height = %u\n", op->height);
	dprintf(outfd, "\t\tunsigned int charcount = %u\n", op->charcount);
	dprintf(outfd, "\t\tunsigned char *data = {\n");
	print_chars("\t\t\t", op->data, size, 8);
	dprintf(outfd, "\n");
	dprintf(outfd, "\t\t}\n");
	dprintf(outfd, "\t};\n");
}

static void
print_kbentry(struct kbentry * ke)
{
	dprintf(outfd, "\tstruct kbentry *arg {\n");
	dprintf(outfd, "\t\tunsigned char kb_table = %u\n", ke->kb_table);
	dprintf(outfd, "\t\tunsigned char kb_index = %u\n", ke->kb_index);
	dprintf(outfd, "\t\tunsigned short kb_value = 0x%04X\n", ke->kb_value);
	dprintf(outfd, "\t};\n");
}

static void
print_kbsentry(struct kbsentry * ks)
{
	size_t sz = strlen((char *) ks->kb_string);

	dprintf(outfd, "\tstruct kbsentry *arg {\n");
	dprintf(outfd, "\t\tunsigned char kb_func = %u\n", ks->kb_func);
	dprintf(outfd, "\t\tunsigned char kb_string[%ld] = {\n", sz);
	print_chars("\t\t\t", ks->kb_string, sz, 16);
	dprintf(outfd, "\n");
	dprintf(outfd, "\t\t}\n");
	dprintf(outfd, "\t};\n");
}

static void
print_kbdiacrs(struct kbdiacrs * kd)
{
	size_t i;
	dprintf(outfd, "\tstruct kbdiacrs *arg {\n");
	dprintf(outfd, "\t\tunsigned int kb_cnt = %u\n", kd->kb_cnt);
	dprintf(outfd, "\t\tstruct kbdiacr kbdiacr[256] = {\n");
	for (i = 0; i < kd->kb_cnt; i++) {
		dprintf(outfd, "\t\t\t{\n");
		dprintf(outfd, "\t\t\t\tunsigned int diacr  = 0x%04x\n", kd->kbdiacr[i].diacr);
		dprintf(outfd, "\t\t\t\tunsigned int base   = 0x%04x\n", kd->kbdiacr[i].base);
		dprintf(outfd, "\t\t\t\tunsigned int result = 0x%04x\n", kd->kbdiacr[i].result);
		dprintf(outfd, "\t\t\t}\n");
	}
	dprintf(outfd, "\t\t}\n");
	dprintf(outfd, "\t};\n");
}

static void
print_kbdiacrsuc(struct kbdiacrsuc * kd)
{
	size_t i;
	dprintf(outfd, "\tstruct kbdiacrsuc *arg {\n");
	dprintf(outfd, "\t\tunsigned int kb_cnt = %u\n", kd->kb_cnt);
	dprintf(outfd, "\t\tstruct kbdiacruc kbdiacruc[256] = {\n");
	for (i = 0; i < kd->kb_cnt; i++) {
		dprintf(outfd, "\t\t\t{\n");
		dprintf(outfd, "\t\t\t\tunsigned int diacr  = 0x%04x\n", kd->kbdiacruc[i].diacr);
		dprintf(outfd, "\t\t\t\tunsigned int base   = 0x%04x\n", kd->kbdiacruc[i].base);
		dprintf(outfd, "\t\t\t\tunsigned int result = 0x%04x\n", kd->kbdiacruc[i].result);
		dprintf(outfd, "\t\t\t}\n");
	}
	dprintf(outfd, "\t\t}\n");
	dprintf(outfd, "\t};\n");
}

int ioctl(int fd, ioctl_request_t request, ...)
{
	char buf0[4096];
	char buf1[4096];
	int rc = 0;
	unsigned int arg_i = 0;
	char arg_char = 0;
	ioctl_request_t arg = 0;
	va_list ap;

	if (!real_ioctl) {
		real_ioctl = dlsym(RTLD_NEXT, "ioctl");
		if (!real_ioctl) {
			fprintf(stderr, "dlsym(ioctl): %s\n", dlerror());
			exit(1);
		}

		char *env = getenv("LIBTSWRAP_OUTPUT");
		if (env) {
			outfd = open(env, O_WRONLY|O_CREAT|O_TRUNC, S_IRUSR|S_IWUSR|S_IRGRP|S_IROTH);
			if (outfd < 0) {
				fprintf(stderr, "open(%s): %s\n", env, strerror(errno));
				exit(1);
			}
		}
	}

	va_start(ap, request);
	arg = va_arg(ap, ioctl_request_t);
	rc = real_ioctl(fd, request, arg);

	switch (request) {
		case PIO_FONT:
			dprintf(outfd, "ioctl(%d, %s, char *arg) = %d;\n", fd,
				translate_ntoa(kd_request, request, buf0, sizeof(buf0)),
				rc);
			dprintf(outfd, "char *arg = {\n");
			print_chars("\t", (unsigned char *) arg, (8 + 7) / 8 * 32 * 256, 8);
			dprintf(outfd, "};\n");
			break;
		case PIO_FONTX:
			dprintf(outfd, "ioctl(%d, %s, struct consolefontdesc *arg) = %d;\n", fd,
				translate_ntoa(kd_request, request, buf0, sizeof(buf0)),
				rc);
			print_consolefontdesc((struct consolefontdesc *) arg);
			break;
		case PIO_CMAP:
			dprintf(outfd, "ioctl(%d, %s, unsigned char arg[48]) = %d;\n", fd,
				translate_ntoa(kd_request, request, buf0, sizeof(buf0)),
				rc);
			dprintf(outfd, "unsigned char arg[48] = {\n");
			print_chars("\t", (unsigned char *) arg, 48, 16);
			dprintf(outfd, "\n");
			dprintf(outfd, "};\n");
			break;
		case KDGKBTYPE:
			arg_char = *((char *) arg);
			dprintf(outfd, "ioctl(%d, %s, 0x%04x) = %d;\n", fd,
				translate_ntoa(kd_request, request, buf0, sizeof(buf0)),
				(int) arg_char,
				rc);
			dprintf(outfd, "\t(descriptor %d is%sconsole)\n", fd,
				((arg_char == KB_101) || (arg_char == KB_84))
					? " "
					: " not ");
			break;
		case KDGETLED:
		case KDSETLED:
		case KDGKBLED:
		case KDSKBLED:
			arg_char = (request != KDSKBLED)
				? *((char *) arg)
				: (char) arg;
			dprintf(outfd, "ioctl(%d, %s, 0x%04x) = %d;\n", fd,
				translate_ntoa(kd_request, request, buf0, sizeof(buf0)),
				(int) arg_char,
				rc);
			dprintf(outfd, "\tdef: LED_NUM:%d LED_CAP:%d LED_SCR:%d\n",
				(((arg_char >> 4) & 7) & LED_NUM) != 0,
				(((arg_char >> 4) & 7) & LED_CAP) != 0,
				(((arg_char >> 4) & 7) & LED_SCR) != 0);
			dprintf(outfd, "\tval: LED_NUM:%d LED_CAP:%d LED_SCR:%d\n",
				((arg_char & 7) & LED_NUM) != 0,
				((arg_char & 7) & LED_CAP) != 0,
				((arg_char & 7) & LED_SCR) != 0);
			break;
		case KDGETMODE:
		case KDSETMODE:
			arg_i = *((unsigned int *) arg);
			dprintf(outfd, "ioctl(%d, %s, %s) = %d;\n", fd,
				translate_ntoa(kd_request, request, buf0, sizeof(buf0)),
				translate_ntoa(kd_mode, arg_i, buf1, sizeof(buf1)),
				rc);
			break;
		case GIO_UNIMAP:
		case PIO_UNIMAP:
			dprintf(outfd, "ioctl(%d, %s, struct unimapdesc *arg) = %d;\n", fd,
				translate_ntoa(kd_request, request, buf0, sizeof(buf0)),
				rc);
			print_unimapdesc((struct unimapdesc *) arg);
			break;
		case PIO_UNIMAPCLR:
			dprintf(outfd, "ioctl(%d, %s, struct unimapinit *arg) = %d;\n", fd,
				translate_ntoa(kd_request, request, buf0, sizeof(buf0)),
				rc);
			print_unimapinit((struct unimapinit *) arg);
			break;
		case KDFONTOP:
			dprintf(outfd, "ioctl(%d, %s, struct console_font_op *arg) = %d;\n", fd,
				translate_ntoa(kd_request, request, buf0, sizeof(buf0)),
				rc);
			print_console_font_op((struct console_font_op *) arg);
			break;
		case KDGKBMODE:
		case KDSKBMODE:
			arg_i = (request != KDSKBMODE)
				? *((unsigned int *) arg)
				: (unsigned int) arg;
			dprintf(outfd, "ioctl(%d, %s, %s) = %d;\n", fd,
				translate_ntoa(kd_request, request, buf0, sizeof(buf0)),
				translate_ntoa(kb_mode, arg_i, buf1, sizeof(buf1)),
				rc);
			break;
		case KDGKBMETA:
			arg_i = *((unsigned int *) arg);
			dprintf(outfd, "ioctl(%d, %s, %s) = %d;\n", fd,
				translate_ntoa(kd_request, request, buf0, sizeof(buf0)),
				translate_ntoa(kb_meta, arg_i, buf1, sizeof(buf1)),
				rc);
			break;
		case KDSKBMETA:
			dprintf(outfd, "ioctl(%d, %s, %s) = %d;\n", fd,
				translate_ntoa(kd_request, request, buf0, sizeof(buf0)),
				translate_ntoa(kb_meta, arg, buf1, sizeof(buf1)),
				rc);
			break;
		case KDGKBENT:
		case KDSKBENT:
			dprintf(outfd, "ioctl(%d, %s, struct kbentry *arg) = %d;\n", fd,
				translate_ntoa(kd_request, request, buf0, sizeof(buf0)),
				rc);
			print_kbentry((struct kbentry *) arg);
			break;
		case KDGKBSENT:
		case KDSKBSENT:
			dprintf(outfd, "ioctl(%d, %s, struct kbsentry *arg) = %d;\n", fd,
				translate_ntoa(kd_request, request, buf0, sizeof(buf0)),
				rc);
			print_kbsentry((struct kbsentry *) arg);
			break;
		case KDGKBDIACR:
		case KDSKBDIACR:
			dprintf(outfd, "ioctl(%d, %s, struct kbdiacrs *arg) = %d;\n", fd,
				translate_ntoa(kd_request, request, buf0, sizeof(buf0)),
				rc);
			print_kbdiacrs((struct kbdiacrs *) arg);
			break;
		case KDGKBDIACRUC:
		case KDSKBDIACRUC:
			dprintf(outfd, "ioctl(%d, %s, struct kbdiacrsuc *arg) = %d;\n", fd,
				translate_ntoa(kd_request, request, buf0, sizeof(buf0)),
				rc);
			print_kbdiacrsuc((struct kbdiacrsuc *) arg);
			break;
		default:
			dprintf(outfd, "ioctl(%d, %s, ...) = %d;\n", fd,
				translate_ntoa(kd_request, request, buf0, sizeof(buf0)),
				rc);
			break;
	}

	va_end(ap);
	return rc;
}
