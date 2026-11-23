#include "pcm.h"
#include "slide_panel.h"
#include <assert.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
typedef struct {short *p;size_t count,cap;int fail;} Sink;
static int emit(void *v,int16_t l,int16_t r){Sink *s=v;if(s->fail)return -77;assert(s->count<s->cap);s->p[s->count*2]=l;s->p[s->count*2+1]=r;s->count++;return 0;}
static Sink convert(unsigned rate,int channels,const short *p,size_t frames,int chunk){Sink s={calloc(90000,4),0,90000,0};assert(s.p);PcmConvert c;pcm_init(&c,rate);for(size_t i=0;i<frames;i+=chunk){size_t n=frames-i;if(n>(unsigned)chunk)n=chunk;assert(!pcm_push(&c,p+i*channels,n,channels,emit,&s));}assert(!pcm_finish(&c,emit,&s));assert(s.count==(frames*44100ull+rate-1)/rate);return s;}
int main(void){
 unsigned rates[]={11025,22050,32000,44100,48000};
 for(unsigned j=0;j<5;j++)for(int channels=1;channels<=2;channels++){
  unsigned rate=rates[j];short *in=malloc(rate*channels*sizeof(short));assert(in);for(unsigned i=0;i<rate;i++){in[i*channels]=(short)(12000*sin(2*3.141592653589793*1000*i/rate));if(channels==2)in[i*2+1]=-in[i*2];}
  Sink a=convert(rate,channels,in,rate,1152),b=convert(rate,channels,in,rate,17);assert(a.count==44100&&!memcmp(a.p,b.p,a.count*4));
  int crossings=0;for(size_t i=1;i<a.count;i++){if(a.p[(i-1)*2]<0&&a.p[i*2]>=0)crossings++;if(channels==1)assert(a.p[i*2]==a.p[i*2+1]);}assert(crossings>=998&&crossings<=1001);if(rate==44100&&channels==2)assert(!memcmp(in,a.p,rate*4));free(a.p);free(b.p);free(in);
 }
 PcmConvert c;pcm_init(&c,44100);short sample[2]={1,-1};Sink failure={0,0,0,1};assert(pcm_push(&c,sample,1,2,emit,&failure)==-77);pcm_init(&c,0);assert(pcm_push(&c,sample,1,2,emit,&failure)<0);
 SlidePanel p;slide_panel_init(&p);assert(slide_panel_offset(&p,1)==272);slide_panel_request(&p,1,1000);assert(slide_panel_offset(&p,1000)==272);int halfway=slide_panel_offset(&p,121000);assert(halfway==136);slide_panel_request(&p,0,121000);assert(slide_panel_offset(&p,121000)==halfway);assert(slide_panel_offset(&p,361000)==272);
 slide_panel_request(&p,1,0xffffff00);assert(slide_panel_offset(&p,0x100)>=0);assert(slide_panel_offset(&p,0x40000)==0);
 for(unsigned i=0;i<1000;i++){slide_panel_request(&p,i&1,i*1000);int x=slide_panel_offset(&p,i*1000);assert(x>=0&&x<=272);}
 puts("PASS: 11.025/22.05/32/44.1/48 kHz conversion, stereo/mono, exact frame counts, chunk continuity, tone frequency, error propagation; reversible slide timing and wrap");
}
