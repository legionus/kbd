#ifndef LOADKEYS_ACTIONS_H
#define LOADKEYS_ACTIONS_H

#define U(x) ((x) ^ 0xf000)

#ifdef KDSKBDIACRUC
typedef struct kbdiacruc accent_entry;
#else
typedef struct kbdiacr accent_entry;
#endif

extern void mktable(u_short *key_map[], char *func_table[]);
extern void bkeymap(u_short *key_map[]);

#endif /* LOADKEYS_ACTIONS_H */
