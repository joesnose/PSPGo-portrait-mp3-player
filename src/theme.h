#ifndef MUSIC_THEME_H
#define MUSIC_THEME_H
#include <stdint.h>
typedef struct {const char *name;uint32_t ink,muted,back,accent,panel,frame,secondary;} MusicTheme;
extern const MusicTheme music_themes[5];
extern int music_theme;
#define THEME_COUNT 5
#define THEME (music_themes[music_theme])
void theme_load(const char *path);
int theme_save(const char *path);
#endif
