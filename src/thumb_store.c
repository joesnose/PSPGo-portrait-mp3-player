/* SPDX-License-Identifier: BSD-2-Clause */
#include "thumb_store.h"
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
static uint64_t hash(const unsigned char *p,size_t n){uint64_t h=UINT64_C(14695981039346656037);for(size_t i=0;i<n;i++){h^=p[i];h*=UINT64_C(1099511628211);}return h;}
static int filename(char out[PATH_CAP],const char *dir,const char *folder){char name[40];uint64_t h=hash((const unsigned char*)folder,strlen(folder));snprintf(name,sizeof(name),"%08x%08x.thumb",(unsigned)(h>>32),(unsigned)h);return dir[0]&&path_join(out,dir,name);}
static int put32(FILE *f,uint32_t v){unsigned char b[4]={(unsigned char)v,(unsigned char)(v>>8),(unsigned char)(v>>16),(unsigned char)(v>>24)};return fwrite(b,1,4,f)==4;}
static int get32(FILE *f,uint32_t *v){unsigned char b[4];if(fread(b,1,4,f)!=4)return 0;*v=(uint32_t)b[0]|(uint32_t)b[1]<<8|(uint32_t)b[2]<<16|(uint32_t)b[3]<<24;return 1;}
static int put64(FILE *f,uint64_t v){return put32(f,(uint32_t)v)&&put32(f,(uint32_t)(v>>32));}
static int get64(FILE *f,uint64_t *v){uint32_t lo,hi;if(!get32(f,&lo)||!get32(f,&hi))return 0;*v=(uint64_t)hi<<32|lo;return 1;}
static int stamp(const char *path,uint64_t *size,uint64_t *time){struct stat st;if(stat(path,&st))return 0;*size=(uint64_t)st.st_size;*time=(uint64_t)st.st_mtime;return 1;}
static int putpath(FILE *f,const char *path){char s[PATH_CAP]={0};if(strlen(path)>=sizeof(s))return 0;strcpy(s,path);return fwrite(s,1,sizeof(s),f)==sizeof(s);}
static int getpath(FILE *f,char path[PATH_CAP]){return fread(path,1,PATH_CAP,f)==PATH_CAP&&memchr(path,0,PATH_CAP);}
static int checkstamp(FILE *f,const char *path){uint64_t a,b,size,time;return get64(f,&a)&&get64(f,&b)&&stamp(path,&size,&time)&&a==size&&b==time;}
static int writestamp(FILE *f,const char *path){uint64_t size,time;return stamp(path,&size,&time)&&put64(f,size)&&put64(f,time);}
static uint32_t checksum(const uint16_t *p){uint32_t h=2166136261u;for(unsigned i=0;i<THUMB_PIXELS;i++){h=(h^(p[i]&255u))*16777619u;h=(h^(p[i]>>8))*16777619u;}return h;}
int thumb_store_load(const char *dir,const char *folder,const char *track,uint32_t background,uint16_t pixels[THUMB_PIXELS]){
 char file[PATH_CAP],saved_folder[PATH_CAP],saved_track[PATH_CAP],source[PATH_CAP],magic[8];uint32_t bg,crc;if(!filename(file,dir,folder))return 0;FILE *f=fopen(file,"rb");if(!f)return 0;
 int ok=fread(magic,1,8,f)==8&&!memcmp(magic,"JPTN018",8)&&get32(f,&bg)&&bg==background&&getpath(f,saved_folder)&&!strcmp(saved_folder,folder)&&getpath(f,saved_track)&&!strcmp(saved_track,track)&&getpath(f,source)&&checkstamp(f,folder)&&checkstamp(f,track)&&checkstamp(f,source)&&get32(f,&crc);
 if(ok){unsigned char bytes[THUMB_PIXELS*2];ok=fread(bytes,1,sizeof(bytes),f)==sizeof(bytes);if(ok)for(unsigned i=0;i<THUMB_PIXELS;i++)pixels[i]=(uint16_t)(bytes[i*2]|(unsigned)bytes[i*2+1]<<8);}
 if(ok)ok=checksum(pixels)==crc&&fgetc(f)==EOF;fclose(f);return ok;
}
int thumb_store_save(const char *dir,const char *folder,const char *track,const char *source,uint32_t background,const uint16_t pixels[THUMB_PIXELS]){
 char file[PATH_CAP],tmp[PATH_CAP];if(!source[0]||!filename(file,dir,folder)||snprintf(tmp,sizeof(tmp),"%s.tmp",file)>=(int)sizeof(tmp))return 0;FILE *f=fopen(tmp,"wb");if(!f)return 0;
 int ok=fwrite("JPTN018",1,8,f)==8&&put32(f,background)&&putpath(f,folder)&&putpath(f,track)&&putpath(f,source)&&writestamp(f,folder)&&writestamp(f,track)&&writestamp(f,source)&&put32(f,checksum(pixels));
 if(ok){unsigned char bytes[THUMB_PIXELS*2];for(unsigned i=0;i<THUMB_PIXELS;i++){bytes[i*2]=(unsigned char)pixels[i];bytes[i*2+1]=(unsigned char)(pixels[i]>>8);}ok=fwrite(bytes,1,sizeof(bytes),f)==sizeof(bytes);}
 if(fclose(f))ok=0;if(!ok){remove(tmp);return 0;}if(!rename(tmp,file))return 1;remove(file);if(!rename(tmp,file))return 1;remove(tmp);return 0;
}
