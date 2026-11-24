#include "thumb_store.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>
static void file(const char *path,const char *data){FILE *f=fopen(path,"wb");assert(f);assert(fwrite(data,1,strlen(data),f)==strlen(data));assert(!fclose(f));}
int main(void){
 char root[]="/tmp/portrait-thumbs-XXXXXX";assert(mkdtemp(root));char cache[PATH_CAP],folder[PATH_CAP],track[PATH_CAP],source[PATH_CAP];assert(path_join(cache,root,"cache"));assert(!mkdir(cache,0700));assert(path_join(folder,root,"album"));assert(!mkdir(folder,0700));assert(path_join(track,folder,"song.mp3"));assert(path_join(source,folder,"cover.jpg"));file(track,"mp3");file(source,"image");
 uint16_t pixels[THUMB_PIXELS],read[THUMB_PIXELS];for(unsigned i=0;i<THUMB_PIXELS;i++)pixels[i]=(uint16_t)i;
 assert(!thumb_store_load(cache,folder,track,1,read));assert(thumb_store_save(cache,folder,track,source,1,pixels));assert(thumb_store_load(cache,folder,track,1,read));assert(!memcmp(pixels,read,sizeof(pixels)));assert(!thumb_store_load(cache,folder,track,2,read));
 file(source,"new bigger image");assert(!thumb_store_load(cache,folder,track,1,read));assert(thumb_store_save(cache,folder,track,source,1,pixels));file(track,"changed mp3");assert(!thumb_store_load(cache,folder,track,1,read));assert(thumb_store_save(cache,folder,track,source,1,pixels));
 DIR *d=opendir(cache);assert(d);struct dirent *e;char entry[PATH_CAP]="";while((e=readdir(d)))if(strstr(e->d_name,".thumb")){assert(path_join(entry,cache,e->d_name));break;}closedir(d);assert(entry[0]);FILE *f=fopen(entry,"r+b");assert(f);assert(!fseek(f,-1,SEEK_END));int byte=fgetc(f);assert(!fseek(f,-1,SEEK_END));fputc(byte^1,f);fclose(f);assert(!thumb_store_load(cache,folder,track,1,read));file(entry,"truncated");assert(!thumb_store_load(cache,folder,track,1,read));
 assert(thumb_unpack(thumb_pack(0xff0000ffu))==0xff0000ffu);assert(thumb_unpack(thumb_pack(0xff00ff00u))==0xff00ff00u);assert(thumb_unpack(thumb_pack(0xffff0000u))==0xffff0000u);
 remove(entry);remove(source);remove(track);rmdir(folder);rmdir(cache);rmdir(root);
 puts("PASS: persistent pixel round-trip, colour conversion, source/track/theme invalidation and corrupt/truncated cache recovery");
}
