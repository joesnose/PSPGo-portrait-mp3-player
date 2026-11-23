#ifndef MUSIC_PROBE_H
#define MUSIC_PROBE_H
#include "core.h"
#include <stdio.h>
typedef struct {
 char path[PATH_CAP];int stat_rc,open_rc,read_rc,listed,dir_rc;long long size;unsigned parent_entries;
} FileProbe;
extern FileProbe file_probes[2];
int probe_load(const char *config);
void probe_run(FILE *log);
const char *probe_playable(void);
#endif
