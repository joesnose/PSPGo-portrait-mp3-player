#ifndef PLAYER_H
#define PLAYER_H
#include "core.h"
enum {PLAYER_STOPPED,PLAYER_LOADING,PLAYER_PLAYING,PLAYER_PAUSED,PLAYER_ENDED,PLAYER_ERROR};
typedef struct {int state,rate,bitrate,error,generation; unsigned seconds; char path[PATH_CAP];} Playback;
int player_init(void);
void player_play(const char *path);
void player_pause(void);
void player_stop(void);
Playback player_snapshot(void);
void player_shutdown(void);
#endif
