#ifndef MUSIC_FS_H
#define MUSIC_FS_H
#include <stdio.h>
#include "core.h"
typedef struct MusicDir MusicDir;
typedef struct {char name[PATH_CAP];int directory,regular,link;} MusicEntry;
MusicDir *music_dir_open(const char *path);
int music_dir_read(MusicDir *dir,MusicEntry *entry);
void music_dir_close(MusicDir *dir);
int music_path_compare(const char *a,const char *b);
int music_path_prefix(const char *a,const char *b,size_t n);
int music_is_dir(const char *path);
void music_fs_log(FILE *file);
/* PSP native metadata classification; attributes can carry the type even
 * when a storage driver leaves st_mode empty. Shared with the regression test. */
int music_other_audio(const char *name);
int music_native_directory(unsigned mode,unsigned attr);
#endif
