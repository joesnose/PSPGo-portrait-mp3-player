/* SPDX-License-Identifier: BSD-2-Clause */
#include <pspkernel.h>
#include <pspctrl.h>
#include <pspdisplay.h>
#include <psppower.h>
#include <pspimpose.h>
#include <pspiofilemgr.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>
#include <dirent.h>
#include <sys/stat.h>
#include "core.h"
#include "fs.h"
#include "player.h"
#include "gfx.h"
#include "probe.h"
#include "controls.h"
#include "slide_panel.h"
#include "session.h"
PSP_MODULE_INFO("JoesPortraitMusic",0,0,16);
PSP_MAIN_THREAD_ATTR(PSP_THREAD_ATTR_USER);
PSP_MAIN_THREAD_STACK_SIZE_KB(512);
PSP_HEAP_SIZE_KB(20*1024);
extern SceUID kuKernelLoadModule(const char*,int,SceKernelLMOption*);
extern void *sctrlHENFindFunction(const char*,const char*,unsigned);
extern int sctrlKernelQuerySystemCall(void*);
extern int sctrlKernelMsIsEf(void);
extern int sceImposeHomeButton(void);
static volatile int running=1;
static Roots roots;static Library library;static Tags tags;
static char theme_file[PATH_CAP];static char base[PATH_CAP],config[PATH_CAP],browser[PATH_CAP],shown[PATH_CAP],notice[128],direct_playing[PATH_CAP];
static uint32_t title_changed;static float drum_offset;static char title_old[256],album_old[256],title_new[256],album_new[256];
static int *shuffle_order,shuffle_enabled,shuffle_cursor;static unsigned library_revision;static int *album_indices,album_count,album_selected;static const char *card_root="ms0:/";static int direct_card;
static BrowseIdle browse_idle;static char preview_art_path[PATH_CAP];static int folder_preview;static int page,selected,folder_selected,browse_selected,playing_index=-1,ended_generation=-1;
static int audio_ready,helper_ready,helper_uid=-1;static unsigned (*volume_read)(void);
static int (*read_volume)(void),(*read_home_active)(void),(*read_slide)(void);
static int slide_state=-1;static uint32_t slide_polled;
static uint32_t syscall_stubs[4][16] __attribute__((aligned(64)));
static int home_custom,previous_popup=-1,menu_waiting,menu_seen,storage_redirected;
static uint32_t menu_requested_at,volume_shown_at,volume_polled_at;
static int stick_volume_overlay;static int volume_reversed;static int volume_visible,system_volume=-1,last_volume=-1,volume_direction;
typedef struct {char *name;int directory;} BrowserEntry;
static char browse_error[100];static int browser_folders,browser_tracks,browser_other;
static BrowserEntry *entries;static int dir_count;static FILE *logfile;
enum {NOW,LIBRARY,FOLDERS,BROWSE,DIRECT};
static int exiting(int a,int b,void *p){(void)a;(void)b;(void)p;running=0;return 0;}
static int callback_thread(SceSize n,void *p){(void)n;(void)p;int cb=sceKernelCreateCallback("music_exit",exiting,0);if(cb>=0)sceKernelRegisterExitCallback(cb);sceKernelSleepThreadCB();return 0;}
static void note(const char *message){snprintf(notice,sizeof(notice),"%s",message);if(logfile){fprintf(logfile,"NOTICE %s\n",notice);fflush(logfile);}}
static void header(const char *title){
 label(16,10,"joesnose_player",ACCENT,10);int battery=scePowerGetBatteryLifePercent();char b[16];snprintf(b,sizeof(b),battery>=0?"%d%%":"AC",battery);
 rounded(221,8,35,18,8,PANEL);label(229,11,b,MUTED,10);label(16,36,title,INK,20);
}
static void footer(const char *a,const char *b){rect(0,435,272,45,PANEL);label(16,443,a,INK,10);label(16,461,b,MUTED,10);}
static void busy(const char *message){gfx_clear(BACK);header("Updating library");rounded(16,146,240,104,14,PANEL);label(32,168,message,INK,13);label(32,198,"Your music keeps playing.",MUTED,10);gfx_flip();}
static void card_path(char path[PATH_CAP],int canonical){
 const char *from=canonical?"fatms0:":direct_card?"ms0:":"fatms0:",*to=canonical?"ms0:":direct_card?"fatms0:":"ms0:";
 size_t n=strlen(from),m=strlen(to);if(strncasecmp(path,from,n))return;size_t tail=strlen(path+n);if(m+tail>=PATH_CAP)return;memmove(path+m,path+n,tail+1);memcpy(path,to,m);
}
static void save_roots(void){Roots saved=roots;for(int i=0;i<saved.count;i++)card_path(saved.roots[i],1);if(!roots_save(&saved,config))note("Could not save folder choices");}
static void rescan(void){
 busy("Scanning selected folders...");if(logfile){for(int i=0;i<roots.count;i++)fprintf(logfile,"SCAN_ROOT index=%d path=%s\n",i,roots.roots[i]);fflush(logfile);}Playback p=player_snapshot();
 if(!library_scan(&library,&roots)){note("Scan failed: not enough memory");return;}
 cover_thumbnails_clear();free(shuffle_order);shuffle_order=0;shuffle_enabled=0;free(album_indices);album_indices=0;size_t count=0;if(!library_folders(&library,&album_indices,&count))note("Not enough memory for folder list");album_count=(int)count;if(logfile){fprintf(logfile,"FOLDERS count=%d tracks=%u\n",album_count,(unsigned)library.count);fflush(logfile);}if(album_selected>=album_count)album_selected=0;library_revision++;playing_index=-1;for(size_t i=0;i<library.count;i++)if(!music_path_compare(library.paths[i],p.path))playing_index=(int)i;
 if(selected>=(int)library.count)selected=library.count?(int)library.count-1:0;
 char s[128];snprintf(s,sizeof(s),"%u tracks; %u skipped entries",(unsigned)library.count,library.skipped);note(s);
}
static void play_index(int index){
 if(!audio_ready){note("MP3 decoder unavailable - see log");return;}if(!library.count){note("Choose music folders with SELECT");return;}
 if(index<0)index=(int)library.count-1;if(index>=(int)library.count)index=0;folder_preview=0;browse_idle.armed=0;
 if(shuffle_enabled)for(size_t i=0;i<library.count;i++)if(shuffle_order[i]==index){shuffle_cursor=(int)i;break;}direct_playing[0]=0;playing_index=selected=index;if(logfile){fprintf(logfile,"PLAY_REQUEST index=%d path=%s\n",index,library.paths[index]);fflush(logfile);}player_play(library.paths[index]);
}
static void skip(int direction){if(shuffle_enabled&&library.count){int next=(shuffle_cursor+direction+(int)library.count)%(int)library.count;play_index(shuffle_order[next]);}else play_index(playing_index<0?0:playing_index+direction);}
static void shuffle_all(void){
 if(shuffle_enabled){shuffle_enabled=0;free(shuffle_order);shuffle_order=0;note("Shuffle off");return;}
 if(!library.count){note("No music to shuffle");return;}uint32_t seed=sceKernelGetSystemTimeLow();if(!library_shuffle(library.count,&seed,&shuffle_order)){note("Not enough memory to shuffle");return;}
 shuffle_enabled=1;shuffle_cursor=0;note("Shuffle all music");if(logfile){fprintf(logfile,"SHUFFLE count=%u\n",(unsigned)library.count);fflush(logfile);}play_index(shuffle_order[0]);page=NOW;
}
static void refresh_art(const Playback *p){
 if(!p->path[0]){if(shown[0]){shown[0]=0;tags_free(&tags);cover_load(0,0,"");}return;}if(!strcmp(shown,p->path))return;snprintf(shown,sizeof(shown),"%s",p->path);tags_free(&tags);tags_read(shown,&tags);cover_load(tags.art,tags.art_size,shown);free(tags.art);tags.art=0;tags.art_size=0;
}
static const char *selected_path(void){return selected>=0&&selected<(int)library.count?library.paths[selected]:0;}
static void select_music(int direction,int album){drum_offset=direction>0?22:-22;int index=library_browse(&library,selected,direction,album);if(index<0){note("Choose music folders with SELECT");return;}selected=index;browse_idle_touch(&browse_idle,sceKernelGetSystemTimeLow());if(album){folder_preview=1;snprintf(preview_art_path,sizeof(preview_art_path),"%s",library.paths[index]);}page=NOW;if(logfile){fprintf(logfile,"SELECT kind=%s index=%d path=%s\n",album?"folder":"track",index,library.paths[index]);fflush(logfile);}}
static void open_album(void){if(!album_count)return;selected=album_indices[album_selected];browse_idle_touch(&browse_idle,sceKernelGetSystemTimeLow());folder_preview=1;snprintf(preview_art_path,sizeof(preview_art_path),"%s",library.paths[selected]);page=NOW;if(logfile){fprintf(logfile,"OPEN_FOLDER index=%d track=%d preview=1\n",album_selected,selected);fflush(logfile);}}
static void play_selected(void){
 if(page==LIBRARY&&album_count){open_album();return;}
 folder_preview=0;browse_idle.armed=0;
 Playback p=player_snapshot();const char *choice=selected_path();
 if(choice){if(p.path[0]&&!music_path_compare(choice,p.path)&&(p.state==PLAYER_PLAYING||p.state==PLAYER_PAUSED))player_pause();else play_index(selected);}
 else if(direct_playing[0]){if(p.state==PLAYER_PLAYING||p.state==PLAYER_PAUSED)player_pause();else player_play(direct_playing);}
 else {note("Choose a track from your music folders");return;}page=NOW;
}
static void return_to_player(void){
 browse_idle.armed=0;Playback p=player_snapshot();if(p.path[0]){folder_preview=0;for(size_t i=0;i<library.count;i++)if(!music_path_compare(library.paths[i],p.path)){selected=(int)i;break;}}
 drum_offset=0;page=NOW;if(logfile){fprintf(logfile,"RETURN_PLAYER selected=%d preview=%d path=%s\n",selected,folder_preview,p.path);fflush(logfile);}
}
static void update_volume(unsigned raw,uint32_t now){
 int v=system_volume;if(read_volume&&(uint32_t)(now-volume_polled_at)>=50000u){int value=read_volume();volume_polled_at=now;if(value>=0&&value<=30)v=value;}
 unsigned keys=raw&(PSP_CTRL_VOLUP|PSP_CTRL_VOLDOWN);
 if(keys)stick_volume_overlay=0;
 if(keys||(last_volume>=0&&v>=0&&v!=last_volume)){
  volume_direction=keys==PSP_CTRL_VOLUP?(volume_reversed?-1:1):keys==PSP_CTRL_VOLDOWN?(volume_reversed?1:-1):v>last_volume?1:v<last_volume?-1:0;
  volume_visible=1;volume_shown_at=now;
 }
 if(v>=0)last_volume=v;system_volume=v;
}
static void draw_volume(uint32_t now){
 if(!volume_visible||(uint32_t)(now-volume_shown_at)>=1400000u)return;
 rounded(10,59,252,75,12,0xff090706);rounded(12,60,248,71,11,PANEL);
 label(24,72,stick_volume_overlay?"Player volume":volume_direction>0?"Volume +":volume_direction<0?"Volume -":"Volume",INK,13);
 int displayed_volume=stick_volume_overlay?player_snapshot().volume:system_volume;
 if(displayed_volume>=0){char value[24];snprintf(value,sizeof(value),"%d / 30",displayed_volume);label(244-label_width(value,13),72,value,ACCENT,13);rounded(24,108,224,7,3,0xff493f35);if(displayed_volume)rounded(24,108,224*displayed_volume/30,7,3,ACCENT);}
 else label(24,105,"System volume",MUTED,10);
}
static uint32_t fade_color(uint32_t c,unsigned amount){unsigned r=((c&255)*amount+(BACK&255)*(255-amount))/255,g=(((c>>8)&255)*amount+((BACK>>8)&255)*(255-amount))/255,b=(((c>>16)&255)*amount+((BACK>>16)&255)*(255-amount))/255;return 0xff000000u|r|(g<<8)|(b<<16);}
static void track_name(int index,char name[PATH_CAP]){snprintf(name,PATH_CAP,"%s",path_name(library.paths[index]));char *ext=strrchr(name,'.');if(ext)*ext=0;}
static void draw_now(const Playback *p){
 static Tags browsing_tags;static char browsing_path[PATH_CAP];static unsigned browsing_revision=~0u;
 const char *choice=selected_path();if(choice&&(strcmp(browsing_path,choice)||browsing_revision!=library_revision)){
  tags_free(&browsing_tags);tags_read(choice,&browsing_tags);free(browsing_tags.art);browsing_tags.art=0;browsing_tags.art_size=0;snprintf(browsing_path,sizeof(browsing_path),"%s",choice);browsing_revision=library_revision;
 }
 char folder[PATH_CAP]="Choose a music folder";if(choice){snprintf(folder,sizeof(folder),"%s",choice);char *slash=strrchr(folder,'/');if(slash)*slash=0;const char *name=path_name(folder);memmove(folder,name,strlen(name)+1);}
 uint32_t text_key=folder_preview?(0x80000000u|(uint32_t)selected):(uint32_t)p->generation;const char *artist=folder_preview&&choice?browsing_tags.artist:tags.artist;scrolling_label(2,16,322,artist[0]?artist:shown[0]?"Unknown artist":"",240,ACCENT,13,text_key);
 uint32_t now=sceKernelGetSystemTimeLow();const char *title=folder_preview&&choice?browsing_tags.title:shown[0]?tags.title:"Choose your music",*album=folder_preview&&choice?(browsing_tags.album[0]?browsing_tags.album:folder):tags.album[0]?tags.album:"Your music collection";
 if(strcmp(title,title_new)||strcmp(album,album_new)){snprintf(title_old,sizeof(title_old),"%s",title_new);snprintf(album_old,sizeof(album_old),"%s",album_new);snprintf(title_new,sizeof(title_new),"%s",title);snprintf(album_new,sizeof(album_new),"%s",album);title_changed=now;}
 label(16,10,"joesnose_player",ACCENT,10);char battery[16];int percent=scePowerGetBatteryLifePercent();snprintf(battery,sizeof(battery),percent>=0?"%d%%":"AC",percent);label(226,10,battery,MUTED,10);
 unsigned age=now-title_changed,alpha=age<140000?255-age*255/140000:age<340000?(age-140000)*255/200000:255;
 if(age<340000u){fitted(16,34,age<140000?title_old:title_new,240,fade_color(INK,alpha),20);fitted(16,61,age<140000?album_old:album_new,240,fade_color(ACCENT,alpha),13);}
 else {scrolling_label(0,16,34,title_new,240,INK,20,text_key);scrolling_label(1,16,61,album_new,240,ACCENT,13,text_key);}
 rounded(20,84,232,232,14,FRAME);cover_draw(24,88);
 if(music_theme==4){rounded(9,388,254,31,9,SECONDARY);}rounded(10,389,252,29,8,PANEL);rounded(10,395,3,17,1,ACCENT);
 static int indices[5]={-1,-1,-1,-1,-1},cached_selection=-2;static unsigned cached_revision=~0u;if(library.count&&(cached_selection!=selected||cached_revision!=library_revision)){cached_selection=selected;cached_revision=library_revision;indices[2]=selected;indices[1]=library_browse(&library,selected,-1,0);indices[0]=library_browse(&library,indices[1],-1,0);indices[3]=library_browse(&library,selected,1,0);indices[4]=library_browse(&library,indices[3],1,0);}
 for(int i=0;i<5;i++)if(indices[i]>=0&&indices[i]<(int)library.count){char name[PATH_CAP];track_name(indices[i],name);int distance=abs(i-2),x=22+distance*6,y=346+i*25+(int)drum_offset;if(y<343||y>449)continue;if(i==2)scrolling_label(3,x,y,name,264-2*x,ACCENT,16,(uint32_t)selected);else fitted(x,y,name,264-2*x,fade_color(INK,distance==2?85:150),distance==2?10:13);}
 drum_offset*=0.65f;if(drum_offset<0.5f&&drum_offset>-0.5f)drum_offset=0;
 char clock[40];snprintf(clock,sizeof(clock),"%02u:%02u",p->seconds/60,p->seconds%60);label(12,465,clock,MUTED,10);
 int bar_x=20+label_width(clock,10),bar_w=234-bar_x;rounded(bar_x,468,bar_w,4,2,PANEL);if(p->duration){unsigned seconds=p->seconds>p->duration?p->duration:p->seconds;int fill=(uint64_t)seconds*bar_w/p->duration;if(fill)rounded(bar_x,468,fill,4,2,ACCENT);}
 playback_icon(244,463,p->state==PLAYER_PLAYING);

}
static unsigned rotate_faces(unsigned b){unsigned mask=PSP_CTRL_CROSS|PSP_CTRL_CIRCLE|PSP_CTRL_TRIANGLE|PSP_CTRL_SQUARE;return (b&~mask)|((b&PSP_CTRL_CIRCLE)?PSP_CTRL_CROSS:0)|((b&PSP_CTRL_TRIANGLE)?PSP_CTRL_CIRCLE:0)|((b&PSP_CTRL_SQUARE)?PSP_CTRL_TRIANGLE:0)|((b&PSP_CTRL_CROSS)?PSP_CTRL_SQUARE:0);}
static void remember_position(void){
 char file[PATH_CAP];if(!path_join(file,base,"position.txt"))return;MusicSession saved={0};const char *choice=selected_path();if(choice)snprintf(saved.selected,sizeof(saved.selected),"%s",choice);Playback p=player_snapshot();snprintf(saved.playing,sizeof(saved.playing),"%s",p.path);saved.seconds=p.seconds;saved.paused=p.state==PLAYER_PAUSED;card_path(saved.selected,1);card_path(saved.playing,1);
 if(!session_save(&saved,file))note("Could not save player position");else if(logfile){fprintf(logfile,"POSITION_SAVED seconds=%u playing=%s\n",saved.seconds,saved.playing);fflush(logfile);}
}
static void restore_position(void){char file[PATH_CAP];MusicSession saved;if(!path_join(file,base,"position.txt")||!session_load(&saved,file))return;card_path(saved.selected,0);for(size_t i=0;i<library.count;i++)if(!music_path_compare(saved.selected,library.paths[i])){selected=(int)i;folder_preview=1;snprintf(preview_art_path,sizeof(preview_art_path),"%s",library.paths[i]);if(logfile){fprintf(logfile,"RESTORE_SELECTION index=%d path=%s\n",selected,saved.selected);fflush(logfile);}break;}}
static void row(int y,const char *name,const char *detail,int active,int kind){
 if(active){if(music_theme==4)rounded(9,y-5,254,36,8,SECONDARY);rounded(10,y-4,252,34,7,PANEL);rounded(10,y+2,3,22,1,ACCENT);}
 if(kind==1)folder_icon(21,y+3,active?ACCENT:MUTED);else if(kind==2)music_icon(21,y+3,active?ACCENT:MUTED);
 int x=kind?49:22;fitted(x,y,name,250-x,active?ACCENT:INK,13);if(detail&&detail[0])fitted(x,y+17,detail,250-x,MUTED,10);
}
static void scroll_marker(int selection,int count){if(count<=9)return;rect(267,100,2,300,PANEL);int h=300*9/count;if(h<12)h=12;int pos=(300-h)*selection/(count-1);rounded(267,100+pos,2,h,1,ACCENT);}
static void parent_name(const char *path,char out[PATH_CAP]){snprintf(out,PATH_CAP,"%s",path);char *slash=strrchr(out,'/');if(slash)*slash=0;const char *name=path_name(out);memmove(out,name,strlen(name)+1);}
static void draw_library(void){
 header("Library");char info[80];snprintf(info,sizeof(info),"%d folders / %u tracks",album_count,(unsigned)library.count);label(16,68,info,MUTED,10);
 label(16,84,shuffle_enabled?"Square  Shuffle all: ON":"Square  Shuffle all music",ACCENT,10);int top=album_selected>=6?album_selected-5:0;
 for(int i=0;i<6&&top+i<album_count;i++){int index=top+i,y=101+i*54;char dir[PATH_CAP];snprintf(dir,sizeof(dir),"%s",library.paths[album_indices[index]]);char *slash=strrchr(dir,'/');if(slash)*slash=0;
  if(index==album_selected){if(music_theme==4)rounded(9,y-4,254,52,8,SECONDARY);rounded(10,y-3,252,50,7,PANEL);rounded(10,y+7,3,29,1,ACCENT);}
  folder_thumbnail(17,y,dir,library.paths[album_indices[index]]);if(index==album_selected){scrolling_label(4,73,y+6,path_name(dir),181,ACCENT,13,(uint32_t)index);scrolling_label(5,73,y+25,dir,181,MUTED,10,(uint32_t)index);}else{fitted(73,y+6,path_name(dir),181,INK,13);fitted(73,y+25,dir,181,MUTED,10);}
 }
 if(album_count>6){int h=324*6/album_count;if(h<12)h=12;rect(267,100,2,324,PANEL);rounded(267,100+(324-h)*album_selected/(album_count-1),2,h,1,ACCENT);}
 if(!album_count){folder_icon(122,163,ACCENT);label(46,205,"No music found yet",INK,13);label(29,234,"SELECT to choose a music folder.",MUTED,10);}
 footer("HOME / O Open folder    Triangle Player","Square Shuffle all       SELECT Music folders");
}
static void draw_folders(void){
 header("Settings");fitted(16,68,storage_redirected&&!direct_card?"Card access unavailable; see log":notice[0]?notice:"Music folders and appearance",240,MUTED,10);int total=roots.count+3,top=folder_selected>=9?folder_selected-8:0;
 char theme_label[40];snprintf(theme_label,sizeof(theme_label),"Theme: %s",THEME.name);
 for(int i=0;i<9&&top+i<total;i++){int index=top+i;const char *name=index==0?"Add a music folder":index==1?"Refresh library":index==2?theme_label:roots.roots[index-3];const char *detail=index==0?"Internal storage or Memory Stick":index==1?"Look for new folders and tracks":index==2?"Circle cycles colours; saved automatically":"Includes every nested subfolder";row(101+i*36,name,detail,index==folder_selected,index>=3?1:0);}
 scroll_marker(folder_selected,total);footer("O Open / change theme     X Remove root","Triangle Refresh         Square Exact file");
}
static int open_direct_test(void){
 char path[PATH_CAP];if(!path_join(path,base,"DIRECT_TEST.txt")||!probe_load(path)){note("No valid DIRECT_TEST.txt found");return 0;}
 busy("Checking the exact file...");probe_run(logfile);return 1;
}
static void draw_direct(void){
 header("Exact file check");label(16,68,"Read-only test; folder scan is bypassed.",MUTED,10);
 fitted(16,96,path_name(file_probes[0].path),240,INK,13);
 char parent[PATH_CAP];parent_name(file_probes[0].path,parent);fitted(16,119,parent,240,MUTED,10);
 for(int i=0;i<2;i++){
  const FileProbe *p=&file_probes[i];int y=152+i*110;rounded(12,y,248,99,12,PANEL);label(24,y+10,!strncmp(p->path,"ms0:",4)?"Memory Stick / ms0:":"Internal storage / ef0:",INK,13);
  char status[64];if(p->open_rc<0)snprintf(status,sizeof(status),"Open failed: %08x",(unsigned)p->open_rc);else if(p->read_rc<=0)snprintf(status,sizeof(status),"Opened; read result: %d",p->read_rc);else snprintf(status,sizeof(status),"File opened and read (%d bytes)",p->read_rc);
  label(24,y+37,status,p->open_rc>=0&&p->read_rc>0?ACCENT:0xffa3a3ff,10);
  snprintf(status,sizeof(status),"In folder listing: %s",p->listed>0?"yes":p->listed==0?"no":"unavailable");label(24,y+59,status,MUTED,10);
  if(p->size>=0){snprintf(status,sizeof(status),"Size: %lld bytes",p->size);label(24,y+78,status,MUTED,10);}
 }
 const char *found=probe_playable();fitted(16,389,found?"O plays this file directly:":"No readable file at either location.",240,INK,10);if(found)fitted(16,410,found,240,ACCENT,10);
 footer("O Try playback             Square Retest","Triangle Folders              START Player");
}
static void free_dirs(void){for(int i=0;i<dir_count;i++)free(entries[i].name);free(entries);entries=0;dir_count=0;}
static int compare_dirs(const void *a,const void *b){const BrowserEntry *x=a,*y=b;if(x->directory!=y->directory)return y->directory-x->directory;return strcasecmp(x->name,y->name);}
static void list_dirs(void){
 free_dirs();browse_selected=0;browse_error[0]=0;browser_folders=browser_tracks=browser_other=0;if(!browser[0])return;MusicDir *d=music_dir_open(browser);if(!d){note("Cannot read this folder - see log");strcpy(browse_error,"This location could not be read.");return;}MusicEntry e;int rc=0;
 while(dir_count<TRACK_CAP&&(rc=music_dir_read(d,&e))>0){if(!strcmp(e.name,".")||!strcmp(e.name,"..")||e.link||(!e.directory&&!is_mp3(e.name)))continue;
  char *name=malloc(strlen(e.name)+1);if(!name){note("Not enough memory for folder list");strcpy(browse_error,"List incomplete: not enough memory.");break;}strcpy(name,e.name);BrowserEntry *next=realloc(entries,(dir_count+1)*sizeof(*next));if(!next){free(name);strcpy(browse_error,"List incomplete: not enough memory.");break;}entries=next;entries[dir_count++]=(BrowserEntry){name,e.directory};if(e.directory)browser_folders++;else if(is_mp3(e.name))browser_tracks++;else browser_other++;
 }if(rc<0)strcpy(browse_error,"Read error: some items may be missing.");if(dir_count>=TRACK_CAP)strcpy(browse_error,"List limit reached (20,000 items).");music_dir_close(d);if(dir_count)qsort(entries,dir_count,sizeof(*entries),compare_dirs);
 if(logfile){fprintf(logfile,"BROWSER path=%s entries=%d read=%08x\n",browser,dir_count,rc);fflush(logfile);}
}
static void draw_browser(void){
 header(browser[0]?"Browse music":"Choose storage");fitted(16,68,browser[0]?browser:"Your folders and MP3s, together.",240,MUTED,10);
 char summary[100];snprintf(summary,sizeof(summary),"%d folders / %d MP3s",browser_folders,browser_tracks);if(browser[0])fitted(16,84,browse_error[0]?browse_error:summary,240,browse_error[0]?0xffa3a3ff:MUTED,10);
 int count=browser[0]?dir_count:2,top=browse_selected>=9?browse_selected-8:0;
 for(int i=0;i<9&&top+i<count;i++){int index=top+i;
  const char *name=browser[0]?entries[index].name:(index==0?"PSP Go internal storage":"Memory Stick");
  const char *detail=browser[0]?(entries[index].directory?"Folder":is_mp3(entries[index].name)?"MP3 - press O to play":"Audio file - MP3 playback only"):(index==0?"ef0:/":"ms0:/");
  row(101+i*36,name,detail,index==browse_selected,browser[0]?(entries[index].directory?1:2):1);
 }scroll_marker(browse_selected,count);
 if(browser[0]&&!dir_count){label(29,180,"No folders or MP3s in this folder.",MUTED,10);label(29,201,"X still adds this location.",MUTED,10);}
 footer("O Open / play            X Use this folder","Triangle Parent               Square Done");
}
static void browser_up(void){
 size_t n=strlen(browser);while(n&&browser[n-1]=='/')browser[--n]=0;char *p=strrchr(browser,'/');
 char *colon=strchr(browser,':');
 if(p&&(!colon||p>colon)){p[1]=0;}else browser[0]=0;list_dirs();
}
static void *helper_function(unsigned nid){return sctrlHENFindFunction("JoeMusicButtons12","JoeMusicControls",nid);}
static void *make_syscall(unsigned slot,unsigned nid){void *fn=helper_function(nid);int number=fn?sctrlKernelQuerySystemCall(fn):-1;if(number<0||number>0xfffff)return 0;syscall_stubs[slot][0]=0x03e00008;syscall_stubs[slot][1]=((uint32_t)number<<6)|0x0c;return syscall_stubs[slot];}
static void helper_init(void){
 char path[PATH_CAP];struct stat st;
 if(path_join(path,base,"SAFE_MODE.txt")&&!stat(path,&st)){note("Safe mode: normal HOME and volume; Circle plays");return;}
 if(!path_join(path,base,"music_buttons.prx")||stat(path,&st)){note("Helper absent: Circle plays; normal HOME");return;}
 if(!helper_function(0xef3e192c)){
  if(logfile){fprintf(logfile,"STARTUP helper_file_load\n");fflush(logfile);}
  helper_uid=kuKernelLoadModule(path,0,0);if(logfile){fprintf(logfile,"HELPER load=%08x\n",helper_uid);fflush(logfile);}if(helper_uid<0)return;
  int status=0,rc=sceKernelStartModule(helper_uid,0,0,&status,0);if(logfile){fprintf(logfile,"HELPER start=%08x status=%08x\n",rc,status);fflush(logfile);}if(rc<0||status<0)return;
 }
 volume_read=(unsigned(*)(void))make_syscall(0,0xef3e192c);read_volume=(int(*)(void))make_syscall(1,0x4e6b867c);read_home_active=(int(*)(void))make_syscall(2,0xbd8417f5);
 sceKernelDcacheWritebackInvalidateRange(syscall_stubs,sizeof(syscall_stubs));sceKernelIcacheInvalidateRange(syscall_stubs,sizeof(syscall_stubs));read_slide=(int(*)(void))make_syscall(3,0x47936bba);sceKernelDcacheWritebackInvalidateRange(syscall_stubs,sizeof(syscall_stubs));sceKernelIcacheInvalidateRange(syscall_stubs,sizeof(syscall_stubs));helper_ready=volume_read&&read_volume&&read_home_active;volume_reversed=0;
 if(helper_ready){
  system_volume=read_volume();if(system_volume<0||system_volume>30)system_volume=-1;last_volume=system_volume;
  previous_popup=sceImposeGetHomePopup();int status=read_home_active();
  if(previous_popup>=0&&status>=0){int rc=sceImposeSetHomePopup(0);home_custom=rc>=0;}
  storage_redirected=sctrlKernelMsIsEf()>0;
 }
 if(logfile){fprintf(logfile,"HELPER controls=%d volume=%d home_status=%d custom_home=%d storage_redirected=%d reverse_disabled=1\n",volume_read!=0,read_volume!=0,read_home_active!=0,home_custom,storage_redirected);fflush(logfile);}
}
static int wait_home_menu(unsigned raw,uint32_t now){
 if(!menu_waiting)return 0;int active=read_home_active?read_home_active():0;if(active>0)menu_seen=1;
 if(active<=0&&!(raw&PSP_CTRL_HOME)&&(menu_seen||(uint32_t)(now-menu_requested_at)>2000000u)){
  sceImposeSetHomePopup(0);menu_waiting=0;return 2;
 }return 1;
}
static void open_home_menu(uint32_t now){
 int rc=sceImposeSetHomePopup(1);if(rc>=0)rc=sceImposeHomeButton();
 if(logfile){fprintf(logfile,"HOME_MENU request=%08x\n",rc);fflush(logfile);}
 if(rc<0){sceImposeSetHomePopup(previous_popup>=0?previous_popup:1);home_custom=0;note("Normal HOME controls restored");return;}
 menu_waiting=1;menu_seen=0;menu_requested_at=now;
}
int main(void){
 sceCtrlSetSamplingCycle(0);sceCtrlSetSamplingMode(PSP_CTRL_MODE_ANALOG);
 int cb=sceKernelCreateThread("music_callbacks",callback_thread,0x11,16384,PSP_THREAD_ATTR_USER,0);if(cb>=0)sceKernelStartThread(cb,0,0);
 if(!getcwd(base,sizeof(base)))strcpy(base,".");path_join(config,base,"folders.cfg");path_join(theme_file,base,"theme.cfg");theme_load(theme_file);remove("JOE_MUSIC_LOG.txt");logfile=fopen("JOE_MUSIC_LOG.txt","wb");if(logfile){fprintf(logfile,"BUILD=0.16 memory_request=normal audio_backend=standard44100 folder_preview=1 portrait=clockwise logical=272x480 base=%s\n",base);fflush(logfile);}
 music_fs_log(logfile);cover_log(logfile);gfx_init();busy("Starting MP3 decoder...");int rc=player_init();audio_ready=rc>=0;if(logfile){fprintf(logfile,"AUDIO init=%08x\n",rc);fflush(logfile);}busy("Starting HOME controls...");helper_init();busy("Checking music storage...");
 if(storage_redirected){direct_card=music_is_dir("fatms0:/");if(direct_card)card_root="fatms0:/";if(logfile){fprintf(logfile,"CARD_BYPASS available=%d prefix=%s\n",direct_card,card_root);fflush(logfile);}}
 roots_load(&roots,config);for(int i=0;i<roots.count;i++)card_path(roots.roots[i],0);
 const char *standard[]={"ef0:/MUSIC","ms0:/MUSIC","ef0:/PSP/MUSIC","ms0:/PSP/MUSIC"};
 for(unsigned i=0;i<sizeof(standard)/sizeof(*standard);i++){char path[PATH_CAP];snprintf(path,sizeof(path),"%s",standard[i]);card_path(path,0);int found=music_is_dir(path);if(logfile)fprintf(logfile,"STANDARD_LOCATION path=%s available=%d\n",path,found);}
 if(!roots.defaults_checked){int added=0;for(unsigned i=0;i<4;i++){char path[PATH_CAP];snprintf(path,sizeof(path),"%s",standard[i]);card_path(path,0);if(music_is_dir(path)&&root_add(&roots,path)>=0)added++;}roots.defaults_checked=1;save_roots();if(logfile){fprintf(logfile,"ROOTS_MIGRATED added=%d selected=%d\n",added,roots.count);fflush(logfile);}}
 rescan();restore_position();page=NOW;SlidePanel panel;slide_panel_init(&panel);int saved_selected=selected;uint32_t selection_changed=0,position_saved_at=sceKernelGetSystemTimeLow();
 int logged_state=-1;unsigned logged_second=~0u;
 int stick_last=0;uint32_t stick_at=0;
 unsigned last=0,last_nav=0;uint32_t nav_since=0;int nav_repeated=0;Press left={0},right={0},home={0};uint32_t last_left=0,last_right=0;int left_armed=0,right_armed=0;
 while(running){
  SceCtrlData pad;memset(&pad,0,sizeof(pad));pad.Lx=pad.Ly=128;sceCtrlPeekBufferPositive(&pad,1);unsigned buttons=rotate_faces(pad.Buttons),pressed=buttons&~last;last=buttons;uint32_t now=sceKernelGetSystemTimeLow();
  unsigned raw=volume_read?volume_read():buttons;
  int menu_state=wait_home_menu(raw,now);if(menu_state){if(menu_state==2){memset(&left,0,sizeof(left));memset(&right,0,sizeof(right));memset(&home,0,sizeof(home));press_cancel(&left);press_cancel(&right);press_cancel(&home);left_armed=right_armed=0;last=buttons;last_nav=0;}sceDisplayWaitVblankStart();continue;}
  if(read_slide&&(uint32_t)(now-slide_polled)>=100000u){int state=read_slide();slide_polled=now;if(state!=slide_state){slide_state=state;if(state==1)page=LIBRARY;else if(state==0){return_to_player();pressed=0;buttons=0;press_cancel(&left);press_cancel(&right);press_cancel(&home);left_armed=right_armed=0;}if(logfile){fprintf(logfile,"SLIDE state=%s\n",state==1?"open":state==0?"closed":"unknown");fflush(logfile);}}}
  update_volume(raw,now);
  /* Portrait up = physical left; portrait right = physical up. Ignore diagonals near the dead zone. */
  int ax=(int)pad.Lx-128,ay=(int)pad.Ly-128,stick=0;if(abs(ax)>64||abs(ay)>64)stick=abs(ax)>=abs(ay)?(ax<0?1:2):(ay<0?3:4);
  if(stick&&(stick!=stick_last||(uint32_t)(now-stick_at)>=(stick<=2?150000u:500000u))){stick_at=now;if(stick<=2){stick_volume_overlay=1;player_volume(stick==1?1:-1);volume_visible=1;volume_shown_at=now;volume_direction=stick==1?1:-1;if(logfile){fprintf(logfile,"STICK_VOLUME value=%d\n",player_snapshot().volume);fflush(logfile);}}else if(page==NOW){player_seek(stick==3?5:-5);if(logfile){fprintf(logfile,"SEEK delta=%d from=%u\n",stick==3?5:-5,player_snapshot().seconds);fflush(logfile);}}}
  stick_last=stick;
  if(home_custom){int event=press_event(&home,!!(raw&PSP_CTRL_HOME),now,800000);if(event==PRESS_LONG){open_home_menu(now);continue;}if(event==PRESS_SHORT){if(logfile)fprintf(logfile,"HOME_SHORT play_selection\n");play_selected();}}

  if((buttons&(PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER))==(PSP_CTRL_LTRIGGER|PSP_CTRL_RTRIGGER)){press_cancel(&left);press_cancel(&right);left_armed=right_armed=0;}
  int le=press_event(&left,!!(buttons&PSP_CTRL_LTRIGGER),now,800000),re=press_event(&right,!!(buttons&PSP_CTRL_RTRIGGER),now,800000);
  if(double_tap(&last_left,&left_armed,now,le))skip(-1);else if(le)select_music(-1,le==PRESS_LONG);if(double_tap(&last_right,&right_armed,now,re))skip(1);else if(re)select_music(1,re==PRESS_LONG);
  if(pressed&PSP_CTRL_SELECT){page=FOLDERS;folder_selected=0;}
  if(pressed&PSP_CTRL_START){if(page==NOW)page=LIBRARY;else return_to_player();}
  /* Physical LEFT points up after rotating the PSP clockwise. */
  unsigned nav=buttons&(page==NOW?(PSP_CTRL_LEFT|PSP_CTRL_RIGHT|PSP_CTRL_UP|PSP_CTRL_DOWN):(PSP_CTRL_LEFT|PSP_CTRL_RIGHT));int move=0;
  if(nav!=last_nav){nav_since=now;nav_repeated=0;move=!!nav;last_nav=nav;}
  else if(nav&&(uint32_t)(now-nav_since)>=(nav_repeated?100000u:350000u)){move=1;nav_repeated=1;nav_since=now;}
  int delta=(nav&PSP_CTRL_RIGHT)?1:-1;
  if(move&&page==NOW){if(nav&(PSP_CTRL_LEFT|PSP_CTRL_RIGHT))select_music(delta,0);else select_music((nav&PSP_CTRL_UP)?1:-1,1);}
  else if(move){int *selection=page==LIBRARY?&album_selected:page==FOLDERS?&folder_selected:&browse_selected;int count=page==LIBRARY?album_count:page==FOLDERS?roots.count+3:page==BROWSE?(browser[0]?dir_count:2):0;
   if(count){*selection+=delta;if(*selection<0)*selection=count-1;if(*selection>=count)*selection=0;}}
  if(page==NOW){if(pressed&PSP_CTRL_CROSS)play_selected();if(pressed&PSP_CTRL_SQUARE)player_stop();if(pressed&PSP_CTRL_CIRCLE)page=LIBRARY;}
  else if(page==LIBRARY){if(pressed&PSP_CTRL_TRIANGLE)shuffle_all();if((pressed&PSP_CTRL_CROSS)&&album_count)open_album();if(pressed&PSP_CTRL_CIRCLE)return_to_player();}
  else if(page==FOLDERS){
   if((pressed&PSP_CTRL_TRIANGLE)&&open_direct_test())page=DIRECT;
   if(pressed&PSP_CTRL_CROSS){if(folder_selected==0){browser[0]=0;list_dirs();page=BROWSE;}else if(folder_selected==1)rescan();else if(folder_selected==2){music_theme=(music_theme+1)%THEME_COUNT;shown[0]=0;cover_thumbnails_clear();if(!theme_save(theme_file))note("Theme changed; could not save");else note(THEME.name);if(logfile){fprintf(logfile,"THEME name=%s\n",THEME.name);fflush(logfile);}}else{snprintf(browser,sizeof(browser),"%s",roots.roots[folder_selected-3]);list_dirs();page=BROWSE;}}
   if((pressed&PSP_CTRL_SQUARE)&&folder_selected>=3&&folder_selected<roots.count+3){int i=folder_selected-3;memmove(roots.roots[i],roots.roots[i+1],(roots.count-i-1)*sizeof(roots.roots[0]));roots.count--;save_roots();if(folder_selected>=roots.count+3)folder_selected=roots.count+2;note("Folder removed; Triangle rescans");}
   if(pressed&PSP_CTRL_CIRCLE){rescan();page=LIBRARY;}
  }else if(page==DIRECT){
   if(pressed&PSP_CTRL_CIRCLE)page=FOLDERS;
   if(pressed&PSP_CTRL_TRIANGLE)open_direct_test();
   if(pressed&PSP_CTRL_CROSS){const char *path=probe_playable();if(path&&audio_ready){playing_index=-1;folder_preview=0;snprintf(direct_playing,sizeof(direct_playing),"%s",path);player_play(path);page=NOW;if(logfile){fprintf(logfile,"DIRECT_PLAY path=%s\n",path);fflush(logfile);}}}
  }else if(page==BROWSE){
   if(pressed&PSP_CTRL_CROSS){if(!browser[0]){snprintf(browser,sizeof(browser),"%s",browse_selected?card_root:"ef0:/");list_dirs();}else if(browse_selected<dir_count){char path[PATH_CAP];if(path_join(path,browser,entries[browse_selected].name)){
 if(entries[browse_selected].directory){strcpy(browser,path);list_dirs();}
 else if(!is_mp3(entries[browse_selected].name)){strcpy(browse_error,"Only MP3 playback is supported so far.");}
 else if(root_add(&roots,browser)>=0){save_roots();rescan();for(size_t i=0;i<library.count;i++)if(!music_path_compare(library.paths[i],path)){play_index((int)i);page=NOW;break;}}
 }}}
   if(pressed&PSP_CTRL_CIRCLE)browser_up();
   if((pressed&PSP_CTRL_SQUARE)&&browser[0]){if(root_add(&roots,browser)>=0){save_roots();note("Folder added with all subfolders");page=FOLDERS;}else note("Folder limit reached (16)");}
   if(pressed&PSP_CTRL_TRIANGLE)page=FOLDERS;
  }
  Playback p=player_snapshot();if(logfile&&(p.state!=logged_state||p.seconds!=logged_second)){fprintf(logfile,"AUDIO state=%d seconds=%u rate=%d bitrate=%d error=%08x generation=%d duration=%u volume=%d\n",p.state,p.seconds,p.rate,p.bitrate,(unsigned)p.error,p.generation,p.duration,p.volume);fflush(logfile);logged_state=p.state;logged_second=p.seconds;}if(p.state==PLAYER_ENDED&&p.generation!=ended_generation){ended_generation=p.generation;if(library.count&&playing_index>=0){int keep=selected,preview=folder_preview,pending=preview||selected!=playing_index;BrowseIdle saved_idle=browse_idle;skip(1);if(pending){selected=keep;browse_idle=saved_idle;}folder_preview=preview;}}
  if(page==NOW&&p.path[0]&&(p.state==PLAYER_PLAYING||p.state==PLAYER_PAUSED)){
   const char *choice=selected_path();int pending=folder_preview||!choice||music_path_compare(choice,p.path),reading=0;
   if(pending&&choice){char name[PATH_CAP];track_name(selected,name);if(label_width(name,16)>220)reading=!scrolling_label_read(3,name,(uint32_t)selected,sceKernelGetSystemTimeLow());}
   if(!reading&&browse_idle_due(&browse_idle,sceKernelGetSystemTimeLow())&&pending){if(logfile){fprintf(logfile,"SCROLL_IDLE_RETURN seconds=4\n");fflush(logfile);}return_to_player();}
  }
  if(p.state==PLAYER_PLAYING)scePowerTick(PSP_POWER_TICK_SUSPEND);
  if(selected!=saved_selected){saved_selected=selected;selection_changed=now;}if(selection_changed&&(uint32_t)(now-selection_changed)>700000u){remember_position();position_saved_at=now;selection_changed=0;}
  if((p.state==PLAYER_PLAYING||p.state==PLAYER_PAUSED)&&(uint32_t)(now-position_saved_at)>=10000000u){remember_position();position_saved_at=now;}
  Playback visual=p;if(folder_preview&&preview_art_path[0])snprintf(visual.path,sizeof(visual.path),"%s",preview_art_path);refresh_art(&visual);slide_panel_request(&panel,page==LIBRARY,now);int panel_x=slide_panel_offset(&panel,now);gfx_clear(BACK);
  if(page==LIBRARY&&panel_x==0)draw_library();
  else {if(page==NOW||page==LIBRARY)draw_now(&p);else if(page==FOLDERS)draw_folders();else if(page==DIRECT)draw_direct();else draw_browser();
   if(panel_x<272){if(gfx_panel_begin()){draw_library();gfx_panel_end(panel_x);}else if(page==LIBRARY)draw_library();}}
  draw_volume(now);gfx_flip();

 }
 remember_position();if(home_custom||menu_waiting)sceImposeSetHomePopup(previous_popup>=0?previous_popup:1);volume_read=0;read_volume=0;read_home_active=0;read_slide=0;helper_ready=0;player_shutdown();if(helper_uid>=0){int status;int rc=sceKernelStopModule(helper_uid,0,0,&status,0);if(rc>=0&&status>=0)sceKernelUnloadModule(helper_uid);}
 free(shuffle_order);free(album_indices);free_dirs();library_free(&library);tags_free(&tags);if(logfile){fprintf(logfile,"EXIT clean\n");fclose(logfile);}sceKernelExitGame();return 0;
}
