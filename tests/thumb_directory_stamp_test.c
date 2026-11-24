#include <sys/stat.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <unistd.h>
#include <dirent.h>
static char unavailable_directory[1024];
static int cache_stat(const char *path,struct stat *out){if(!strcmp(path,unavailable_directory))return -1;return stat(path,out);}
#define stat(path,out) cache_stat(path,out)
#include "../src/thumb_store.c"
#undef stat
static void make_file(const char *path,const char *s){FILE *f=fopen(path,"wb");assert(f);fputs(s,f);fclose(f);}
int main(void){char root[]="/tmp/portrait-dir-stamp-XXXXXX",cache[PATH_CAP],folder[PATH_CAP],track[PATH_CAP],source[PATH_CAP];assert(mkdtemp(root));assert(path_join(cache,root,"cache")&&!mkdir(cache,0700));assert(path_join(folder,root,"album")&&!mkdir(folder,0700));assert(path_join(track,folder,"track.mp3")&&path_join(source,folder,"cover.jpg"));make_file(track,"track");make_file(source,"cover");strcpy(unavailable_directory,folder);
 uint16_t pixels[THUMB_PIXELS]={1},read[THUMB_PIXELS];assert(thumb_store_save(cache,folder,track,source,0,pixels));assert(thumb_store_load(cache,folder,track,0,read));assert(!memcmp(pixels,read,sizeof(pixels)));
 DIR *legacy=opendir(cache);assert(legacy);struct dirent *item;while((item=readdir(legacy)))if(strstr(item->d_name,".thumb")){char name[PATH_CAP];assert(path_join(name,cache,item->d_name));FILE *f=fopen(name,"r+b");assert(f);assert(!fseek(f,3084,SEEK_SET));for(int i=0;i<16;i++)fputc(0x77,f);fclose(f);break;}closedir(legacy);assert(thumb_store_load(cache,folder,track,0,read));
 make_file(source,"modified cover");assert(!thumb_store_load(cache,folder,track,0,read));
 DIR *d=opendir(cache);assert(d);struct dirent *e;while((e=readdir(d)))if(strstr(e->d_name,".thumb")){char file[PATH_CAP];assert(path_join(file,cache,e->d_name));remove(file);}closedir(d);remove(source);remove(track);rmdir(folder);rmdir(cache);rmdir(root);puts("PASS: directory-stat absence or legacy-directory-stamp changes do not invalidate unchanged art; actual image fingerprint remains required");return 0;}
