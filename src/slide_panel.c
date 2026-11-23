/* SPDX-License-Identifier: BSD-2-Clause */
#include "slide_panel.h"
void slide_panel_init(SlidePanel *p){*p=(SlidePanel){272,272,0};}
int slide_panel_offset(const SlidePanel *p,uint32_t now){uint32_t age=now-p->began;if(age>=240000u||p->from==p->target)return p->target;float t=age/240000.0f;float ease=t*t*(3.0f-2.0f*t);return p->from+(int)((p->target-p->from)*ease);}
void slide_panel_request(SlidePanel *p,int open,uint32_t now){int target=open?0:272;if(target==p->target)return;p->from=slide_panel_offset(p,now);p->target=target;p->began=now;}
