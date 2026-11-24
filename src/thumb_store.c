/* SPDX-License-Identifier: BSD-2-Clause */
#include "thumb_store.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
static uint64_t hash(const unsigned char *p,size_t n){uint64_t h=UINT64_C(14695981039346656037);for(size_t i=0;i<n;i++){h^=p[i];h*=UINT64_C(1099511628211);}return h;}
static int filename(char out[PATH_CAP],const char *dir,const char *folder){char name[40];uint64_t h=hash((const unsigned char*)folder,strlen(folder));snprintf(name,sizeof(name),"%08x%08x.thumb",(unsigned)(h>>32),(unsigned)h);return dir[0]&&path_join(out,dir,name);}
static int put32(FILE *f,uint32_t v){unsigned char b[4]={(unsigned char)v,(unsigned char)(v>>8),(unsigned char)(v>>16),(unsigned char)(v>>24)};return fwrite(b,1,4,f)==4;}
static int put64(FILE *f,uint64_t v){return put32(f,(uint32_t)v)&&put32(f,(uint32_t)(v>>32));}
static int stamp(const char *path,uint64_t *size,uint64_t *time){struct stat st;if(stat(path,&st))return 0;*size=(uint64_t)st.st_size;*time=(uint64_t)st.st_mtime;return 1;}
static int putpath(FILE *f,const char *path){char s[PATH_CAP]={0};if(strlen(path)>=sizeof(s))return 0;strcpy(s,path);return fwrite(s,1,sizeof(s),f)==sizeof(s);}
static int writestamp(FILE *f,const char *path){uint64_t size,time;return stamp(path,&size,&time)&&put64(f,size)&&put64(f,time);}
static uint32_t checksum(const uint16_t *p){uint32_t h=2166136261u;for(unsigned i=0;i<THUMB_PIXELS;i++){h=(h^(p[i]&255u))*16777619u;h=(h^(p[i]>>8))*16777619u;}return h;}
static uint32_t le32(const unsigned char *b){return (uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24;}
static uint64_t le64(const unsigned char *b){return (uint64_t)le32(b+4)<<32|le32(b);}
static int same_stamp(const unsigned char *b,const char *path){uint64_t size,time;return stamp(path,&size,&time)&&size==le64(b)&&time==le64(b+8);}
int thumb_store_load(const char *dir,const char *folder,const char *track,uint32_t background,uint16_t pixels[THUMB_PIXELS]){
 char file[PATH_CAP];unsigned char bytes[3136+THUMB_PIXELS*2];if(!filename(file,dir,folder))return 0;FILE *f=fopen(file,"rb");if(!f)return 0;
 int ok=fread(bytes,1,sizeof(bytes),f)==sizeof(bytes)&&fgetc(f)==EOF;fclose(f);if(!ok)return 0;
 const char *saved_folder=(const char*)bytes+12,*saved_track=(const char*)bytes+1036,*source=(const char*)bytes+2060;
 if(memcmp(bytes,"JPTN018",8)||le32(bytes+8)!=background||!memchr(saved_folder,0,PATH_CAP)||!memchr(saved_track,0,PATH_CAP)||!memchr(source,0,PATH_CAP)||strcmp(saved_folder,folder)||strcmp(saved_track,track)||!same_stamp(bytes+3084,folder)||!same_stamp(bytes+3100,track)||!same_stamp(bytes+3116,source))return 0;
 for(unsigned i=0;i<THUMB_PIXELS;i++)pixels[i]=(uint16_t)(bytes[3136+i*2]|(unsigned)bytes[3137+i*2]<<8);
 return checksum(pixels)==le32(bytes+3132);
}
int thumb_store_save(const char *dir,const char *folder,const char *track,const char *source,uint32_t background,const uint16_t pixels[THUMB_PIXELS]){
 char file[PATH_CAP],tmp[PATH_CAP];if(!source[0]||!filename(file,dir,folder)||snprintf(tmp,sizeof(tmp),"%s.tmp",file)>=(int)sizeof(tmp))return 0;FILE *f=fopen(tmp,"wb");if(!f)return 0;
 int ok=fwrite("JPTN018",1,8,f)==8&&put32(f,background)&&putpath(f,folder)&&putpath(f,track)&&putpath(f,source)&&writestamp(f,folder)&&writestamp(f,track)&&writestamp(f,source)&&put32(f,checksum(pixels));
 if(ok){unsigned char bytes[THUMB_PIXELS*2];for(unsigned i=0;i<THUMB_PIXELS;i++){bytes[i*2]=(unsigned char)pixels[i];bytes[i*2+1]=(unsigned char)(pixels[i]>>8);}ok=fwrite(bytes,1,sizeof(bytes),f)==sizeof(bytes);}
 if(fclose(f))ok=0;if(!ok){remove(tmp);return 0;}if(!rename(tmp,file))return 1;remove(file);if(!rename(tmp,file))return 1;remove(tmp);return 0;
}
