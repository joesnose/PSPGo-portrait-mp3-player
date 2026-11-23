/* SPDX-License-Identifier: BSD-2-Clause */
#include "session.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
int session_load(MusicSession *s,const char *path){
 memset(s,0,sizeof(*s));char backup[PATH_CAP+8];FILE *f=fopen(path,"rb");if(!f&&snprintf(backup,sizeof(backup),"%s.bak",path)<(int)sizeof(backup))f=fopen(backup,"rb");if(!f)return 0;
 char line[PATH_CAP+32];int first=1;while(fgets(line,sizeof(line),f)){line[strcspn(line,"\r\n")]=0;if(first){snprintf(s->selected,sizeof(s->selected),"%.*s",PATH_CAP-1,line);first=0;}
 else if(!strncmp(line,"playing=",8))snprintf(s->playing,sizeof(s->playing),"%.*s",PATH_CAP-1,line+8);
 else if(!strncmp(line,"seconds=",8)){char *end;unsigned long n=strtoul(line+8,&end,10);if(*end==0&&n<=2678400ul)s->seconds=(unsigned)n;}
 else if(!strcmp(line,"paused=1"))s->paused=1;}
 int ok=!ferror(f);fclose(f);return ok;
}
int session_save(const MusicSession *s,const char *path){
 char temp[PATH_CAP+8],backup[PATH_CAP+8];if(snprintf(temp,sizeof(temp),"%s.tmp",path)>=(int)sizeof(temp)||snprintf(backup,sizeof(backup),"%s.bak",path)>=(int)sizeof(backup))return 0;
 FILE *f=fopen(temp,"wb");if(!f)return 0;int ok=fprintf(f,"%s\nplaying=%s\nseconds=%u\npaused=%d\n",s->selected,s->playing,s->seconds,s->paused)>0;if(fflush(f))ok=0;if(fclose(f))ok=0;if(!ok){remove(temp);return 0;}
 if(!rename(temp,path))return 1;remove(backup);int moved=!rename(path,backup);if(!rename(temp,path)){if(moved)remove(backup);return 1;}if(moved)rename(backup,path);remove(temp);return 0;
}
