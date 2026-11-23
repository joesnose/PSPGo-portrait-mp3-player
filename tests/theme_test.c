#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
static int fat_rename(const char *from,const char *to);
#define rename fat_rename
#include "../src/theme.c"
#undef rename
static int fat_rename(const char *from,const char *to){if(!access(to,F_OK)){errno=EEXIST;return -1;}return rename(from,to);}
int main(void){char dir[]="/tmp/joe-themes-XXXXXX",path[128];assert(mkdtemp(dir));snprintf(path,sizeof(path),"%s/theme.cfg",dir);
 theme_load(path);assert(music_theme==0);
 for(int i=0;i<THEME_COUNT;i++){music_theme=i;assert(theme_save(path));music_theme=0;theme_load(path);assert(music_theme==i);}
 FILE *f=fopen(path,"wb");assert(f);fputs("not a theme\n",f);fclose(f);theme_load(path);assert(music_theme==0);unlink(path);rmdir(dir);
 puts("PASS: default Sage, all five saved themes, FAT rename replacement and invalid-config fallback");}
