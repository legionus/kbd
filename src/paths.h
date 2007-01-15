/*
 * All data is in subdirectories of DATADIR, by default /usr/lib/kbd
 * The following four subdirectories are defined:
 */
#define KEYMAPDIR "keymaps"
#define OLDKEYMAPDIR "keytables"
#define FONTDIR "consolefonts"
#define TRANSDIR "consoletrans"
#define VIDEOMODEDIR "videomodes"

/*
 * Default keymap, and where the kernel copy of it lives.
 */
#ifdef sparc
# define DEFMAP "sunkeymap.map"
# define KERNDIR "/usr/src/linux/drivers/sbus/char"
#else
# define DEFMAP "defkeymap.map"
# define KERNDIR "/usr/src/linux/drivers/char"
#endif

extern FILE *findfile(char *fnam, char **dirpath, char **suffixes);
extern char pathname[];
extern void fpclose(FILE *fp);

extern int verbose;
