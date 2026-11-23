/* SPDX-License-Identifier: BSD-2-Clause */
#include "controls.h"
#include "fs.h"
#include <string.h>
#include <stdlib.h>
int press_event(Press *s,int down,uint32_t now,uint32_t threshold){
 if(!down){int event=PRESS_NONE;if(s->down&&!s->fired&&!s->cancelled)event=(uint32_t)(now-s->began)>=threshold?PRESS_LONG:PRESS_SHORT;memset(s,0,sizeof(*s));return event;}
 if(!s->down){s->down=1;s->began=now;}
 if(!s->fired&&!s->cancelled&&(uint32_t)(now-s->began)>=threshold){s->fired=1;return PRESS_LONG;}return PRESS_NONE;
}
void press_cancel(Press *s){s->cancelled=1;}
static void parent(const char *p,char out[PATH_CAP]){size_t n=strlen(p);if(n>=PATH_CAP)n=PATH_CAP-1;memcpy(out,p,n);out[n]=0;char *slash=strrchr(out,'/');if(slash)*slash=0;}
int library_browse(const Library *l,int current,int direction,int album){
 if(!l->count)return -1;if(current<0||current>=(int)l->count)current=0;direction=direction<0?-1:1;
 char folder[PATH_CAP];parent(l->paths[current],folder);
 if(!album){for(size_t step=1;step<=l->count;step++){int i=(current+direction*(int)step)%(int)l->count;if(i<0)i+=(int)l->count;char p[PATH_CAP];parent(l->paths[i],p);if(!music_path_compare(p,folder))return i;}return current;}
 int best=-1,wrap=-1;char best_name[PATH_CAP]="",wrap_name[PATH_CAP]="";
 for(size_t i=0;i<l->count;i++){
  char p[PATH_CAP];parent(l->paths[i],p);int rel=music_path_compare(p,folder);
  if(wrap<0||(direction>0?music_path_compare(p,wrap_name)<0:music_path_compare(p,wrap_name)>0)){wrap=(int)i;strcpy(wrap_name,p);}
  if((direction>0?rel>0:rel<0)&&(best<0||(direction>0?music_path_compare(p,best_name)<0:music_path_compare(p,best_name)>0))){best=(int)i;strcpy(best_name,p);}
 }
 return best>=0?best:wrap;
}
static const Library *folder_sort_library;
static int compare_folder_indices(const void *a,const void *b){int x=*(const int*)a,y=*(const int*)b;char px[PATH_CAP],py[PATH_CAP];parent(folder_sort_library->paths[x],px);parent(folder_sort_library->paths[y],py);int c=music_path_compare(px,py);return c?c:x-y;}
int library_folders(const Library *l,int **indices,size_t *count){
 *indices=0;*count=0;if(!l->count)return 1;int *out=malloc(l->count*sizeof(*out));if(!out)return 0;for(size_t i=0;i<l->count;i++)out[i]=(int)i;
 folder_sort_library=l;qsort(out,l->count,sizeof(*out),compare_folder_indices);folder_sort_library=0;
 char previous[PATH_CAP]="";for(size_t i=0;i<l->count;i++){char dir[PATH_CAP];parent(l->paths[out[i]],dir);if(!*count||music_path_compare(dir,previous)){strcpy(previous,dir);out[(*count)++]=out[i];}}
 *indices=out;return 1;
}
static uint32_t shuffle_random(uint32_t *state){uint32_t x=*state?*state:0x9e3779b9u;x^=x<<13;x^=x>>17;x^=x<<5;return *state=x;}
int library_shuffle(size_t count,uint32_t *seed,int **out){*out=0;if(!count)return 1;int *p=malloc(count*sizeof(*p));if(!p)return 0;for(size_t i=0;i<count;i++)p[i]=(int)i;for(size_t i=count;i>1;i--){size_t j=shuffle_random(seed)%i;int tmp=p[i-1];p[i-1]=p[j];p[j]=tmp;}*out=p;return 1;}
int double_tap(uint32_t *last,int *armed,uint32_t now,int event){if(event==PRESS_LONG){*armed=0;return 0;}if(event!=PRESS_SHORT)return 0;if(*armed&&(uint32_t)(now-*last)<350000u){*armed=0;return 1;}*last=now;*armed=1;return 0;}

void browse_idle_touch(BrowseIdle *s,uint32_t now){s->changed=now;s->armed=1;}
int browse_idle_due(BrowseIdle *s,uint32_t now){if(!s->armed||(uint32_t)(now-s->changed)<4000000u)return 0;s->armed=0;return 1;}
