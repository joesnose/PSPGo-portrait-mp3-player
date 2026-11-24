/* PSP firmware MP3 pipeline, based on the PSPSDK BSD MP3 sample by Alexander Berl. */
#include <pspkernel.h>
#include <pspaudio.h>
#include <pspmp3.h>
#include <psputility.h>
#include <pspiofilemgr.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "player.h"
#include "pcm.h"
#include "mp3_info.h"
static unsigned duration;static int duration_exact;static int seeking;
static unsigned char input[32768] __attribute__((aligned(64)));
static unsigned char pcm[18432] __attribute__((aligned(64)));
#define MIX_FRAMES 1024
static short mix[2][MIX_FRAMES*2] __attribute__((aligned(64)));
typedef struct {int channel,which,filled;} MixOutput;
static int output_sample(void *user,int16_t left,int16_t right){MixOutput *o=user;mix[o->which][o->filled*2]=left;mix[o->which][o->filled*2+1]=right;if(++o->filled==MIX_FRAMES){int rc=sceAudioOutputBlocking(o->channel,PSP_AUDIO_VOLUME_MAX,mix[o->which]);if(rc<0)return rc;o->filled=0;o->which^=1;}return 0;}
static int finish_output(MixOutput *o){if(!o->filled)return 0;memset(mix[o->which]+o->filled*2,0,(MIX_FRAMES-o->filled)*2*sizeof(short));int rc=sceAudioOutputBlocking(o->channel,PSP_AUDIO_VOLUME_MAX,mix[o->which]);if(rc<0)return rc;o->filled=0;o->which^=1;return 0;}
static SceUID mutex=-1,thread=-1;
static Playback published;
static char wanted[PATH_CAP];
static int request,paused,quitting,revision,seek_pending;static unsigned seek_target;
static void lock(void){sceKernelWaitSema(mutex,1,0);}static void unlock(void){sceKernelSignalSema(mutex,1);}
static void publish(int state,int error,int generation,uint64_t frames,int rate,int bitrate,const char *path){
 lock();published.duration_exact=duration_exact;published.seeking=seeking||seek_pending;published.duration=duration;published.state=state;published.error=error;published.generation=generation;published.seconds=published.seeking?seek_target:rate>0?frames/(unsigned)rate:0;published.rate=rate;published.bitrate=bitrate;snprintf(published.path,sizeof(published.path),"%s",path);unlock();
}
static int fill(int fd,int handle,int *eof,SceOff end){
 if(*eof)return 0; /* Never feed a decoder that wraps its request offset at EOF. */
 unsigned char *dst;SceInt32 n,pos;int rc=sceMp3GetInfoToAddStreamData(handle,&dst,&n,&pos);if(rc<0)return rc;if(pos>=end){*eof=1;return 0;}if(n<=0)return 0;
 if(sceIoLseek32(fd,pos,PSP_SEEK_SET)<0)return -1;int got=sceIoRead(fd,dst,n);if(got==0||(got>0&&(SceOff)pos+got>=end))*eof=1;if(got<=0)return got;rc=sceMp3NotifyAddStreamData(handle,got);return rc<0?rc:got;
}
static unsigned sync32(const unsigned char *p){return ((p[0]&127u)<<21)|((p[1]&127u)<<14)|((p[2]&127u)<<7)|(p[3]&127u);}
/* Drain before releasing: a queued DMA buffer still belongs to the device. */
static int release_channel(int *channel){
 if(*channel<0)return 0;
 for(int i=0;i<120;i++){int remaining=sceAudioGetChannelRestLen(*channel);if(remaining<0)return remaining;if(!remaining){int rc=sceAudioChRelease(*channel);if(rc<0)return rc;*channel=-1;return 0;}sceKernelDelayThread(5000);}
 return (int)0x80260002;
}
static int worker(SceSize size,void *arg){
 (void)size;(void)arg;int fd=-1,handle=-1,channel=-1,rate=0,channels=0,bitrate=0,gen=0,state=PLAYER_STOPPED;uint64_t played=0,target=0;int eof=0,empty=0;SceOff stream_end=0;PcmConvert convert;MixOutput output={-1,0,0};char path[PATH_CAP]="";
 for(;;){
  lock();int quit=quitting,cmd=request,pause=paused;request=0;char next[PATH_CAP];if(cmd==1)strcpy(next,wanted);int newgen=revision,do_seek=seek_pending;unsigned destination=seek_target;seek_pending=0;unlock();
  if(quit||cmd){
   int release_error=release_channel(&channel);
   if(release_error<0&&!quit){state=PLAYER_ERROR;publish(state,release_error,gen,played,rate,bitrate,path);continue;}
   if(handle>=0){sceMp3ReleaseMp3Handle(handle);handle=-1;}if(fd>=0){sceIoClose(fd);fd=-1;}
   rate=channels=bitrate=0;played=duration=0;duration_exact=0;seeking=eof=empty=0;gen=newgen;output=(MixOutput){-1,0,0};
   if(quit)break;
   state=PLAYER_STOPPED;
   if(cmd==1){
    strcpy(path,next);state=PLAYER_LOADING;publish(state,0,gen,0,0,0,path);
    fd=sceIoOpen(path,PSP_O_RDONLY,0);int err=fd<0?fd:0;
    if(!err){
     SceOff end=sceIoLseek(fd,0,PSP_SEEK_END);stream_end=end;unsigned char hdr[10];sceIoLseek32(fd,0,PSP_SEEK_SET);int n=sceIoRead(fd,hdr,10);unsigned start=0;
     if(n==10&&!memcmp(hdr,"ID3",3)&&hdr[3]>=2&&hdr[3]<=4&&!((hdr[6]|hdr[7]|hdr[8]|hdr[9])&128))start=10+sync32(hdr+6)+((hdr[3]==4&&(hdr[5]&16))?10:0);
     unsigned hint=0;if(end>start){unsigned char prefix[1024];if(sceIoLseek32(fd,start,PSP_SEEK_SET)>=0){int got=sceIoRead(fd,prefix,sizeof(prefix));if(got>0)hint=mp3_header_duration(prefix,(size_t)got);}}
     if(end<=start||end>0x7fffffff)err=-2;
     else {SceMp3InitArg init;memset(&init,0,sizeof(init));init.mp3StreamStart=start;init.mp3StreamEnd=end;init.mp3Buf=input;init.mp3BufSize=sizeof(input);init.pcmBuf=pcm;init.pcmBufSize=sizeof(pcm);handle=sceMp3ReserveMp3Handle(&init);if(handle<0)err=handle;}
     if(!err){int n=fill(fd,handle,&eof,stream_end);if(n<=0)err=n?n:-3;}
     if(!err)err=sceMp3Init(handle);
     if(err>=0){err=sceMp3SetLoopNum(handle,0);rate=sceMp3GetSamplingRate(handle);channels=sceMp3GetMp3ChannelNum(handle);bitrate=sceMp3GetBitRate(handle);if(rate<=0||(channels!=1&&channels!=2))err=-4;else {int count=sceMp3GetFrameNum(handle);int samples=rate>=32000?1152:576;duration_exact=hint>0;duration=hint?hint:count>0?(unsigned)((uint64_t)count*samples/rate):bitrate>0?(unsigned)((end-start)*8/(bitrate*1000)):0;}}
    }
    if(err<0){state=PLAYER_ERROR;publish(state,err,gen,0,rate,bitrate,path);}
    else {channel=sceAudioChReserve(PSP_AUDIO_NEXT_CHANNEL,MIX_FRAMES,PSP_AUDIO_FORMAT_STEREO);if(channel<0){state=PLAYER_ERROR;publish(state,channel,gen,0,rate,bitrate,path);}else{output.channel=channel;pcm_init(&convert,(unsigned)rate);state=PLAYER_PLAYING;}}
   }else{path[0]=0;publish(state,0,gen,0,0,0,path);}
  }
  if(state==PLAYER_ERROR||state==PLAYER_STOPPED||state==PLAYER_ENDED){sceKernelDelayThread(10000);continue;}
  if(do_seek&&!cmd){
   target=(uint64_t)destination*(unsigned)rate;seeking=1;output.filled=0;pcm_init(&convert,(unsigned)rate);
   empty=0;
   /* The frame-reset API can leave the streaming decoder at EOF on hardware.
      Reset only to the known stream start, then decode-discard to the target. */
   if(target<played){int rc=sceMp3ResetPlayPosition(handle);if(rc<0){state=PLAYER_ERROR;publish(state,rc,gen,played,rate,bitrate,path);continue;}played=0;eof=0;}

  }
  if(seeking&&played>=target){seeking=0;pcm_init(&convert,(unsigned)rate);}
  if(pause&&!seeking){state=PLAYER_PAUSED;publish(state,0,gen,played,rate,bitrate,path);sceKernelDelayThread(10000);continue;}
  state=PLAYER_PLAYING;
  int err=0;if(sceMp3CheckStreamDataNeeded(handle)>0){int rc=fill(fd,handle,&eof,stream_end);if(rc<0)err=rc;}
  short *decoded=0;int bytes=err?err:sceMp3Decode(handle,&decoded);
  if(bytes==0||bytes==(int)0x80671402){
   if(seeking&&!eof){int rc=fill(fd,handle,&eof,stream_end);if(rc<0){state=PLAYER_ERROR;publish(state,rc,gen,played,rate,bitrate,path);continue;}if(!eof&&++empty<100){sceKernelDelayThread(5000);continue;}if(!eof){state=PLAYER_ERROR;publish(state,-7,gen,played,rate,bitrate,path);continue;}}
   if(seeking){seeking=0;lock();paused=1;unlock();state=PLAYER_PAUSED;publish(state,0,gen,played,rate,bitrate,path);continue;}
   int rc=pcm_finish(&convert,output_sample,&output);if(rc>=0)rc=finish_output(&output);state=rc<0?PLAYER_ERROR:PLAYER_ENDED;publish(state,rc<0?rc:0,gen,played,rate,bitrate,path);continue;}
  if(bytes<0||!decoded){state=PLAYER_ERROR;publish(state,bytes<0?bytes:-5,gen,played,rate,bitrate,path);continue;}
  empty=0;int frames=bytes/(2*channels);if(frames<=0||frames>9216){state=PLAYER_ERROR;publish(state,-6,gen,played,rate,bitrate,path);continue;}
  /* Decode-discard preserves VBR timing and the MP3 bit reservoir. No PCM is queued while seeking. */
  int rc=seeking?0:pcm_push(&convert,decoded,(size_t)frames,channels,output_sample,&output);
  if(rc<0){state=PLAYER_ERROR;publish(state,rc,gen,played,rate,bitrate,path);continue;}
  played+=(unsigned)frames;publish(pause?PLAYER_PAUSED:state,0,gen,played,rate,bitrate,path);
 }
 return 0;
}
int player_init(void){
 int rc=sceUtilityLoadModule(PSP_MODULE_AV_AVCODEC);if(rc<0)return rc;
 rc=sceUtilityLoadModule(PSP_MODULE_AV_MP3);if(rc<0)return rc;rc=sceMp3InitResource();if(rc<0)return rc;
 mutex=sceKernelCreateSema("music_state",0,1,1,0);if(mutex<0)return mutex;
 thread=sceKernelCreateThread("music_audio",worker,0x18,64*1024,PSP_THREAD_ATTR_USER,0);if(thread<0)return thread;return sceKernelStartThread(thread,0,0);
}
int player_play(const char *path){if(mutex<0)return -1;lock();snprintf(wanted,sizeof(wanted),"%s",path);paused=0;seek_pending=0;request=1;int generation=++revision;unlock();return generation;}
void player_pause(void){if(mutex<0)return;lock();paused=!paused;unlock();}
void player_stop(void){if(mutex<0)return;lock();request=2;paused=0;seek_pending=0;revision++;unlock();}
void player_seek(int delta){if(mutex<0)return;lock();if(!request&&(published.state==PLAYER_PLAYING||published.state==PLAYER_PAUSED)){int64_t value=(seek_pending||published.seeking?seek_target:published.seconds)+(int64_t)delta;if(value<0)value=0;if(published.duration_exact&&published.duration&&value>=(int64_t)published.duration)value=published.duration>1?published.duration-1:0;seek_target=(unsigned)value;seek_pending=1;published.seeking=1;published.seconds=seek_target;}unlock();}
Playback player_snapshot(void){Playback p={0};if(mutex>=0){lock();p=published;unlock();}return p;}
void player_shutdown(void){
 if(mutex>=0){lock();quitting=1;unlock();}if(thread>=0){sceKernelWaitThreadEnd(thread,0);sceKernelDeleteThread(thread);thread=-1;}
 if(mutex>=0){sceKernelDeleteSema(mutex);mutex=-1;}sceMp3TermResource();sceUtilityUnloadModule(PSP_MODULE_AV_MP3);sceUtilityUnloadModule(PSP_MODULE_AV_AVCODEC);
}
