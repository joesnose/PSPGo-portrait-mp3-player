/* SPDX-License-Identifier: BSD-2-Clause */
#include "marquee.h"
#include <string.h>
#include <stdio.h>
#define START_PAUSE 700000u
#define END_PAUSE 900000u
static uint64_t travel(const Marquee *s){return (uint64_t)(s->text_width-s->width)*1000000u/32u;}
int marquee_offset(Marquee *s,const char *text,uint32_t key,int text_width,int width,uint32_t now){
 if(!s->valid||s->key!=key||s->text_width!=text_width||s->width!=width||strcmp(s->text,text)||(uint32_t)(now-s->last_seen)>500000u){snprintf(s->text,sizeof(s->text),"%s",text);s->key=key;s->text_width=text_width;s->width=width;s->began=now;s->valid=1;}
 s->last_seen=now;if(text_width<=width||width<=0)return 0;
 uint64_t moving=travel(s);if(!moving)moving=1;uint64_t t=(uint32_t)(now-s->began),cycle=START_PAUSE+moving+END_PAUSE+moving;t%=cycle;
 if(t<START_PAUSE)return 0;t-=START_PAUSE;
 if(t<moving)return (int)((uint64_t)(text_width-width)*t/moving);t-=moving;
 if(t<END_PAUSE)return text_width-width;t-=END_PAUSE;
 return (text_width-width)-(int)((uint64_t)(text_width-width)*t/moving);
}
int marquee_read(const Marquee *s,const char *text,uint32_t key,uint32_t now){
 if(!s->valid||s->key!=key||strcmp(s->text,text)||(uint32_t)(now-s->last_seen)>500000u)return 0;
 if(s->text_width<=s->width)return 1;
 return (uint64_t)(uint32_t)(now-s->began)>=START_PAUSE+travel(s)+END_PAUSE;
}

int marquee_once(Marquee *s,const char *text,uint32_t key,int text_width,int width,uint32_t now){
 marquee_offset(s,text,key,text_width,width,now);if(text_width<=width)return 0;
 uint64_t t=(uint32_t)(now-s->began),moving=travel(s);
 if(t<START_PAUSE)return 0;if(t>=START_PAUSE+moving)return text_width-width;
 return (int)((t-START_PAUSE)*32u/1000000u);
}
int marquee_loop(Marquee *s,const char *text,uint32_t key,int text_width,int width,int gap,uint32_t now){
 marquee_offset(s,text,key,text_width,width,now);if(text_width<=width)return 0;
 uint64_t moving=(uint64_t)(text_width+gap)*1000000u/32u,t=(uint32_t)(now-s->began);t%=START_PAUSE+moving;
 return t<START_PAUSE?0:(int)((t-START_PAUSE)*32u/1000000u);
}
