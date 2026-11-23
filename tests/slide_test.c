/* Firmware mocks verify unsupported models/errors aren't reported as closed. */
#include <assert.h>
#include <stdio.h>
#include "../helper/main.c"
static int model=4,result=1;static unsigned buttons,k1=0x1234;
unsigned pspSdkSetK1(unsigned v){unsigned old=k1;k1=v;return old;}
int sceKernelGetModel(void){return model;}
int sceCtrlPeekBufferPositive(SceCtrlData *p,int n){assert(n==1);p->Buttons=buttons;return result;}
int sceImposeGetStatus(void){return 0;}int sceImposeGetParam(int n){return n==1?15:0;}
int main(void){buttons=0x20000000;assert(joeMusicSlideState()==1&&k1==0x1234);buttons=0;assert(joeMusicSlideState()==0&&k1==0x1234);model=0;assert(joeMusicSlideState()==-1&&k1==0x1234);model=4;result=-1;assert(joeMusicSlideState()==-1&&k1==0x1234);result=0;assert(joeMusicSlideState()==-1&&k1==0x1234);puts("PASS: slide open/closed, unsupported-model and failed-read fallback, K1 restoration");}
