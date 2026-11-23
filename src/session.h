#ifndef MUSIC_SESSION_H
#define MUSIC_SESSION_H
#include "core.h"
typedef struct {char selected[PATH_CAP],playing[PATH_CAP];unsigned seconds;int paused;} MusicSession;
int session_load(MusicSession *s,const char *path);
int session_save(const MusicSession *s,const char *path);
#endif
