#ifndef _MAPSCRN_H
#define _MAPSCRN_H

int kfont_saveoldmap(int fd, char *omfil);
int kfont_loadnewmap(int fd, char *mfil, const char *const *mapdirpath, const char *const *mapsuffixes);

#endif // _MAPSCRN_H
