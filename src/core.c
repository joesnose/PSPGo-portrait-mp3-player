/* SPDX-License-Identifier: BSD-2-Clause */
#include "core.h"
#include "fs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>

int hold_event(Hold *h,int down,uint32_t now,uint32_t threshold){
 if(!down){h->down=h->fired=0;return 0;}
 if(!h->down){h->down=1;h->fired=0;h->began=now;}
 if(!h->fired && (uint32_t)(now-h->began)>=threshold){h->fired=1;return 1;}return 0;
}
const char *path_name(const char *p){const char *n=strrchr(p,'/');return n?n+1:p;}
int path_join(char out[PATH_CAP],const char *dir,const char *name){int n=snprintf(out,PATH_CAP,"%s%s%s",dir,dir[0]&&dir[strlen(dir)-1]=='/'?"":"/",name);return n>=0&&n<PATH_CAP;}
int is_mp3(const char *name){size_t n=strlen(name);return n>=4&&!strcasecmp(name+n-4,".mp3");}
int root_add(Roots *r,const char *path){
 size_t n=strlen(path);if(!n||n>=PATH_CAP||strchr(path,'\n')||strchr(path,'\r'))return -1;
 char normalized[PATH_CAP];memcpy(normalized,path,n+1);
 while(n>1&&normalized[n-1]=='/'&&normalized[n-2]!=':')normalized[--n]=0;
 for(int i=0;i<r->count;i++)if(!music_path_compare(r->roots[i],normalized))return i;
 if(r->count>=ROOT_CAP)return -1;
 strcpy(r->roots[r->count],normalized);return r->count++;
}
int roots_load(Roots *r,const char *path){
 memset(r,0,sizeof(*r));FILE *f=fopen(path,"rb");if(!f)return 0;char line[PATH_CAP+32];
 while(fgets(line,sizeof(line),f)){
  if(!strchr(line,'\n')&&!feof(f)){int c;while((c=fgetc(f))!='\n'&&c!=EOF){}continue;}
  line[strcspn(line,"\r\n")]=0;if(!strncmp(line,"root=",5))root_add(r,line+5);else if(!strcmp(line,"defaults_checked=1"))r->defaults_checked=1;
 }int ok=!ferror(f);fclose(f);return ok;
}
int roots_save(const Roots *r,const char *path){
 char tmp[PATH_CAP],bak[PATH_CAP];if(snprintf(tmp,sizeof(tmp),"%s.tmp",path)>=(int)sizeof(tmp)||snprintf(bak,sizeof(bak),"%s.bak",path)>=(int)sizeof(bak))return 0;
 FILE *f=fopen(tmp,"wb");if(!f)return 0;int ok=fputs("# Joe's portrait player: folders selected in the app\n",f)>=0;
 if(ok)ok=fprintf(f,"defaults_checked=%d\n",r->defaults_checked)>=0;
 for(int i=0;i<r->count&&ok;i++)ok=fprintf(f,"root=%s\n",r->roots[i])>=0;
 if(fflush(f))ok=0;if(fclose(f))ok=0;if(!ok){remove(tmp);return 0;}
 if(!rename(tmp,path))return 1;
 remove(bak);int moved=!rename(path,bak);
 if(!rename(tmp,path)){if(moved)remove(bak);return 1;}
 if(moved)rename(bak,path);remove(tmp);return 0;
}
int roots_discover_standard(Roots *r){
 const char *locations[]={"ef0:/MUSIC","ms0:/MUSIC","ef0:/PSP/MUSIC","ms0:/PSP/MUSIC"};int before=r->count;
 for(unsigned i=0;i<sizeof(locations)/sizeof(*locations);i++)if(music_is_dir(locations[i]))root_add(r,locations[i]);
 r->defaults_checked=1;return r->count-before;
}
void library_free(Library *l){for(size_t i=0;i<l->count;i++)free(l->paths[i]);free(l->paths);memset(l,0,sizeof(*l));}
static int append(Library *l,const char *path){
 if(l->count>=TRACK_CAP){l->skipped++;return 1;}
 if(l->count==l->capacity){size_t cap=l->capacity?l->capacity*2:128;if(cap>TRACK_CAP)cap=TRACK_CAP;char **p=realloc(l->paths,cap*sizeof(*p));if(!p)return 0;l->paths=p;l->capacity=cap;}
 size_t n=strlen(path)+1;l->paths[l->count]=malloc(n);if(!l->paths[l->count])return 0;memcpy(l->paths[l->count++],path,n);return 1;
}
static int walk(Library *l,const char *dir,unsigned depth){
 if(depth>64){l->skipped++;return 1;}
 MusicDir *d=music_dir_open(dir);if(!d){l->skipped++;return 1;}MusicEntry e;int ok=1,rc=0;
 char **children=0;size_t count=0,capacity=0;
 /* Finish and close the parent iterator before opening any child. This also
  * keeps the scan from exhausting storage-driver directory handles. */
 while(ok&&(rc=music_dir_read(d,&e))>0){
  if(!strcmp(e.name,".")||!strcmp(e.name,"..")||e.link)continue;
  char path[PATH_CAP];if(!path_join(path,dir,e.name)){l->skipped++;continue;}
  if(e.directory){
   if(count==capacity){size_t next=capacity?capacity*2:16;char **p=realloc(children,next*sizeof(*p));if(!p){ok=0;break;}children=p;capacity=next;}
   children[count]=malloc(strlen(path)+1);if(!children[count]){ok=0;break;}strcpy(children[count++],path);
  }else if(e.regular&&is_mp3(e.name))ok=append(l,path);
 }
 if(rc<0)l->skipped++;music_dir_close(d);
 for(size_t i=0;i<count;i++){if(ok)ok=walk(l,children[i],depth+1);free(children[i]);}free(children);return ok;
}
static int compare(const void *a,const void *b){return music_path_compare(*(const char *const*)a,*(const char *const*)b);}
int library_scan(Library *lib,const Roots *roots){
 Library next={0};for(int i=0;i<roots->count;i++){
  int covered=0;for(int j=0;j<roots->count;j++)if(i!=j){size_t n=strlen(roots->roots[j]);if(n<strlen(roots->roots[i])&&!music_path_prefix(roots->roots[i],roots->roots[j],n)&&(roots->roots[j][n-1]=='/'||roots->roots[i][n]=='/')){covered=1;break;}}
  if(!covered&&!walk(&next,roots->roots[i],0)){library_free(&next);return 0;}
 }
 if(next.count)qsort(next.paths,next.count,sizeof(*next.paths),compare);
 size_t out=0;for(size_t i=0;i<next.count;i++){
  if(out&&!music_path_compare(next.paths[i],next.paths[out-1])){free(next.paths[i]);continue;}next.paths[out++]=next.paths[i];
 }next.count=out;library_free(lib);*lib=next;return 1;
}

static uint32_t be32(const unsigned char *b){return ((uint32_t)b[0]<<24)|((uint32_t)b[1]<<16)|((uint32_t)b[2]<<8)|b[3];}
static uint32_t sync32(const unsigned char *b){return ((uint32_t)(b[0]&127)<<21)|((uint32_t)(b[1]&127)<<14)|((uint32_t)(b[2]&127)<<7)|(b[3]&127);}
static int synchsafe(const unsigned char *b){return !((b[0]|b[1]|b[2]|b[3])&128);}
static void text_tag(char *out,size_t cap,const unsigned char *b,size_t n){
 if(!n)return;unsigned enc=*b++;n--;size_t w=0;
 if(enc==0||enc==3){while(n&&w+1<cap){unsigned c=*b++;n--;if(!c)break;out[w++]=c<32?' ':(char)c;}}
 else if(enc==1||enc==2){int le=enc==1;if(n>=2&&b[0]==255&&b[1]==254){le=1;b+=2;n-=2;}else if(n>=2&&b[0]==254&&b[1]==255){le=0;b+=2;n-=2;}
  while(n>=2&&w+1<cap){unsigned c=le?(b[0]|b[1]<<8):(b[0]<<8|b[1]);b+=2;n-=2;if(!c)break;out[w++]=c>=32&&c<127?(char)c:'?';}}
 out[w]=0;
}
static void apic(Tags *t,const unsigned char *p,size_t n){
 if(t->art||n<5)return;unsigned enc=*p++;n--;const unsigned char *end=memchr(p,0,n);if(!end)return;n-=end-p+1;p=end+1;if(!n)return;
 unsigned type=*p++;n--;if(type!=3&&type!=0)return;
 size_t desc=0;if(enc==0||enc==3){while(desc<n&&p[desc])desc++;if(desc==n)return;desc++;}
 else {while(desc+1<n&&(p[desc]||p[desc+1]))desc+=2;if(desc+1>=n)return;desc+=2;}
 p+=desc;n-=desc;if(!n||n>ART_CAP)return;
 t->art=malloc(n);if(t->art){memcpy(t->art,p,n);t->art_size=n;}
}
void tags_free(Tags *t){free(t->art);memset(t,0,sizeof(*t));}
void tags_read(const char *path,Tags *t){
 memset(t,0,sizeof(*t));snprintf(t->title,sizeof(t->title),"%s",path_name(path));char *dot=strrchr(t->title,'.');if(dot)*dot=0;
 FILE *f=fopen(path,"rb");if(!f)return;unsigned char h[10];if(fread(h,1,10,f)!=10)goto done;
 if(!memcmp(h,"ID3",3)&&h[3]>=2&&h[3]<=4&&synchsafe(h+6)){
  unsigned ver=h[3];size_t size=sync32(h+6);if(size>(size_t)LONG_MAX-20)goto done;
  long total;fseek(f,0,SEEK_END);total=ftell(f);if(total<10||(unsigned long)total<size+10u)goto done;
  t->audio_start=(long)size+10+((ver==4&&(h[5]&16))?10:0);
  /* Unsupported unsynchronised/compressed tags still have a valid audio offset. */
  if(ver==2||size>8u*1024u*1024u||(h[5]&128))goto id1;
  unsigned char *body=malloc(size?size:1);if(!body)goto id1;fseek(f,10,SEEK_SET);if(fread(body,1,size,f)!=size){free(body);goto id1;}
  size_t pos=0;if(h[5]&64){if(size<4){free(body);goto id1;}uint32_t ext=ver==4?sync32(body):be32(body)+4u;if(ext<4||ext>size){free(body);goto id1;}pos=ext;}
  while(pos+10<=size&&body[pos]){
   const unsigned char *fh=body+pos;if(ver==4&&!synchsafe(fh+4))break;size_t n=ver==4?sync32(fh+4):be32(fh+4);pos+=10;if(n>size-pos)break;
   int supported=ver==4?!(fh[9]&0x4fu):!(fh[9]&0xe0u);
   if(supported){
    if(!memcmp(fh,"TIT2",4))text_tag(t->title,sizeof(t->title),body+pos,n);
    else if(!memcmp(fh,"TPE1",4))text_tag(t->artist,sizeof(t->artist),body+pos,n);
    else if(!memcmp(fh,"TALB",4))text_tag(t->album,sizeof(t->album),body+pos,n);
    else if(!memcmp(fh,"APIC",4))apic(t,body+pos,n);
   }pos+=n;
  }free(body);
 }
id1:
 if(!t->artist[0]||!t->album[0]){unsigned char v[128];if(!fseek(f,-128,SEEK_END)&&fread(v,1,128,f)==128&&!memcmp(v,"TAG",3)){
  if(!t->artist[0]){memcpy(t->artist,v+33,30);t->artist[30]=0;}
  if(!t->album[0]){memcpy(t->album,v+63,30);t->album[30]=0;}
 }}
done:fclose(f);
}
