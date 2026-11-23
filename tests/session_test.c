#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
static int fat_rename(const char*,const char*);
#define rename fat_rename
#include "../src/session.c"
#undef rename
static int fat_rename(const char *a,const char *b){if(!access(b,F_OK)){errno=EEXIST;return -1;}return rename(a,b);}
int main(void){char dir[]="/tmp/joe-session-XXXXXX",path[120],bak[128];assert(mkdtemp(dir));snprintf(path,sizeof(path),"%s/position.txt",dir);MusicSession a={0},b;assert(!session_load(&b,path));strcpy(a.selected,"ms0:/MUSIC/Other/01.mp3");strcpy(a.playing,"ms0:/MUSIC/Album/02.mp3");a.seconds=123;a.paused=1;
 for(int i=0;i<3;i++){a.seconds++;assert(session_save(&a,path));assert(session_load(&b,path));assert(!strcmp(a.selected,b.selected)&&!strcmp(a.playing,b.playing)&&a.seconds==b.seconds&&b.paused==1);}
 snprintf(bak,sizeof(bak),"%s.bak",path);assert(!rename(path,bak));assert(session_load(&b,path)&&b.seconds==a.seconds);unlink(bak);
 FILE *f=fopen(path,"wb");assert(f);fputs("ms0:/MUSIC/legacy.mp3\n",f);fclose(f);assert(session_load(&b,path)&&!strcmp(b.selected,"ms0:/MUSIC/legacy.mp3")&&!b.playing[0]&&!b.seconds);unlink(path);rmdir(dir);puts("PASS: separate browsing/playback state, timestamp and pause round-trip, FAT replacement, backup recovery, legacy position format");}
