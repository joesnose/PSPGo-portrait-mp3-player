#ifndef MOCK_LOADCORE_H
#define MOCK_LOADCORE_H
#include "pspkernel.h"
typedef struct {void *stub_top;unsigned stub_size;} SceModule;
typedef struct {const char *libname;unsigned char version[2];unsigned short attribute;unsigned char len,vstubcount;unsigned short stubcount;unsigned *nidtable;void *stubtable,*vstubtable;} SceLibraryStubTable;
SceModule *sceKernelFindModuleByName(const char*);
#endif
