/* SPDX-License-Identifier: BSD-2-Clause
 * Recovery helper: controls/status and bounded system-volume adjustment; no patches or hooks.
 */
#include <pspkernel.h>
#include <pspctrl.h>
#include <pspsysmem_kernel.h>
#include <pspsdk.h>
#include <pspimpose_driver.h>
extern int sceImposeGetStatus(void);
PSP_MODULE_INFO("JoeMusicButtons12",0x1000,0,17);
PSP_MAIN_THREAD_ATTR(0);
unsigned int joeMusicReadControls(void){
 unsigned k1=pspSdkSetK1(0);SceCtrlData pad;int rc=sceCtrlPeekBufferPositive(&pad,1);pspSdkSetK1(k1);
 return rc>0?(pad.Buttons&(PSP_CTRL_HOME|PSP_CTRL_VOLUP|PSP_CTRL_VOLDOWN)):0;
}
int joeMusicGetVolume(void){unsigned k1=pspSdkSetK1(0);int v=sceImposeGetParam(PSP_IMPOSE_MAIN_VOLUME);if(v>=0&&sceImposeGetParam(PSP_IMPOSE_MUTE)>0)v=0;pspSdkSetK1(k1);return v;}
int joeMusicHomeActive(void){unsigned k1=pspSdkSetK1(0);int status=sceImposeGetStatus();pspSdkSetK1(k1);return status<0?status:!!(status&0x100);}
/* PSP Go slide-switch flag in the kernel controller buffer. */
int joeMusicSlideState(void){
 unsigned k1=pspSdkSetK1(0);int state=-1;
 if(sceKernelGetModel()==4){SceCtrlData pad;int rc=sceCtrlPeekBufferPositive(&pad,1);if(rc>0)state=!!(pad.Buttons&0x20000000u);}
 pspSdkSetK1(k1);return state;
}
int module_start(SceSize size,void *arg){(void)size;(void)arg;return 0;}
int module_stop(SceSize size,void *arg){(void)size;(void)arg;return 0;}

int joeMusicAdjustVolume(int delta){
 unsigned k1=pspSdkSetK1(0);int v=sceImposeGetParam(PSP_IMPOSE_MAIN_VOLUME),mute=sceImposeGetParam(PSP_IMPOSE_MUTE);
 if(v>=0&&mute>=0){if(mute)v=0;delta=delta<0?-1:delta>0?1:0;v+=delta;if(v<0)v=0;if(v>30)v=30;int rc=sceImposeSetParam(PSP_IMPOSE_MAIN_VOLUME,v);if(rc<0)v=rc;else if(delta>0&&mute){rc=sceImposeSetParam(PSP_IMPOSE_MUTE,0);if(rc<0)v=rc;}}
 pspSdkSetK1(k1);return v;
}
