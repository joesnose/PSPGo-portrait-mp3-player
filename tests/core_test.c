#include "core.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
static void file(const char *p,const void *data,size_t n){FILE *f=fopen(p,"wb");assert(f);assert(fwrite(data,1,n,f)==n);assert(!fclose(f));}
static void path(char out[PATH_CAP],const char *root,const char *name){assert(path_join(out,root,name));}
static void frame(unsigned char *b,size_t *pos,const char *id,const unsigned char *data,size_t n){memcpy(b+*pos,id,4);memset(b+*pos+4,0,6);b[*pos+7]=(unsigned char)n;*pos+=10;memcpy(b+*pos,data,n);*pos+=n;}
int main(void){
 Hold h={0};assert(!hold_event(&h,1,100,800));assert(!hold_event(&h,1,899,800));assert(hold_event(&h,1,900,800));assert(!hold_event(&h,1,4000,800));assert(!hold_event(&h,0,4001,800));assert(!hold_event(&h,1,0xffffff00,800));assert(hold_event(&h,1,0x220,800));
 char dir[]="/tmp/joemusic-test-XXXXXX";assert(mkdtemp(dir));char sub[PATH_CAP],nested[PATH_CAP],p[PATH_CAP];path(sub,dir,"album");assert(!mkdir(sub,0700));path(nested,sub,"disc");assert(!mkdir(nested,0700));
 path(p,dir,"one.mp3");file(p,"test",4);path(p,sub,"two.MP3");file(p,"test",4);path(p,nested,"three.mp3");file(p,"test",4);path(p,nested,"ignore.mp3.txt");file(p,"test",4);path(p,nested,"loop");assert(!symlink(dir,p));
 Roots r={0};assert(root_add(&r,dir)==0);char trailing[PATH_CAP];snprintf(trailing,sizeof(trailing),"%s/",dir);assert(root_add(&r,trailing)==0);assert(root_add(&r,sub)==1);
 Library lib={0};assert(library_scan(&lib,&r));assert(lib.count==3);library_free(&lib);
 char cfg[PATH_CAP];path(cfg,dir,"folders.cfg");assert(roots_save(&r,cfg));Roots loaded={0};assert(roots_load(&loaded,cfg));assert(loaded.count==2&&!strcmp(loaded.roots[1],sub));assert(roots_save(&r,cfg));
 assert(!loaded.defaults_checked);r.defaults_checked=1;assert(roots_save(&r,cfg));assert(roots_load(&loaded,cfg));assert(loaded.defaults_checked&&loaded.count==2);
 unsigned char tag[1024]={0};size_t pos=10;const unsigned char title[]={3,'A',' ','t','i','t','l','e'};frame(tag,&pos,"TIT2",title,sizeof(title));const unsigned char artist[]={1,255,254,'J',0,'o',0,'e',0};frame(tag,&pos,"TPE1",artist,sizeof(artist));const unsigned char art[]={0,'i','m','a','g','e','/','p','n','g',0,3,0,137,'P','N','G'};frame(tag,&pos,"APIC",art,sizeof(art));memcpy(tag,"ID3\3\0\0",6);unsigned n=pos-10;tag[6]=(n>>21)&127;tag[7]=(n>>14)&127;tag[8]=(n>>7)&127;tag[9]=n&127;path(p,dir,"tags.mp3");file(p,tag,pos);Tags t;tags_read(p,&t);assert(!strcmp(t.title,"A title")&&!strcmp(t.artist,"Joe"));assert(t.art_size==4&&!memcmp(t.art,"\x89PNG",4));assert(t.audio_start==(long)pos);tags_free(&t);
 /* A valid outer tag with an overflowing inner frame must stop safely. */
 memset(tag,0,sizeof(tag));memcpy(tag,"ID3\3\0\0\0\0\0\x0aTIT2\xff\xff\xff\xff",18);file(p,tag,20);tags_read(p,&t);assert(!t.art);tags_free(&t);
 unsigned seed=17;for(int i=0;i<300;i++){for(size_t j=0;j<sizeof(tag);j++){seed=seed*1664525u+1013904223u;tag[j]=(unsigned char)(seed>>24);}memcpy(tag,"ID3",3);tag[3]=3+(i&1);tag[5]=0;tag[6]=tag[7]=0;tag[8]=7;tag[9]=118;file(p,tag,sizeof(tag));tags_read(p,&t);tags_free(&t);}
 printf("PASS: recursive/overlapping folders, symlink loop, config persistence, metadata/artwork bounds, malformed tags, hold timing/wrap\n");return 0;
}
