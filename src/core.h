#ifndef MUSIC_CORE_H
#define MUSIC_CORE_H
#include <stddef.h>
#include <stdint.h>
#define PATH_CAP 1024
#define ROOT_CAP 16
#define TRACK_CAP 20000
#define ART_CAP (4u*1024u*1024u)
typedef struct {char roots[ROOT_CAP][PATH_CAP]; int count,defaults_checked;} Roots;
typedef struct {char **paths; size_t count,capacity; unsigned skipped;} Library;
typedef struct {char title[128],artist[128],album[128]; unsigned char *art; size_t art_size; long audio_start;} Tags;
typedef struct {uint32_t began; int down,fired;} Hold;
int hold_event(Hold *h,int down,uint32_t now,uint32_t threshold);
int path_join(char out[PATH_CAP],const char *dir,const char *name);
int root_add(Roots *roots,const char *path);
int roots_discover_standard(Roots *roots);
int roots_load(Roots *roots,const char *path);
int roots_save(const Roots *roots,const char *path);
int library_scan(Library *lib,const Roots *roots);
void library_free(Library *lib);
int is_mp3(const char *name);
void tags_read(const char *path,Tags *tags);
int tags_read_interruptible(const char *path,Tags *tags,int (*keep_going)(void));
void tags_free(Tags *tags);
const char *path_name(const char *path);
#endif
