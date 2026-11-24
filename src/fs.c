/* SPDX-License-Identifier: BSD-2-Clause */
#include "fs.h"
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#ifdef __PSP__
#include <pspiofilemgr.h>
#else
#include <dirent.h>
#include <sys/stat.h>
#endif
struct MusicDir {
#ifdef __PSP__
 SceUID fd;
#else
 DIR *fd;
#endif
 char path[PATH_CAP];unsigned entries,folders,mp3,other_audio,other,logged;int ended,read_started,quiet;
};
static FILE *diagnostic;static unsigned lines;
void music_fs_log(FILE *file){diagnostic=file;lines=0;}
int music_path_compare(const char *a,const char *b){
#ifdef __PSP__
 return strcasecmp(a,b);
#else
 return strcmp(a,b);
#endif
}
int music_path_prefix(const char *a,const char *b,size_t n){
#ifdef __PSP__
 return strncasecmp(a,b,n);
#else
 return strncmp(a,b,n);
#endif
}
int music_native_directory(unsigned mode,unsigned attr){return (mode&0xf000u)==0x1000u || !!(attr&0x10u);}
static MusicDir *dir_open(const char *path,int quiet){
 MusicDir *d=calloc(1,sizeof(*d));if(!d)return 0;d->quiet=quiet;snprintf(d->path,sizeof(d->path),"%s",path);
#ifdef __PSP__
 d->fd=sceIoDopen(path);if(d->fd<0){if(!d->quiet&&diagnostic){fprintf(diagnostic,"DIR_OPEN error=%08x path=%s\n",d->fd,path);fflush(diagnostic);}free(d);return 0;}
#else
 d->fd=opendir(path);if(!d->fd){free(d);return 0;}
#endif
 if(!d->quiet&&diagnostic)fprintf(diagnostic,"DIR_BEGIN path=%s\n",path);return d;
}
MusicDir *music_dir_open(const char *path){return dir_open(path,0);}
MusicDir *music_dir_open_quiet(const char *path){return dir_open(path,1);}
int music_other_audio(const char *name){const char *ext=strrchr(name,'.');if(!ext)return 0;const char *types[]={".wma",".m4a",".m4b",".oma",".aa3",".flac",".wav",".ogg",".aac",".mp4"};for(unsigned i=0;i<sizeof(types)/sizeof(*types);i++)if(!strcasecmp(ext,types[i]))return 1;return 0;}
int music_dir_read(MusicDir *d,MusicEntry *e){
 memset(e,0,sizeof(*e));d->read_started=1;unsigned mode=0,attr=0;
#ifdef __PSP__
 SceIoDirent native;memset(&native,0,sizeof(native));int rc=sceIoDread(d->fd,&native);if(rc<=0){d->ended=rc;if(rc<0&&!d->quiet&&diagnostic)fprintf(diagnostic,"DIR_READ error=%08x path=%s\n",rc,d->path);return rc;}
 snprintf(e->name,sizeof(e->name),"%s",native.d_name);mode=native.d_stat.st_mode;attr=native.d_stat.st_attr;
 e->directory=music_native_directory(mode,attr);e->link=FIO_S_ISLNK(mode);e->regular=!e->directory&&!e->link;
#else
 struct dirent *entry=readdir(d->fd);if(!entry){d->ended=0;return 0;}snprintf(e->name,sizeof(e->name),"%s",entry->d_name);
 char p[PATH_CAP];struct stat st;if(!path_join(p,d->path,e->name)||lstat(p,&st))return 1;
 e->directory=S_ISDIR(st.st_mode);e->regular=S_ISREG(st.st_mode);e->link=S_ISLNK(st.st_mode);mode=(unsigned)st.st_mode;
#endif
 if(strcmp(e->name,".")&&strcmp(e->name,"..")){
  d->entries++;if(e->directory)d->folders++;else if(is_mp3(e->name))d->mp3++;else if(music_other_audio(e->name))d->other_audio++;else d->other++;
  if(!d->quiet&&diagnostic&&d->logged++<48&&lines++<2048)fprintf(diagnostic,"DIR_ENTRY path=%s name=%s mode=%08x attr=%08x kind=%s\n",d->path,e->name,mode,attr,e->directory?"directory":is_mp3(e->name)?"mp3":music_other_audio(e->name)?"other-audio":"file");
 }
 return 1;
}
void music_dir_close(MusicDir *d){if(!d)return;
 if(!d->quiet&&diagnostic&&d->read_started){fprintf(diagnostic,"DIR_SUMMARY path=%s entries=%u folders=%u mp3=%u other_audio=%u other=%u read=%08x\n",d->path,d->entries,d->folders,d->mp3,d->other_audio,d->other,d->ended);fflush(diagnostic);}
#ifdef __PSP__
 sceIoDclose(d->fd);
#else
 closedir(d->fd);
#endif
 free(d);
}
int music_is_dir(const char *path){MusicDir *d=music_dir_open(path);if(!d)return 0;music_dir_close(d);return 1;}
