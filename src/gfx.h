#ifndef MUSIC_GFX_H
#define MUSIC_GFX_H
#include <stdint.h>
#include <stddef.h>
#include <stdio.h>
#include "theme.h"
#define INK (THEME.ink)
#define MUTED (THEME.muted)
#define BACK (THEME.back)
#define ACCENT (THEME.accent)
#define PANEL (THEME.panel)
#define FRAME (THEME.frame)
#define SECONDARY (THEME.secondary)
int gfx_panel_begin(void);void gfx_panel_end(int x);
void gfx_init(void);void gfx_clear(uint32_t c);void gfx_flip(void);
void rect(int x,int y,int w,int h,uint32_t c);
void text(int x,int y,const char *str,uint32_t c,int scale);
void line_text(int x,int y,const char *str,int chars,uint32_t c);
void rounded(int x,int y,int w,int h,int radius,uint32_t c);
void label(int x,int y,const char *s,uint32_t color,int size);
int label_width(const char *s,int size);
void scrolling_label(unsigned slot,int x,int y,const char *s,int width,uint32_t color,int size,uint32_t key);
int scrolling_label_read(unsigned slot,const char *s,uint32_t key,uint32_t now);
void fitted(int x,int y,const char *s,int width,uint32_t color,int size);
void title_lines(int x,int y,const char *s,int width,uint32_t color);
void folder_icon(int x,int y,uint32_t color);
void music_icon(int x,int y,uint32_t color);
void transport(int playing);
void playback_icon(int x,int y,int playing);
void cover_thumbnails_clear(void);
void folder_thumbnail(int x,int y,const char *folder,const char *representative);
void cover_log(FILE *log);
void cover_load(const unsigned char *data,size_t size,const char *track);
void cover_draw(int x,int y);
#endif
