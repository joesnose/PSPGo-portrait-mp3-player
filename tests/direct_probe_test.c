#include "probe.h"
#include "fs.h"
#include <pspiofilemgr.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
static int present=1,stat_failure,opened;
static const char *exact="ms0:/MUSIC/(1985) Little Creatures/09. Road to Nowhere.mp3";
int sceIoGetstat(const char *p,SceIoStat *s){if(!present||stat_failure||strcasecmp(p,exact))return (int)0x80010002;s->st_size=4000000;return 0;}
int sceIoOpen(const char *p,int flags,int mode){assert(flags==PSP_O_RDONLY);assert(mode==0);if(!present||strcasecmp(p,exact))return (int)0x80010002;assert(!opened);opened=1;return 42;}
int sceIoRead(int fd,void *data,unsigned n){assert(opened&&fd==42&&n==16);memset(data,0,n);memcpy(data,"ID3",3);return n;}
int sceIoClose(int fd){assert(opened&&fd==42);opened=0;return 0;}
int sceIoDopen(const char *p){return !strcmp(p,"ms0:/MUSIC/(1985) Little Creatures")?77:-1;}
int sceIoDread(int fd,SceIoDirent *e){(void)e;assert(fd==77);return 0;}
int sceIoDclose(int fd){assert(fd==77);return 0;}
int main(void){
 char cfg[]="/tmp/joemusic-probe-XXXXXX";int fd=mkstemp(cfg);assert(fd>=0);FILE *f=fdopen(fd,"w");assert(f);fputs("# file probe\nms0/MUSIC/(1985) Little Creatures/09. Road to Nowhere.mp3\n",f);assert(!fclose(f));
 assert(probe_load(cfg));FILE *log=tmpfile();assert(log);probe_run(log);
 assert(!strcmp(file_probes[0].path,exact));assert(file_probes[0].listed==0);assert(file_probes[0].read_rc==16);assert(!strcmp(probe_playable(),exact));assert(file_probes[1].open_rc<0);assert(!opened);
 stat_failure=1;probe_run(log);assert(file_probes[0].stat_rc<0&&probe_playable());
 present=0;probe_run(log);assert(!probe_playable()&&file_probes[0].open_rc<0&&file_probes[1].open_rc<0);fclose(log);unlink(cfg);
 puts("PASS: read-only direct open succeeds independently of missing directory entries or failed stat; shorthand/space/parenthesis path preserved; missing-file retest clears stale success");
}
