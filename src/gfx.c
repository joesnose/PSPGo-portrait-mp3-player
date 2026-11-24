#ifdef GFX_CACHE_HOST_TEST
#include "cache_platform.h"
#else
/* SPDX-License-Identifier: BSD-2-Clause */
#include <pspkernel.h>
#include <pspdisplay.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <setjmp.h>
#include <jpeglib.h>
#include <png.h>
#include "gfx.h"
#include "core.h"
#include "fs.h"
#include "ui_fonts.h"
#include "marquee.h"
#include "thumb_store.h"
#include <sys/stat.h>
#include <errno.h>
extern unsigned char msx[];
static uint32_t *fb,*panel_pixels,*panel_base;static int buffer;
static void cover_worker_init(void);
static void cover_seed(const char *track,const char *source,const uint32_t *pixels);
static volatile unsigned thumb_visible,thumb_frame;static volatile uint32_t thumb_moved;
static int thumbnail_continue(void){if(!thumb_visible||(uint32_t)(sceKernelGetSystemTimeLow()-thumb_moved)<140000u)return 0;sceKernelDelayThread(100);return 1;}
static FILE *art_log;
void cover_log(FILE *log){art_log=log;}
static uint32_t cover_pixels[224*224],thumb_work[THUMB_PIXELS];static uint32_t *art=cover_pixels;static int has_art,mixtape;
/* Holding the console clockwise means logical (x,y) is physical (y,271-x). */
static void pixel(int x,int y,uint32_t c){if((unsigned)x<272&&(unsigned)y<480)fb[(271-x)*512+y]=c;}
void gfx_init(void){sceDisplaySetMode(0,480,272);buffer=0;fb=(uint32_t*)0x44000000;gfx_clear(BACK);gfx_flip();cover_worker_init();}
void gfx_clear(uint32_t c){for(int y=0;y<272;y++)for(int x=0;x<512;x++)fb[y*512+x]=c;}
void gfx_flip(void){sceDisplayWaitVblankStart();sceDisplaySetFrameBuf((void*)((uintptr_t)0x44000000+buffer*512*272*4),512,PSP_DISPLAY_PIXEL_FORMAT_8888,PSP_DISPLAY_SETBUF_NEXTFRAME);buffer^=1;fb=(uint32_t*)((uintptr_t)0x44000000+buffer*512*272*4);}
int gfx_panel_begin(void){if(!panel_pixels)panel_pixels=malloc(512*272*sizeof(*panel_pixels));if(!panel_pixels)return 0;panel_base=fb;fb=panel_pixels;gfx_clear(BACK);return 1;}
void gfx_panel_end(int x){fb=panel_base;if(x<0)x=0;if(x>272)x=272;for(int i=x;i<272;i++)memcpy(fb+(271-i)*512,panel_pixels+(271-(i-x))*512,480*sizeof(*fb));}
void rect(int x,int y,int w,int h,uint32_t c){for(int j=0;j<h;j++)for(int i=0;i<w;i++)pixel(x+i,y+j,c);}
static const UiFont *font_for(int size){return &ui_fonts[size<=10?0:size<=13?1:size<=16?2:3];}
static unsigned next_char(const char **p){
 const unsigned char *s=(const unsigned char*)*p;unsigned c=*s++;
 if(c>=0xc2&&c<=0xdf&&(*s&0xc0)==0x80){c=((c&31)<<6)|(*s++&63);}
 else if(c>=128){while((*s&0xc0)==0x80)s++;c='?';}*p=(const char*)s;return c<256?c:'?';
}
static uint32_t blend(uint32_t fg,uint32_t bg,unsigned a){
 unsigned r=((fg&255)*a+(bg&255)*(255-a)+127)/255;
 unsigned g=(((fg>>8)&255)*a+((bg>>8)&255)*(255-a)+127)/255;
 unsigned b=(((fg>>16)&255)*a+((bg>>16)&255)*(255-a)+127)/255;
 return 0xff000000u|r|(g<<8)|(b<<16);
}
void label(int x,int y,const char *s,uint32_t color,int size){
 const UiFont *f=font_for(size);while(*s){unsigned c=next_char(&s);const UiGlyph *g=&f->glyphs[c];
  for(int row=0;row<f->height;row++)for(int col=0;col<g->width;col++){unsigned a=f->pixels[g->offset+row*g->width+col];int px=x+col,py=y+row;if(a&&(unsigned)px<272&&(unsigned)py<480){uint32_t bg=fb[(271-px)*512+py];pixel(px,py,a==255?color:blend(color,bg,a));}}
  x+=g->advance;
 }
}
int label_width(const char *s,int size){const UiFont *f=font_for(size);int w=0;while(*s)w+=f->glyphs[next_char(&s)].advance;return w;}
void fitted(int x,int y,const char *s,int width,uint32_t color,int size){
 if(label_width(s,size)<=width){label(x,y,s,color,size);return;}
 char b[PATH_CAP];size_t n=0;int w=0,extra=label_width("...",size);const UiFont *f=font_for(size);
 while(*s&&n+8<sizeof(b)){const char *start=s;unsigned c=next_char(&s);int adv=f->glyphs[c].advance;if(w+adv+extra>width)break;size_t bytes=s-start;memcpy(b+n,start,bytes);n+=bytes;w+=adv;}
 memcpy(b+n,"...",4);label(x,y,b,color,size);
}
static Marquee marquees[6];
static void clipped_label(int x,int y,const char *s,int left,int right,uint32_t color,int size){
 const UiFont *f=font_for(size);while(*s&&x<right){unsigned c=next_char(&s);const UiGlyph *g=&f->glyphs[c];if(x+g->width<=left){x+=g->advance;continue;}
  for(int row=0;row<f->height;row++)for(int col=0;col<g->width;col++){unsigned a=f->pixels[g->offset+row*g->width+col];int px=x+col,py=y+row;if(a&&px>=left&&px<right&&(unsigned)px<272&&(unsigned)py<480){uint32_t bg=fb[(271-px)*512+py];pixel(px,py,a==255?color:blend(color,bg,a));}}
  x+=g->advance;
 }
}
void scrolling_label(unsigned slot,int x,int y,const char *s,int width,uint32_t color,int size,uint32_t key){
 if(slot>=6||width<=0)return;int tw=label_width(s,size),gap=label_width(" .....  ",size),offset;uint32_t now=sceKernelGetSystemTimeLow();
 if(slot<2)offset=marquee_once(&marquees[slot],s,key,tw,width,now);
 else if(slot==3)offset=marquee_loop(&marquees[slot],s,key,tw,width,gap,now);
 else offset=marquee_offset(&marquees[slot],s,key,tw,width,now);
 clipped_label(x-offset,y,s,x,x+width,color,size);
 if(slot==3&&tw>width){clipped_label(x-offset+tw,y," .....  ",x,x+width,color,size);clipped_label(x-offset+tw+gap,y,s,x,x+width,color,size);}
}
int scrolling_label_read(unsigned slot,const char *s,uint32_t key,uint32_t now){return slot<6&&marquee_read(&marquees[slot],s,key,now);}
void title_lines(int x,int y,const char *s,int width,uint32_t color){
 const UiFont *f=font_for(20);char b[PATH_CAP];size_t n=0,last_space=0;int w=0;
 const char *start=s;while(*s&&n+8<sizeof(b)){const char *p=s;unsigned c=next_char(&s);int adv=f->glyphs[c].advance;if(w+adv>width){s=p;break;}size_t bytes=s-p;if(c==' ')last_space=n;memcpy(b+n,p,bytes);n+=bytes;w+=adv;}
 if(*s&&last_space){n=last_space;s=start+n+1;}b[n]=0;label(x,y,b,color,20);while(*s==' ')s++;if(*s)fitted(x,y+24,s,width,color,20);
}
void text(int x,int y,const char *s,uint32_t color,int scale){label(x,y,s,color,scale>1?20:13);}
void line_text(int x,int y,const char *s,int chars,uint32_t color){fitted(x,y,s,chars*8,color,13);}
void rounded(int x,int y,int w,int h,int radius,uint32_t c){
 for(int j=0;j<h;j++)for(int i=0;i<w;i++){
  int dx=i<radius?radius-1-i:i>=w-radius?i-(w-radius):0;
  int dy=j<radius?radius-1-j:j>=h-radius?j-(h-radius):0;
  if(dx*dx+dy*dy<=radius*radius)pixel(x+i,y+j,c);
 }
}
void folder_icon(int x,int y,uint32_t color){rounded(x,y+4,19,13,2,color);rounded(x+1,y,8,7,2,color);}
void music_icon(int x,int y,uint32_t color){rect(x+9,y,3,13,color);rect(x+10,y,7,3,color);rounded(x+3,y+10,9,6,3,color);}
static void triangle(int x,int y,int direction,uint32_t color){for(int i=0;i<8;i++)rect(x+direction*i,y+i,1,15-2*i,color);}
void playback_icon(int x,int y,int playing){if(playing){rect(x,y,3,12,ACCENT);rect(x+7,y,3,12,ACCENT);}else triangle(x,y,1,ACCENT);}
void transport(int playing){
 rounded(85,439,25,19,8,PANEL);rounded(118,436,36,24,11,ACCENT);rounded(162,439,25,19,8,PANEL);
 rect(92,444,2,9,INK);triangle(102,442,-1,INK);rect(179,444,2,9,INK);triangle(168,442,1,INK);
 if(playing){rect(130,442,3,12,BACK);rect(138,442,3,12,BACK);}else triangle(133,440,1,BACK);
}
static void dimensions(unsigned w,unsigned h,unsigned *dw,unsigned *dh,unsigned edge){*dw=edge;*dh=edge;if(w>h)*dh=h*edge/w;else *dw=w*edge/h;if(!*dw)*dw=1;if(!*dh)*dh=1;}
static void from_rgba(const unsigned char *p,unsigned w,unsigned h,uint32_t *dest,unsigned edge){
 unsigned dw,dh;dimensions(w,h,&dw,&dh,edge);unsigned ox=(edge-dw)/2,oy=(edge-dh)/2;
 for(unsigned y=0;y<dh;y++)for(unsigned x=0;x<dw;x++){const unsigned char *q=p+((y*h/dh)*w+x*w/dw)*4;dest[(y+oy)*edge+x+ox]=0xff000000U|q[0]|((uint32_t)q[1]<<8)|((uint32_t)q[2]<<16);}
}
struct JErr {struct jpeg_error_mgr pub;jmp_buf jump;};
static void jerr(j_common_ptr p){struct JErr *e=(struct JErr*)p->err;longjmp(e->jump,1);}
static int jpeg_art(const unsigned char *data,size_t n,uint32_t *dest,unsigned edge){
 struct jpeg_decompress_struct c;struct JErr e;volatile int created=0;unsigned char *volatile row=0;memset(&c,0,sizeof(c));c.err=jpeg_std_error(&e.pub);e.pub.error_exit=jerr;
 if(setjmp(e.jump)){free((void*)row);if(created)jpeg_destroy_decompress(&c);return 0;}
 jpeg_create_decompress(&c);created=1;jpeg_mem_src(&c,data,n);jpeg_read_header(&c,TRUE);
 if(!c.image_width||!c.image_height||c.image_width>16384||c.image_height>16384){jpeg_destroy_decompress(&c);return 0;}
 int cmyk=c.jpeg_color_space==JCS_CMYK||c.jpeg_color_space==JCS_YCCK;c.out_color_space=cmyk?JCS_CMYK:JCS_RGB;c.scale_num=1;c.scale_denom=1;
 while(c.scale_denom<8&&(c.image_width/c.scale_denom>edge*2||c.image_height/c.scale_denom>edge*2))c.scale_denom*=2;
 if(edge==THUMB_EDGE){c.dct_method=JDCT_IFAST;c.do_fancy_upsampling=FALSE;c.do_block_smoothing=FALSE;}
 jpeg_start_decompress(&c);if(c.output_width>2048||c.output_height>2048||c.output_components!=(cmyk?4:3)){jpeg_destroy_decompress(&c);return 0;}
 row=malloc(c.output_width*c.output_components);if(!row){jpeg_destroy_decompress(&c);return 0;}
 unsigned dw,dh;dimensions(c.output_width,c.output_height,&dw,&dh,edge);unsigned ox=(edge-dw)/2,oy=(edge-dh)/2,ty=0;
 while(c.output_scanline<c.output_height){if(edge==THUMB_EDGE&&!(c.output_scanline&15)&&!thumbnail_continue()){jpeg_abort_decompress(&c);jpeg_destroy_decompress(&c);free((void*)row);return 0;}unsigned sy=c.output_scanline;JSAMPROW r=(unsigned char*)row;jpeg_read_scanlines(&c,&r,1);
  while(ty<dh&&ty*c.output_height/dh==sy){for(unsigned x=0;x<dw;x++){const unsigned char *q=(unsigned char*)row+(x*c.output_width/dw)*c.output_components;unsigned r=q[0],g=q[1],b=q[2];if(cmyk){if(c.saw_Adobe_marker){r=r*q[3]/255;g=g*q[3]/255;b=b*q[3]/255;}else{r=(255-r)*(255-q[3])/255;g=(255-g)*(255-q[3])/255;b=(255-b)*(255-q[3])/255;}}dest[(ty+oy)*edge+x+ox]=0xff000000U|r|(g<<8)|(b<<16);}ty++;}
 }
 jpeg_finish_decompress(&c);jpeg_destroy_decompress(&c);free((void*)row);return 1;
}
static int decode(const unsigned char *data,size_t n,uint32_t *dest,uint32_t background,unsigned edge){
 for(size_t i=0;i<edge*edge;i++)dest[i]=background;
 if(n>=2&&data[0]==255&&data[1]==216)return jpeg_art(data,n,dest,edge);
 if(n>=8&&!png_sig_cmp((png_bytep)data,0,8)){
  png_image im;memset(&im,0,sizeof(im));im.version=PNG_IMAGE_VERSION;
  if(!png_image_begin_read_from_memory(&im,data,n))return 0;
  if(!im.width||!im.height||im.width>1024||im.height>1024){png_image_free(&im);return 0;}im.format=PNG_FORMAT_RGBA;
  unsigned char *p=malloc(PNG_IMAGE_SIZE(im));if(!p){png_image_free(&im);return 0;}
  int ok=png_image_finish_read(&im,0,p,0,0);if(ok)from_rgba(p,im.width,im.height,dest,edge);free(p);png_image_free(&im);return ok;
 }return 0;
}
static int file_art(const char *path,uint32_t *dest,uint32_t background,int logging,unsigned edge,char *source){
 if(edge==THUMB_EDGE&&!thumbnail_continue())return 0;
 FILE *f=fopen(path,"rb");if(!f)return 0;if(fseek(f,0,SEEK_END)){fclose(f);return 0;}long n=ftell(f);if(n<=0||n>(long)ART_CAP){fclose(f);return 0;}rewind(f);
 unsigned char *p=malloc(n);int ok=0;if(p){size_t got=0;while(got<(size_t)n){if(edge==THUMB_EDGE&&!thumbnail_continue())break;size_t chunk=(size_t)n-got;if(chunk>16384)chunk=16384;if(fread(p+got,1,chunk,f)!=chunk)break;got+=chunk;}if(got==(size_t)n)ok=decode(p,n,dest,background,edge);free(p);}fclose(f);if(logging&&art_log){fprintf(art_log,"ART_FILE decoded=%d bytes=%ld path=%s\n",ok,n,path);fflush(art_log);}if(ok&&source)snprintf(source,PATH_CAP,"%s",path);return ok;
}
static int find_cover(const unsigned char *data,size_t size,const char *track,uint32_t *dest,uint32_t background,int logging,unsigned edge,char *source){
 if(edge==THUMB_EDGE&&!thumbnail_continue())return 0;
if(data&&size){int ok=decode(data,size,dest,background,edge);if(logging&&art_log){fprintf(art_log,"ART_EMBEDDED decoded=%d bytes=%u track=%s\n",ok,(unsigned)size,track);fflush(art_log);}if(ok){if(source)snprintf(source,PATH_CAP,"%s",track);return 1;}}
 char dir[PATH_CAP];snprintf(dir,sizeof(dir),"%s",track?track:"");char *slash=strrchr(dir,'/');if(!slash)return 0;*slash=0;
 const char *names[]={"cover.jpg","folder.jpg","cover.jpeg","folder.jpeg","front.jpg","front.jpeg","album.jpg","album.jpeg","albumart.jpg","AlbumArtSmall.jpg","cover.png","folder.png","front.png","Cover.jpg","Folder.jpg","Cover.png","Folder.png"};
 for(unsigned i=0;i<sizeof(names)/sizeof(names[0]);i++){char p[PATH_CAP];if(path_join(p,dir,names[i])&&file_art(p,dest,background,logging,edge,source))return 1;}
 if(edge==THUMB_EDGE&&!thumbnail_continue())return 0;
 /* Native directory discovery accepts arbitrary sleeve names and mixed case. */
 typedef struct {char path[PATH_CAP];int score;} Candidate;Candidate *images=calloc(64,sizeof(*images));int count=0;MusicDir *d=images?(logging?music_dir_open(dir):music_dir_open_quiet(dir)):0;
 if(d){MusicEntry e;while(count<64&&music_dir_read(d,&e)>0){if(e.directory||e.link)continue;const char *ext=strrchr(e.name,'.');if(!ext||(strcasecmp(ext,".jpg")&&strcasecmp(ext,".jpeg")&&strcasecmp(ext,".png")))continue;char lower[PATH_CAP];snprintf(lower,sizeof(lower),"%s",e.name);for(char *c=lower;*c;c++)if(*c>='A'&&*c<='Z')*c+=32;
 int score=strstr(lower,"cover")||strstr(lower,"front")?30:strstr(lower,"folder")||strstr(lower,"albumart")?20:10;if(strstr(lower,"back")||strstr(lower,"disc"))score=1;
 if(path_join(images[count].path,dir,e.name)){images[count].score=score;count++;}}
 music_dir_close(d);
 for(int pass=0;pass<count;pass++){int best=-1;for(int i=0;i<count;i++)if(images[i].score>0&&(best<0||images[i].score>images[best].score||(images[i].score==images[best].score&&strcasecmp(images[i].path,images[best].path)<0)))best=i;if(best<0)break;if(file_art(images[best].path,dest,background,logging,edge,source)){free(images);return 1;}images[best].score=0;}}
 free(images);if(logging&&art_log){fprintf(art_log,"ART_MISSING track=%s\n",track);fflush(art_log);}
return 0;
}
void cover_load(const unsigned char *data,size_t size,const char *track){
 mixtape=0;if(track){char parent[PATH_CAP];snprintf(parent,sizeof(parent),"%s",track);char *end=strrchr(parent,'/');if(end)*end=0;mixtape=!strcasecmp(parent,"fatms0:/MUSIC")||!strcasecmp(parent,"fatms0:/PSP/MUSIC")||!strcasecmp(parent,"ms0:/MUSIC")||!strcasecmp(parent,"ef0:/MUSIC")||!strcasecmp(parent,"ms0:/PSP/MUSIC")||!strcasecmp(parent,"ef0:/PSP/MUSIC");}
 char source[PATH_CAP]="";has_art=find_cover(data,size,track,art,PANEL,1,224,source);if(has_art)cover_seed(track,source,art);
}
void cover_draw(int x,int y){
 if(has_art){for(int j=0;j<224;j++)for(int i=0;i<224;i++){int dx=i<10?9-i:i>=214?i-214:0,dy=j<10?9-j:j>=214?j-214:0;if(dx*dx+dy*dy<=100)pixel(x+i,y+j,art[j*224+i]);}return;}
 rounded(x,y,224,224,12,PANEL);
 if(mixtape){rounded(x+30,y+59,164,107,13,0xff77705c);rounded(x+35,y+64,154,96,10,0xffc9d9dc);rounded(x+44,y+73,136,47,6,0xff514536);rounded(x+52,y+85,120,23,10,0xff24201c);for(int i=0;i<2;i++){rounded(x+58+i*80,y+83,24,27,12,0xffe4f0ed);rounded(x+65+i*80,y+90,10,13,5,0xff474034);}rect(x+70,y+135,84,17,0xff54493d);label(x+84,y+179,"MIXTAPE",ACCENT,13);}
 else {rounded(x+55,y+40,114,132,27,music_theme?SECONDARY:0xff8d9c7c);rounded(x+58,y+43,108,125,24,music_theme?blend(ACCENT,PANEL,100):0xff4f6650);rounded(x+63,y+47,98,55,20,music_theme?blend(INK,ACCENT,90):0xffa2bba0);rect(x+115,y+67,9,67,(music_theme?INK:0xffedfff0));rect(x+121,y+67,27,9,(music_theme?INK:0xffedfff0));rounded(x+94,y+119,30,20,9,(music_theme?INK:0xffedfff0));label(x+70,y+185,"No album art",MUTED,10);}
}

#endif

/* One descriptor per album; compact pixels are retained to a bounded budget. */
#define THUMB_BUDGET (4u*1024u*1024u)
#define THUMB_TILE_BYTES (THUMB_PIXELS*sizeof(uint16_t))
#define HOT_COUNT 12
typedef struct {char *folder,*track,*source;uint16_t *pixels;uint32_t background;unsigned used,ticket,seen;int available,state,focused,dirty;} AlbumThumb;
typedef struct {uint32_t pixels[THUMB_PIXELS];unsigned album,ticket,used;int valid;} HotThumb;
static AlbumThumb *thumb_cache;static unsigned thumb_count,thumb_clock,thumb_ticket,thumb_bytes,thumb_retained,thumb_focus;
static HotThumb hot_thumbs[HOT_COUNT];static unsigned hot_clock;
static char thumb_directory[PATH_CAP];static unsigned thumb_decoded,thumb_disk_hits,thumb_saved,thumb_errors;
static uint32_t save_retry,load_retry;
static SceUID thumb_mutex=-1,thumb_wake=-1,thumb_thread=-1;static int thumb_quit;
static void thumb_lock(void){sceKernelWaitSema(thumb_mutex,1,0);}static void thumb_unlock(void){sceKernelSignalSema(thumb_mutex,1);}
static char *thumb_string(const char *s){size_t n=strlen(s)+1;char *p=malloc(n);if(p)memcpy(p,s,n);return p;}
static int due(uint32_t now,uint32_t retry){return !retry||(int32_t)(now-retry)>=0;}
static void free_pixels(AlbumThumb *t){if(t->pixels){free(t->pixels);t->pixels=0;thumb_bytes-=THUMB_TILE_BYTES;thumb_retained--;}t->available=0;}
static uint16_t *reserve_pixels(unsigned slot){
 while(thumb_bytes+THUMB_TILE_BYTES>THUMB_BUDGET){unsigned oldest=thumb_count;for(unsigned i=0;i<thumb_count;i++)if(i!=slot&&thumb_cache[i].pixels&&!thumb_cache[i].dirty&&(unsigned)(thumb_frame-thumb_cache[i].seen)>1u&&(oldest==thumb_count||thumb_cache[i].used<thumb_cache[oldest].used))oldest=i;if(oldest==thumb_count)return 0;free_pixels(&thumb_cache[oldest]);thumb_cache[oldest].state=1;}
 uint16_t *pixels=malloc(THUMB_TILE_BYTES);if(pixels){thumb_bytes+=THUMB_TILE_BYTES;thumb_retained++;}return pixels;
}
static void cover_seed(const char *track,const char *source,const uint32_t *pixels){
 if(thumb_mutex<0||!track||!source||!source[0])return;thumb_lock();
 for(unsigned album=0;album<thumb_count;album++){AlbumThumb *t=&thumb_cache[album];if(t->track&&!music_path_compare(t->track,track)){
  if(!t->available){uint16_t *compact=reserve_pixels(album);if(compact){for(unsigned y=0;y<THUMB_EDGE;y++)for(unsigned x=0;x<THUMB_EDGE;x++)compact[y*THUMB_EDGE+x]=thumb_pack(pixels[(y*224/THUMB_EDGE)*224+x*224/THUMB_EDGE]);t->pixels=compact;t->available=1;t->state=3;t->background=PANEL;t->ticket=++thumb_ticket;if(thumb_directory[0]){free(t->source);t->source=thumb_string(source);t->dirty=t->source!=0;}}}break;
 }}thumb_unlock();
}
static int cover_worker(SceSize size,void *arg){
 (void)size;(void)arg;
 for(;;){
  char folder[PATH_CAP],track[PATH_CAP],source[PATH_CAP]="";uint16_t pixels[THUMB_PIXELS];unsigned slot,ticket=0;uint32_t background=0;int saving=0;
  thumb_lock();int quit=thumb_quit;slot=thumb_count;uint32_t now=sceKernelGetSystemTimeLow();int quiet=(uint32_t)(now-thumb_moved)>=140000u;
  if(!quit&&quiet&&thumb_visible&&due(now,load_retry))for(unsigned i=0;i<thumb_count;i++)if(thumb_cache[i].state==1&&(unsigned)(thumb_frame-thumb_cache[i].seen)<=1u&&(slot==thumb_count||thumb_cache[i].focused>thumb_cache[slot].focused||(thumb_cache[i].focused==thumb_cache[slot].focused&&thumb_cache[i].used>thumb_cache[slot].used)))slot=i;
  if(!quit&&quiet&&slot==thumb_count&&thumb_directory[0]&&due(now,save_retry))for(unsigned i=0;i<thumb_count;i++)if(thumb_cache[i].dirty&&thumb_cache[i].pixels&&thumb_cache[i].source){slot=i;saving=1;break;}
  /* Prepare a small neighbourhood after visible work and pending saves. */
  if(!quit&&quiet&&thumb_visible&&slot==thumb_count&&due(now,load_retry))for(unsigned distance=1;distance<=8&&slot==thumb_count;distance++){unsigned after=thumb_focus+distance;if(after<thumb_count&&thumb_cache[after].state==1)slot=after;if(slot==thumb_count&&distance<=thumb_focus&&thumb_cache[thumb_focus-distance].state==1)slot=thumb_focus-distance;}
  int has_job=slot<thumb_count;
  if(has_job){AlbumThumb *t=&thumb_cache[slot];ticket=t->ticket;background=t->background;snprintf(folder,sizeof(folder),"%s",t->folder);snprintf(track,sizeof(track),"%s",t->track);if(saving){snprintf(source,sizeof(source),"%s",t->source);memcpy(pixels,t->pixels,sizeof(pixels));}else t->state=2;}
  thumb_unlock();if(quit)break;if(!has_job){SceUInt timeout=20000;sceKernelWaitSema(thumb_wake,1,&timeout);continue;}
  if(saving){int ok=thumb_store_save(thumb_directory,folder,track,source,background,pixels);thumb_lock();if(ok){thumb_saved++;save_retry=0;if(slot<thumb_count&&thumb_cache[slot].ticket==ticket){thumb_cache[slot].dirty=0;free(thumb_cache[slot].source);thumb_cache[slot].source=0;}}else{thumb_errors++;save_retry=sceKernelGetSystemTimeLow()+5000000u;}thumb_unlock();continue;}
  int cached=thumb_store_load(thumb_directory,folder,track,background,pixels),available=cached;
  if(!cached){Tags metadata={0};if(!tags_read_interruptible(track,&metadata,thumbnail_continue)){tags_free(&metadata);thumb_lock();if(slot<thumb_count&&thumb_cache[slot].ticket==ticket)thumb_cache[slot].state=1;thumb_unlock();continue;}
   available=find_cover(metadata.art,metadata.art_size,track,thumb_work,background,0,THUMB_EDGE,source);tags_free(&metadata);if(available)for(unsigned i=0;i<THUMB_PIXELS;i++)pixels[i]=thumb_pack(thumb_work[i]);}
  if(!thumbnail_continue()){thumb_lock();if(slot<thumb_count&&thumb_cache[slot].ticket==ticket)thumb_cache[slot].state=1;thumb_unlock();continue;}
  thumb_lock();if(cached)thumb_disk_hits++;else thumb_decoded++;
  if(slot<thumb_count&&thumb_cache[slot].ticket==ticket){AlbumThumb *t=&thumb_cache[slot];if(available){uint16_t *memory=reserve_pixels(slot);if(memory){memcpy(memory,pixels,sizeof(pixels));t->pixels=memory;t->available=1;t->state=3;if(!cached&&thumb_directory[0]&&source[0]){t->source=thumb_string(source);t->dirty=t->source!=0;}}else{t->state=1;load_retry=sceKernelGetSystemTimeLow()+1000000u;}}else t->state=3;}
  thumb_unlock();
 }
 return 0;
}
static void cover_worker_init(void){
 thumb_mutex=sceKernelCreateSema("art_cache",0,1,1,0);thumb_wake=sceKernelCreateSema("art_jobs",0,0,1,0);if(thumb_mutex<0||thumb_wake<0)return;
 thumb_thread=sceKernelCreateThread("album_art",cover_worker,0x38,64*1024,PSP_THREAD_ATTR_USER,0);if(thumb_thread>=0&&sceKernelStartThread(thumb_thread,0,0)<0){sceKernelDeleteThread(thumb_thread);thumb_thread=-1;}
}
void cover_cache_setup(const char *base){
 int made=-1;if(path_join(thumb_directory,base,"cover-cache-v1")){made=mkdir(thumb_directory,0777);MusicDir *directory=music_dir_open_quiet(thumb_directory);if(directory)music_dir_close(directory);else thumb_directory[0]=0;}else thumb_directory[0]=0;
 if(art_log){fprintf(art_log,"ART_CACHE enabled=%d mkdir=%d path=%s\n",!!thumb_directory[0],made,thumb_directory);fflush(art_log);}
}
void cover_library_activity(int visible,int moving,uint32_t now){thumb_visible=!!visible;if(moving)thumb_moved=now;thumb_frame++;}
void cover_cache_stats(unsigned *decoded,unsigned *disk){if(thumb_mutex>=0)thumb_lock();*decoded=thumb_decoded;*disk=thumb_disk_hits;if(thumb_mutex>=0)thumb_unlock();}
void cover_cache_details(unsigned *retained,unsigned *bytes,unsigned *saved,unsigned *errors){if(thumb_mutex>=0)thumb_lock();*retained=thumb_retained;*bytes=thumb_bytes;*saved=thumb_saved;*errors=thumb_errors;if(thumb_mutex>=0)thumb_unlock();}
static void clear_entries(void){for(unsigned i=0;i<thumb_count;i++){free(thumb_cache[i].folder);free(thumb_cache[i].track);free(thumb_cache[i].source);free(thumb_cache[i].pixels);}free(thumb_cache);thumb_cache=0;thumb_count=thumb_bytes=thumb_retained=0;memset(hot_thumbs,0,sizeof(hot_thumbs));}
void cover_thumbnails_prepare(char *const *paths,const int *indices,unsigned count){
 AlbumThumb *entries=count?calloc(count,sizeof(*entries)):0;if(count&&!entries)count=0;
 for(unsigned i=0;i<count;i++){char folder[PATH_CAP];snprintf(folder,sizeof(folder),"%s",paths[indices[i]]);char *slash=strrchr(folder,'/');if(slash)*slash=0;entries[i].folder=thumb_string(folder);entries[i].track=thumb_string(paths[indices[i]]);entries[i].state=entries[i].folder&&entries[i].track?1:3;entries[i].background=PANEL;}
 if(thumb_mutex>=0)thumb_lock();clear_entries();thumb_cache=entries;thumb_count=count;thumb_focus=0;thumb_clock=0;load_retry=0;for(unsigned i=0;i<count;i++)entries[i].ticket=++thumb_ticket;if(thumb_mutex>=0)thumb_unlock();
}
void cover_thumbnails_clear(void){if(thumb_mutex>=0)thumb_lock();for(unsigned i=0;i<thumb_count;i++){AlbumThumb *t=&thumb_cache[i];free_pixels(t);free(t->source);t->source=0;t->dirty=0;t->background=PANEL;t->ticket=++thumb_ticket;t->state=t->folder&&t->track?1:3;}memset(hot_thumbs,0,sizeof(hot_thumbs));load_retry=0;if(thumb_mutex>=0)thumb_unlock();}
void cover_shutdown(void){
 thumb_visible=0;if(thumb_thread>=0){thumb_lock();thumb_quit=1;thumb_unlock();sceKernelSignalSema(thumb_wake,1);sceKernelWaitThreadEnd(thumb_thread,0);sceKernelDeleteThread(thumb_thread);thumb_thread=-1;}
 uint32_t flush_started=sceKernelGetSystemTimeLow();
 for(unsigned i=0;i<thumb_count&&(uint32_t)(sceKernelGetSystemTimeLow()-flush_started)<1500000u;i++){AlbumThumb *t=&thumb_cache[i];if(t->dirty&&t->source&&t->pixels&&thumb_directory[0]){if(thumb_store_save(thumb_directory,t->folder,t->track,t->source,t->background,t->pixels))thumb_saved++;else thumb_errors++;}}
 clear_entries();if(thumb_wake>=0){sceKernelDeleteSema(thumb_wake);thumb_wake=-1;}if(thumb_mutex>=0){sceKernelDeleteSema(thumb_mutex);thumb_mutex=-1;}
}
void folder_thumbnail(int x,int y,unsigned album,int focused){
 HotThumb *hot=0;if(thumb_thread>=0){thumb_lock();if(album<thumb_count){AlbumThumb *t=&thumb_cache[album];t->used=++thumb_clock;t->seen=thumb_frame;t->focused=focused;if(focused)thumb_focus=album;
  unsigned oldest=0;for(unsigned i=0;i<HOT_COUNT;i++){if(hot_thumbs[i].valid&&hot_thumbs[i].album==album&&hot_thumbs[i].ticket==t->ticket){hot=&hot_thumbs[i];break;}if(!hot_thumbs[i].valid)oldest=i;else if(hot_thumbs[oldest].valid&&hot_thumbs[i].used<hot_thumbs[oldest].used)oldest=i;}
  if(!hot&&t->available&&t->pixels){hot=&hot_thumbs[oldest];hot->valid=1;hot->album=album;hot->ticket=t->ticket;for(unsigned i=0;i<THUMB_PIXELS;i++)hot->pixels[i]=thumb_unpack(t->pixels[i]);}if(hot)hot->used=++hot_clock;
 }thumb_unlock();}
 if(hot){for(int j=0;j<THUMB_EDGE;j++)for(int i=0;i<THUMB_EDGE;i++){int dx=i<4?3-i:i>=THUMB_EDGE-4?i-(THUMB_EDGE-4):0,dy=j<4?3-j:j>=THUMB_EDGE-4?j-(THUMB_EDGE-4):0;if(dx*dx+dy*dy<=16)pixel(x+i,y+j,hot->pixels[j*THUMB_EDGE+i]);}}
 else{rounded(x,y,THUMB_EDGE,THUMB_EDGE,5,PANEL);rounded(x+2,y+13,40,27,4,MUTED);rounded(x+3,y+7,17,12,3,MUTED);rect(x+5,y+17,34,2,ACCENT);}
}
