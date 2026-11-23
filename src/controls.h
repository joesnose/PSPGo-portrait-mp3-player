#ifndef MUSIC_CONTROLS_H
#define MUSIC_CONTROLS_H
#include "core.h"
enum {PRESS_NONE,PRESS_SHORT,PRESS_LONG};
typedef struct {uint32_t began;int down,fired,cancelled;} Press;
int press_event(Press *state,int down,uint32_t now,uint32_t threshold);
void press_cancel(Press *state);
/* Selection only: containing directories are album/folder groups. */
int library_browse(const Library *library,int current,int direction,int album);
/* Caller owns returned array of first-track indices, sorted by full folder path. */
int library_folders(const Library *library,int **indices,size_t *count);
int library_shuffle(size_t count,uint32_t *seed,int **out);
int double_tap(uint32_t *last,int *armed,uint32_t now,int event);
typedef struct {uint32_t changed;int armed;} BrowseIdle;
void browse_idle_touch(BrowseIdle *state,uint32_t now);
int browse_idle_due(BrowseIdle *state,uint32_t now);
#endif
