/* SPDX-License-Identifier: BSD-2-Clause */
#include "mp3_info.h"
#include <stdint.h>
#include <string.h>
static uint32_t be32(const unsigned char *p){return (uint32_t)p[0]<<24|(uint32_t)p[1]<<16|(uint32_t)p[2]<<8|p[3];}
unsigned mp3_header_duration(const unsigned char *p,size_t size){
 static const unsigned rates[]={44100,48000,32000};
 for(size_t i=0;i+4<=size;i++){
  uint32_t h=be32(p+i);unsigned version=(h>>19)&3,layer=(h>>17)&3,rate_index=(h>>10)&3,bitrate=(h>>12)&15;
  if((h&0xffe00000u)!=0xffe00000u||version==1||layer!=1||rate_index==3||!bitrate||bitrate==15)continue;
  unsigned rate=rates[rate_index]/(version==3?1:version==2?2:4),samples=version==3?1152:576;
  unsigned mono=((h>>6)&3)==3,side=version==3?(mono?17:32):(mono?9:17);size_t x=i+4+((h&0x10000)?0:2)+side;
  uint32_t frames=0;
  if(x+12<=size&&(!memcmp(p+x,"Xing",4)||!memcmp(p+x,"Info",4))&&(be32(p+x+4)&1))frames=be32(p+x+8);
  size_t v=i+36;if(!frames&&v+18<=size&&!memcmp(p+v,"VBRI",4))frames=be32(p+v+14);
  if(frames)return (unsigned)((uint64_t)frames*samples/rate);
  /* Only the first MPEG audio frame carries this information. */
  return 0;
 }
 return 0;
}
