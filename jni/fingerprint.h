#ifndef FINGERPRINT_H
#define FINGERPRINT_H

#include "dejavu.h"

struct song *learn_song(const char *pcmfile_path);
Fingerprint **alloc_fpn_tab(int peaks_num);
int songcmp(struct song *s1, struct song *s2);

#endif //FINGERPRINT_H
