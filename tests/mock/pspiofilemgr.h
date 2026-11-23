#ifndef PSP_IO_MOCK_H
#define PSP_IO_MOCK_H
typedef int SceUID;
typedef struct {unsigned st_mode,st_attr;long long st_size;} SceIoStat;
typedef struct {SceIoStat d_stat;char d_name[256];} SceIoDirent;
#define FIO_S_ISLNK(m) (((m)&0xf000u)==0x4000u)
int sceIoDopen(const char*);
int sceIoDread(int,SceIoDirent*);
int sceIoDclose(int);
#define PSP_O_RDONLY 1
int sceIoGetstat(const char*,SceIoStat*);
int sceIoOpen(const char*,int,int);
int sceIoRead(int,void*,unsigned);
int sceIoClose(int);
#endif
