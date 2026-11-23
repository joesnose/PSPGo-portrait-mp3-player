#ifndef MUSIC_SLIDE_PANEL_H
#define MUSIC_SLIDE_PANEL_H
#include <stdint.h>
typedef struct {int from,target;uint32_t began;} SlidePanel;
void slide_panel_init(SlidePanel *p);
int slide_panel_offset(const SlidePanel *p,uint32_t now);
void slide_panel_request(SlidePanel *p,int open,uint32_t now);
#endif
