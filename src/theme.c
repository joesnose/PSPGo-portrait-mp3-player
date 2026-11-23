/* SPDX-License-Identifier: BSD-2-Clause */
#include "theme.h"
#include <stdio.h>
#include <string.h>
#define RGB(r,g,b) (0xff000000u|((uint32_t)(b)<<16)|((uint32_t)(g)<<8)|(r))
const MusicTheme music_themes[5]={
 {"Sage",0xfff4f6f1u,0xffaaa49bu,0xff17130fu,0xffaadfb0u,0xff29231du,0xff0b0907u,0xffaadfb0u},
 {"Blue",RGB(224,232,237),RGB(145,162,175),RGB(16,22,29),RGB(137,169,194),RGB(29,40,52),RGB(9,15,22),RGB(108,139,163)},
 {"Red",RGB(237,226,224),RGB(175,152,151),RGB(27,18,20),RGB(195,139,143),RGB(48,32,36),RGB(17,10,13),RGB(163,112,119)},
 {"Yellow",RGB(235,232,218),RGB(169,163,139),RGB(24,23,17),RGB(192,180,122),RGB(43,40,28),RGB(14,13,8),RGB(164,149,99)},
 {"Neon",RGB(235,249,255),RGB(138,157,197),RGB(5,7,19),RGB(45,248,237),RGB(17,22,48),RGB(78,33,116),RGB(249,67,222)}
};
int music_theme;
void theme_load(const char *path){music_theme=0;FILE *f=fopen(path,"rb");if(!f)return;char name[24];if(fgets(name,sizeof(name),f)){name[strcspn(name,"\r\n")]=0;for(int i=0;i<THEME_COUNT;i++)if(!strcmp(name,music_themes[i].name)){music_theme=i;break;}}fclose(f);}
int theme_save(const char *path){
 char temp[1100],backup[1100];if(snprintf(temp,sizeof(temp),"%s.tmp",path)>=(int)sizeof(temp)||snprintf(backup,sizeof(backup),"%s.bak",path)>=(int)sizeof(backup))return 0;
 FILE *f=fopen(temp,"wb");if(!f)return 0;int ok=fprintf(f,"%s\n",THEME.name)>0;if(fclose(f))ok=0;if(!ok){remove(temp);return 0;}
 if(!rename(temp,path))return 1;
 /* PSP FAT rename can refuse replacing an existing destination. */
 remove(backup);int moved=!rename(path,backup);
 if(!rename(temp,path)){if(moved)remove(backup);return 1;}
 if(moved)rename(backup,path);remove(temp);return 0;
}
