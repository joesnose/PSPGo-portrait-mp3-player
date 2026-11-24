#include "mp3_info.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
static void put(unsigned char *p,unsigned v){p[0]=v>>24;p[1]=v>>16;p[2]=v>>8;p[3]=v;}
int main(void){
 unsigned char b[100]={0xff,0xfb,0x90,0};memcpy(b+36,"Xing",4);put(b+40,1);put(b+44,3446);assert(mp3_header_duration(b,sizeof(b))==90);
 assert(!mp3_header_duration(b,47));put(b+40,0);assert(!mp3_header_duration(b,sizeof(b)));
 memset(b,0,sizeof(b));b[0]=0xff;b[1]=0xf3;b[2]=0x80;b[3]=0xc0;memcpy(b+13,"Info",4);put(b+17,1);put(b+21,3446);assert(mp3_header_duration(b,sizeof(b))==90);
 memset(b,0,sizeof(b));b[0]=0xff;b[1]=0xfb;b[2]=0x90;memcpy(b+36,"VBRI",4);put(b+50,3446);assert(mp3_header_duration(b,sizeof(b))==90);
 for(unsigned n=0;n<100;n++){memset(b,0x7f,sizeof(b));assert(!mp3_header_duration(b,n));}
 puts("PASS: MPEG1/MPEG2 Xing/Info, VBRI, absent counts, truncation and malformed headers");
}
