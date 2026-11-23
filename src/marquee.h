#ifndef MUSIC_MARQUEE_H
#define MUSIC_MARQUEE_H
#include <stdint.h>
#include "core.h"
typedef struct {char text[PATH_CAP];uint32_t key,began,last_seen;int text_width,width,valid;} Marquee;
int marquee_offset(Marquee *s,const char *text,uint32_t key,int text_width,int width,uint32_t now);
int marquee_read(const Marquee *s,const char *text,uint32_t key,uint32_t now);
#endif
