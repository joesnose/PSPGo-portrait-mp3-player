#ifndef MUSIC_PCM_H
#define MUSIC_PCM_H
#include <stdint.h>
#include <stddef.h>
typedef int (*PcmEmit)(void*,int16_t,int16_t);
typedef struct {unsigned rate;uint64_t frames,next;int16_t left,right;} PcmConvert;
void pcm_init(PcmConvert *c,unsigned rate);
int pcm_push(PcmConvert *c,const int16_t *samples,size_t frames,int channels,PcmEmit emit,void *user);
int pcm_finish(PcmConvert *c,PcmEmit emit,void *user);
#endif
