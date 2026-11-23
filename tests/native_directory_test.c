/* Run the actual PSP directory adapter against native attribute fixtures.
 * No POSIX stat/lstat are provided by this mock backend. */
#include "core.h"
#include "fs.h"
#include <pspiofilemgr.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>
typedef struct {const char *parent,*name;unsigned mode,attr;} Entry;
static Entry entries[]={
 {"ef0:/MUSIC","Albums",0,0x10},
 {"ef0:/MUSIC","Hidden album",0,0x12},
 {"ef0:/MUSIC","Read-only album",0,0x11},
 {"ef0:/MUSIC","Archive album",0,0x30},
 {"ef0:/MUSIC","root song.MP3",0,0x21},
 {"ef0:/MUSIC","another.mp3",0x2000,0},
 {"ef0:/MUSIC","cover.jpg",0,0x20},
 {"ef0:/MUSIC/Albums","Disc 1",0x1000,0},
 {"ef0:/MUSIC/Albums/Disc 1","nested.mp3",0,0},
 {"ef0:/MUSIC/Hidden album","hidden.mp3",0,0x22},
 {"ef0:/MUSIC/Read-only album","readonly.mp3",0,0x21},
 {"ef0:/MUSIC/Archive album","archive.mp3",0,0x20}};
static struct {const char *path;unsigned next;int used;} handles[32];
int sceIoDopen(const char *p){for(int h=0;h<32;h++)if(handles[h].used)return -77;const char *stable=0;for(unsigned i=0;i<sizeof(entries)/sizeof(*entries);i++)if(!strcmp(p,entries[i].parent))stable=entries[i].parent;if(!stable)return -1;for(int i=0;i<32;i++)if(!handles[i].used){handles[i].used=1;handles[i].path=stable;handles[i].next=0;return i;}return -1;}
int sceIoDread(int fd,SceIoDirent *out){assert(fd>=0&&fd<32&&handles[fd].used);while(handles[fd].next<sizeof(entries)/sizeof(*entries)){Entry *e=&entries[handles[fd].next++];if(strcmp(e->parent,handles[fd].path))continue;strcpy(out->d_name,e->name);out->d_stat.st_mode=e->mode;out->d_stat.st_attr=e->attr;return 1;}return 0;}
int sceIoDclose(int fd){handles[fd].used=0;return 0;}
int main(void){Roots roots={0};Library lib={0};assert(root_add(&roots,"ef0:/MUSIC")==0);assert(root_add(&roots,"ef0:/music")==0);assert(roots.count==1);assert(library_scan(&lib,&roots));assert(lib.count==6);assert(lib.skipped==0);int direct=0;for(unsigned i=0;i<lib.count;i++){if(!strcmp(lib.paths[i],"ef0:/MUSIC/root song.MP3"))direct=1;}assert(direct);assert(music_is_dir("ef0:/MUSIC/Hidden album"));assert(!music_is_dir("ef0:/missing"));library_free(&lib);puts("PASS: PSP native Dread: attribute-only, hidden, readonly, archive directories, nested and root-level MP3s; no lstat dependency; one native directory handle at a time");}
