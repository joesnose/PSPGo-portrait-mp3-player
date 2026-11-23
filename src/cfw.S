.set noreorder
#include "pspimport.s"
IMPORT_START "KUBridge",0x40090000
IMPORT_FUNC "KUBridge",0x4C25EA72,kuKernelLoadModule
IMPORT_START "SystemCtrlForUser",0x40090000
IMPORT_FUNC "SystemCtrlForUser",0x159AF5CC,sctrlHENFindFunction
IMPORT_FUNC "SystemCtrlForUser",0x56CEAF00,sctrlKernelQuerySystemCall

IMPORT_FUNC "SystemCtrlForUser",0xAD9849FE,sctrlKernelMsIsEf
