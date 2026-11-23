#ifndef MOCK_CTRL_H
#define MOCK_CTRL_H
#define PSP_CTRL_HOME 0x10000u
#define PSP_CTRL_VOLUP 0x100000u
#define PSP_CTRL_VOLDOWN 0x200000u
typedef struct {unsigned TimeStamp,Buttons;unsigned char Lx,Ly,Rsrv[6];} SceCtrlData;
int sceCtrlPeekBufferPositive(SceCtrlData*,int);int sceCtrlReadBufferPositive(SceCtrlData*,int);int sceCtrlPeekBufferNegative(SceCtrlData*,int);int sceCtrlReadBufferNegative(SceCtrlData*,int);
typedef struct {unsigned uiMake,uiBreak,uiPress,uiRelease;} SceCtrlLatch;
int sceCtrlPeekLatch(SceCtrlLatch*);int sceCtrlReadLatch(SceCtrlLatch*);
#endif
