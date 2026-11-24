#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>
#include <time.h>
#include <assert.h>
#include "gfx.h"
#include "core.h"
#include "fs.h"
#include "thumb_store.h"
typedef int SceUID;typedef unsigned SceSize;typedef unsigned SceUInt;
#define PSP_THREAD_ATTR_USER 0
static volatile unsigned thumb_visible,thumb_frame;static volatile uint32_t thumb_moved;
static uint32_t thumb_work[THUMB_PIXELS],test_pixel;static FILE *art_log;static char test_decode_path[PATH_CAP];static uint16_t test_decode_color;static unsigned test_decodes;
static unsigned sceKernelGetSystemTimeLow(void){struct timespec t;clock_gettime(CLOCK_MONOTONIC,&t);return (unsigned)((uint64_t)t.tv_sec*1000000u+t.tv_nsec/1000);}
static int sceKernelDelayThread(unsigned us){usleep(us);return 0;}
static sem_t test_semas[16];static int test_next_sema;
static int sceKernelCreateSema(const char *n,int a,int initial,int max,void *o){(void)n;(void)a;(void)max;(void)o;int id=test_next_sema++;assert(id<16);assert(!sem_init(&test_semas[id],0,initial));return id;}
static int sceKernelWaitSema(int id,int n,unsigned *timeout){assert(n==1);if(!timeout)return sem_wait(&test_semas[id]);struct timespec t;clock_gettime(CLOCK_REALTIME,&t);t.tv_nsec+=(long)*timeout*1000;t.tv_sec+=t.tv_nsec/1000000000;t.tv_nsec%=1000000000;return sem_timedwait(&test_semas[id],&t);}
static int sceKernelSignalSema(int id,int n){assert(n==1);return sem_post(&test_semas[id]);}
static int sceKernelDeleteSema(int id){return sem_destroy(&test_semas[id]);}
static int (*test_entry)(SceSize,void*);static pthread_t test_thread;
static void *test_run(void *p){(void)p;test_entry(0,0);return 0;}
static int sceKernelCreateThread(const char *n,int(*entry)(SceSize,void*),int priority,int stack,int attr,void *o){(void)n;(void)priority;(void)stack;(void)attr;(void)o;test_entry=entry;return 10;}
static int sceKernelStartThread(int id,int size,void *p){(void)id;(void)size;(void)p;return pthread_create(&test_thread,0,test_run,0);}
static int sceKernelWaitThreadEnd(int id,void *p){(void)id;(void)p;return pthread_join(test_thread,0);}
static int sceKernelDeleteThread(int id){(void)id;return 0;}
static int thumbnail_continue(void){return thumb_visible&&(unsigned)(sceKernelGetSystemTimeLow()-thumb_moved)>=140000u;}
static int find_cover(const unsigned char *data,size_t size,const char *track,uint32_t *dest,uint32_t bg,int logging,unsigned edge,char *source){(void)data;(void)size;(void)bg;(void)logging;assert(test_decode_path[0]&&!strcmp(track,test_decode_path));assert(edge==THUMB_EDGE);test_decodes++;for(unsigned i=0;i<THUMB_PIXELS;i++)dest[i]=thumb_unpack(test_decode_color);strcpy(source,track);return 1;}
static void pixel(int x,int y,uint32_t color){if(x==10&&y==10)test_pixel=color;}
void rounded(int x,int y,int w,int h,int r,uint32_t c){(void)x;(void)y;(void)w;(void)h;(void)r;(void)c;test_pixel=0;}
void rect(int x,int y,int w,int h,uint32_t c){(void)x;(void)y;(void)w;(void)h;(void)c;test_pixel=0;}
