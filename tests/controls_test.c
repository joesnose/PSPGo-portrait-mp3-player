#include "controls.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
int main(void){
 Press p={0};assert(!press_event(&p,1,100,800));assert(press_event(&p,0,300,800)==PRESS_SHORT);assert(!press_event(&p,0,400,800));
 assert(!press_event(&p,1,1000,800));assert(press_event(&p,1,1800,800)==PRESS_LONG);assert(!press_event(&p,1,3000,800));assert(!press_event(&p,0,3100,800));
 assert(!press_event(&p,1,4000,800));assert(press_event(&p,0,5000,800)==PRESS_LONG);
 assert(!press_event(&p,1,0xffffff00,800));assert(press_event(&p,1,0x220,800)==PRESS_LONG);assert(!press_event(&p,0,0x221,800));
 assert(!press_event(&p,1,6000,800));press_cancel(&p);assert(!press_event(&p,1,9000,800));assert(!press_event(&p,0,9001,800));
 /* Parent-folder tracks are deliberately interleaved with subfolders. */
 char *paths[]={"ms0:/MUSIC/A/01.mp3","ms0:/MUSIC/A/Disc 2/01.mp3","ms0:/MUSIC/A/02.mp3","ms0:/MUSIC/B/01.mp3","ms0:/MUSIC/B/02.mp3","ms0:/MUSIC/C/01.mp3"};Library l={paths,6,6,0};
 assert(library_browse(&l,0,1,0)==2);assert(library_browse(&l,2,1,0)==0);assert(library_browse(&l,0,-1,0)==2);
 assert(library_browse(&l,0,1,1)==1);assert(library_browse(&l,1,1,1)==3);assert(library_browse(&l,3,-1,1)==1);assert(library_browse(&l,0,-1,1)==5);assert(library_browse(&l,5,1,1)==0);
 assert(library_browse(&l,4,-1,0)==3);assert(library_browse(&l,5,1,0)==5);Library empty={0};assert(library_browse(&empty,0,1,1)==-1);
 int *folders=0;size_t count=0;assert(library_folders(&l,&folders,&count));assert(count==4);assert(folders[0]==0&&folders[1]==1&&folders[2]==3&&folders[3]==5);free(folders);
 assert(library_folders(&empty,&folders,&count)&&!folders&&!count);
 char *different[]={"ms0:/MUSIC/Album/01.mp3","ef0:/MUSIC/Album/01.mp3","ms0:/MUSIC/Album/02.mp3","ms0:/MUSIC/loose.mp3"};Library devices={different,4,4,0};assert(library_folders(&devices,&folders,&count));assert(count==3&&folders[0]==1&&folders[1]==3&&folders[2]==0);free(folders);
 uint32_t seed=1;int *order=0;assert(library_shuffle(100,&seed,&order));int seen[100]={0};for(int i=0;i<100;i++){assert(order[i]>=0&&order[i]<100);seen[order[i]]++;}for(int i=0;i<100;i++)assert(seen[i]==1);free(order);assert(library_shuffle(0,&seed,&order)&&!order);
 uint32_t last=0;int armed=0;assert(!double_tap(&last,&armed,1000,PRESS_SHORT));assert(double_tap(&last,&armed,2000,PRESS_SHORT));assert(!double_tap(&last,&armed,3000,PRESS_SHORT));assert(!double_tap(&last,&armed,4000,PRESS_LONG));assert(!armed);assert(!double_tap(&last,&armed,5000,PRESS_SHORT));assert(!double_tap(&last,&armed,355000,PRESS_SHORT));armed=1;last=0xffffff00;assert(double_tap(&last,&armed,0x100,PRESS_SHORT));
 BrowseIdle idle={0};assert(!browse_idle_due(&idle,5000000));browse_idle_touch(&idle,100);assert(!browse_idle_due(&idle,4000099));assert(browse_idle_due(&idle,4000100));assert(!browse_idle_due(&idle,8000200));browse_idle_touch(&idle,100);browse_idle_touch(&idle,2000000);assert(!browse_idle_due(&idle,4000100));assert(browse_idle_due(&idle,6000000));browse_idle_touch(&idle,0xfffffff0);assert(!browse_idle_due(&idle,0x100));assert(browse_idle_due(&idle,(uint32_t)(0xfffffff0u+4000000u)));
 puts("PASS: four-second idle deadline, reset on scroll, single return and timer wrap");
 puts("PASS: shuffle visits every track once; double-tap timing, long-press cancellation and timer wrap");
 puts("PASS: unique full-path folders across nested directories and devices");
 puts("PASS: short-on-release, single long event, release after missed threshold, timer wrap, cancellation, per-folder track wrap, album ordering and wrap");
}
