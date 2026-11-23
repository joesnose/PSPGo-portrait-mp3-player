/* SPDX-License-Identifier: BSD-2-Clause
 * Streaming conversion to the standard PSP mixer's 44.1 kHz stereo format.
 */
#include "pcm.h"
#include <string.h>
void pcm_init(PcmConvert *c,unsigned rate){memset(c,0,sizeof(*c));c->rate=rate;}
int pcm_push(PcmConvert *c,const int16_t *samples,size_t frames,int channels,PcmEmit emit,void *user){
 if(!c->rate||(channels!=1&&channels!=2))return -1;
 for(size_t i=0;i<frames;i++){
  int16_t left=samples[i*channels],right=channels==1?left:samples[i*channels+1];
  if(c->rate==44100){int rc=emit(user,left,right);if(rc<0)return rc;c->frames++;continue;}
  if(!c->frames){int rc=emit(user,left,right);if(rc<0)return rc;c->next=c->rate;}
  else {uint64_t boundary=c->frames*44100u,base=boundary-44100u;
   while(c->next<=boundary){unsigned frac=(unsigned)(c->next-base);int l=c->left+(int)(((int64_t)left-c->left)*frac/44100),r=c->right+(int)(((int64_t)right-c->right)*frac/44100);int rc=emit(user,(int16_t)l,(int16_t)r);if(rc<0)return rc;c->next+=c->rate;}
  }
  c->left=left;c->right=right;c->frames++;
 }return 0;
}
int pcm_finish(PcmConvert *c,PcmEmit emit,void *user){
 if(c->rate&&c->rate!=44100&&c->frames)while(c->next<c->frames*44100u){int rc=emit(user,c->left,c->right);if(rc<0)return rc;c->next+=c->rate;}return 0;
}
