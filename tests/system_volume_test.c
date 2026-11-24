#include <assert.h>
#include <stdio.h>
#include "helper_mock/pspctrl.h"
#include "helper_mock/pspimpose_driver.h"
static unsigned k1=0x1234;static int volume=15,mute=0,error=0;
unsigned pspSdkSetK1(unsigned value){unsigned old=k1;k1=value;return old;}
int sceImposeGetParam(int which){assert(!k1);return which==PSP_IMPOSE_MAIN_VOLUME?volume:mute;}
int sceImposeSetParam(int which,int value){assert(!k1);if(error)return error;if(which==PSP_IMPOSE_MAIN_VOLUME)volume=value;else mute=value;return 0;}
int sceCtrlPeekBufferPositive(SceCtrlData *p,int n){(void)n;p->Buttons=0;return 1;}
int sceKernelGetModel(void){return 4;}int sceImposeGetStatus(void){return 0;}
int joeMusicAdjustVolume(int);int joeMusicGetVolume(void);
int main(void){
 assert(joeMusicAdjustVolume(1)==16&&volume==16&&k1==0x1234);volume=5;assert(joeMusicAdjustVolume(-1)==4&&joeMusicGetVolume()==4);
 volume=30;assert(joeMusicAdjustVolume(100)==30);volume=0;assert(joeMusicAdjustVolume(-100)==0);
 mute=1;volume=20;assert(joeMusicAdjustVolume(1)==1&&mute==0&&volume==1);
 mute=1;volume=20;assert(joeMusicAdjustVolume(-1)==0&&mute==1);
 error=-7;assert(joeMusicAdjustVolume(1)==-7&&k1==0x1234);
 puts("PASS: shared system volume, physical-level changes, bounds, mute and error/K1 restoration");
}
