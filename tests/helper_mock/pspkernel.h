#ifndef MOCK_KERNEL_H
#define MOCK_KERNEL_H
#include <stddef.h>
typedef unsigned u32;typedef size_t SceSize;
#define PSP_MODULE_INFO(...)
#define PSP_MAIN_THREAD_ATTR(...)
int sceKernelCpuSuspendIntr(void);void sceKernelCpuResumeIntr(int);
void sceKernelDcacheWritebackInvalidateAll(void);void sceKernelIcacheInvalidateAll(void);int sceKernelDelayThread(unsigned);
#endif
