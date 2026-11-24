#ifndef MUSIC_THUMB_STORE_H
#define MUSIC_THUMB_STORE_H
#include <stdint.h>
#include "core.h"
#define THUMB_EDGE 44
#define THUMB_PIXELS (THUMB_EDGE*THUMB_EDGE)
static inline uint16_t thumb_pack(uint32_t c){return (uint16_t)(((c&0xf8u)<<8)|((c&0xfc00u)>>5)|((c&0xf80000u)>>19));}
static inline uint32_t thumb_unpack(uint16_t c){unsigned r=(c>>11)&31,g=(c>>5)&63,b=c&31;return 0xff000000u|((r<<3)|(r>>2))|(((g<<2)|(g>>4))<<8)|(((b<<3)|(b>>2))<<16);}
int thumb_store_load(const char *dir,const char *folder,const char *track,uint32_t background,uint16_t pixels[THUMB_PIXELS]);
int thumb_store_save(const char *dir,const char *folder,const char *track,const char *source,uint32_t background,const uint16_t pixels[THUMB_PIXELS]);
#endif
