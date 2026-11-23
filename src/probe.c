/* SPDX-License-Identifier: BSD-2-Clause
 * Read-only exact-path check. Directory enumeration never gates direct open.
 */
#include <pspiofilemgr.h>
#include <stdio.h>
#include <string.h>
#include "probe.h"
#include "fs.h"
FileProbe file_probes[2];
static char requested[PATH_CAP];
int probe_load(const char *config){
 requested[0]=0;FILE *f=fopen(config,"rb");if(!f)return 0;char line[PATH_CAP];int loaded=0;
 while(fgets(line,sizeof(line),f)){
  if(!strchr(line,'\n')&&!feof(f))break;line[strcspn(line,"\r\n")]=0;if(!line[0]||line[0]=='#')continue;
  /* Also accept the shorthand ms0/MUSIC/... supplied in conversation. */
  if((!strncmp(line,"ms0/",4)||!strncmp(line,"ef0/",4))&&strlen(line)+1<sizeof(requested)){size_t n=strlen(line);memcpy(requested,line,3);requested[3]=':';memcpy(requested+4,line+3,n-2);}
  else if(!strncmp(line,"ms0:/",5)||!strncmp(line,"ef0:/",5))snprintf(requested,sizeof(requested),"%s",line);
  loaded=requested[0]!=0;break;
 }fclose(f);return loaded;
}
static void probe_one(FileProbe *p,FILE *log){
 p->size=-1;p->listed=-1;p->dir_rc=-1;p->read_rc=-1;p->parent_entries=0;
 SceIoStat stat;memset(&stat,0,sizeof(stat));p->stat_rc=sceIoGetstat(p->path,&stat);if(p->stat_rc>=0)p->size=stat.st_size;
 p->open_rc=sceIoOpen(p->path,PSP_O_RDONLY,0);
 unsigned char bytes[16]={0};
 if(p->open_rc>=0){p->read_rc=sceIoRead(p->open_rc,bytes,sizeof(bytes));int close_rc=sceIoClose(p->open_rc);if(log)fprintf(log,"DIRECT_CLOSE path=%s rc=%08x\n",p->path,close_rc);}
 if(log){fprintf(log,"DIRECT_FILE path=%s stat=%08x size=%lld open=%08x read=%d header=",p->path,p->stat_rc,p->size,p->open_rc,p->read_rc);for(int i=0;i<p->read_rc&&i<16;i++)fprintf(log,"%02x",bytes[i]);fputc('\n',log);fflush(log);}
 char parent[PATH_CAP];snprintf(parent,sizeof(parent),"%s",p->path);char *slash=strrchr(parent,'/');if(!slash)return;*slash=0;
 MusicDir *dir=music_dir_open(parent);if(dir){MusicEntry e;p->listed=0;int rc=0;
  while(p->parent_entries<20000&&(rc=music_dir_read(dir,&e))>0){if(!strcmp(e.name,".")||!strcmp(e.name,".."))continue;p->parent_entries++;if(!music_path_compare(e.name,path_name(p->path)))p->listed=1;}
  p->dir_rc=rc;if(p->parent_entries>=20000&&p->listed==0)p->listed=-1;music_dir_close(dir);
 }
 if(log){fprintf(log,"DIRECT_PARENT path=%s listed=%d entries=%u read=%08x\n",parent,p->listed,p->parent_entries,p->dir_rc);fflush(log);}
}
void probe_run(FILE *log){
 memset(file_probes,0,sizeof(file_probes));if(!requested[0])return;
 snprintf(file_probes[0].path,PATH_CAP,"%s",requested);snprintf(file_probes[1].path,PATH_CAP,"%s",requested);
 memcpy(file_probes[1].path,!strncmp(requested,"ms0:",4)?"ef0:":"ms0:",4);
 if(log){fprintf(log,"DIRECT_BEGIN configured=%s\n",requested);fflush(log);}
 probe_one(&file_probes[0],log);probe_one(&file_probes[1],log);
 if(log){fprintf(log,"DIRECT_END playable=%s\n",probe_playable()?probe_playable():"none");fflush(log);}
}
const char *probe_playable(void){for(int i=0;i<2;i++)if(file_probes[i].path[0]&&file_probes[i].open_rc>=0&&file_probes[i].read_rc>0)return file_probes[i].path;return 0;}
