#define GFX_CACHE_HOST_TEST
#include "../src/gfx.c"
#include <dirent.h>
static void write_file(const char *p){FILE *f=fopen(p,"wb");assert(f);fputs("track",f);fclose(f);}
static void show(unsigned id,int moving){cover_library_activity(1,moving,sceKernelGetSystemTimeLow());test_pixel=0;folder_thumbnail(0,0,id,1);}
static uint16_t color(unsigned id){return (uint16_t)(id*53+0x1234);}
static void await_image(unsigned id){unsigned start=sceKernelGetSystemTimeLow();do{show(id,0);if(test_pixel==thumb_unpack(color(id)))return;usleep(2000);}while((unsigned)(sceKernelGetSystemTimeLow()-start)<3000000u);assert(!"thumbnail load timed out");}
static void remove_cached(const char *cache,const char *folder){DIR *d=opendir(cache);assert(d);struct dirent *e;char path[PATH_CAP];while((e=readdir(d)))if(strstr(e->d_name,".thumb")){assert(path_join(path,cache,e->d_name));FILE *f=fopen(path,"rb");assert(f);char saved[PATH_CAP];fseek(f,12,SEEK_SET);assert(fread(saved,1,sizeof(saved),f)==sizeof(saved));fclose(f);if(!strcmp(saved,folder)){assert(!remove(path));break;}}closedir(d);}
int main(void){
 enum{COUNT=1200};char root[]="/tmp/portrait-cache-life-XXXXXX";assert(mkdtemp(root));char cache[PATH_CAP];assert(path_join(cache,root,"cover-cache-v1"));assert(!mkdir(cache,0700));char **paths=calloc(COUNT,sizeof(*paths));int *indices=malloc(COUNT*sizeof(*indices));assert(paths&&indices);uint16_t encoded[THUMB_PIXELS];
 for(unsigned i=0;i<COUNT;i++){char folder[PATH_CAP],name[30],track[PATH_CAP];snprintf(name,sizeof(name),"album%04u",i);assert(path_join(folder,root,name)&&!mkdir(folder,0700));assert(path_join(track,folder,"song.mp3"));write_file(track);paths[i]=strdup(track);assert(paths[i]);indices[i]=i;for(unsigned j=0;j<THUMB_PIXELS;j++)encoded[j]=color(i);assert(thumb_store_save(cache,folder,track,track,PANEL,encoded));}
 cover_cache_setup(root);cover_worker_init();cover_thumbnails_prepare(paths,indices,COUNT);for(unsigned i=0;i<100;i++)await_image(i);
 cover_library_activity(1,1,sceKernelGetSystemTimeLow());usleep(50000);unsigned before,disk,after,disk_after;cover_cache_stats(&before,&disk);show(0,1);assert(test_pixel==thumb_unpack(color(0)));cover_cache_stats(&after,&disk_after);assert(before==after&&disk==disk_after);
 for(unsigned i=100;i<COUNT;i++)await_image(i);unsigned retained,bytes,saved,errors;cover_cache_details(&retained,&bytes,&saved,&errors);assert(bytes<=4u*1024u*1024u&&retained>=1000&&retained<COUNT);await_image(0);assert(!test_decodes);
 for(unsigned i=0;i<20;i++){cover_thumbnails_prepare(paths,indices,COUNT);show(i,0);usleep(1000);}await_image(0);
 /* A failed write must keep its completed image and retry when idle, even after leaving Library. */
 cover_library_activity(0,0,sceKernelGetSystemTimeLow());usleep(50000);char folder[PATH_CAP];strcpy(folder,paths[0]);*strrchr(folder,'/')=0;remove_cached(cache,folder);strcpy(test_decode_path,paths[0]);test_decode_color=color(0);assert(!chmod(cache,0500));cover_thumbnails_prepare(paths,indices,COUNT);await_image(0);
 unsigned start=sceKernelGetSystemTimeLow();do{cover_cache_details(&retained,&bytes,&saved,&errors);if(errors)break;usleep(5000);}while((unsigned)(sceKernelGetSystemTimeLow()-start)<2000000u);assert(errors&&retained&&test_decodes==1);assert(!chmod(cache,0700));
 for(unsigned i=0;i<550;i++){cover_library_activity(0,1,sceKernelGetSystemTimeLow());usleep(10000);}cover_cache_details(&retained,&bytes,&saved,&errors);assert(!saved);cover_library_activity(0,0,sceKernelGetSystemTimeLow());usleep(200000);
 start=sceKernelGetSystemTimeLow();do{cover_cache_details(&retained,&bytes,&saved,&errors);if(saved)break;usleep(5000);}while((unsigned)(sceKernelGetSystemTimeLow()-start)<2000000u);assert(saved==1);test_decode_path[0]=0;cover_thumbnails_prepare(paths,indices,COUNT);await_image(0);assert(test_decodes==1);
 cover_library_activity(0,1,sceKernelGetSystemTimeLow());usleep(50000);cover_thumbnails_prepare(paths,indices,COUNT);
 uint32_t *full=malloc(224*224*sizeof(*full));assert(full);for(unsigned i=0;i<224*224;i++)full[i]=thumb_unpack(color(0));cover_seed(paths[0],paths[0],full);free(full);show(0,1);assert(test_pixel==thumb_unpack(color(0))&&test_decodes==1);cover_shutdown();
 DIR *d=opendir(cache);assert(d);struct dirent *e;while((e=readdir(d)))if(strcmp(e->d_name,".")&&strcmp(e->d_name,"..")){char p[PATH_CAP];assert(path_join(p,cache,e->d_name));remove(p);}closedir(d);rmdir(cache);
 for(unsigned i=0;i<COUNT;i++){strcpy(folder,paths[i]);*strrchr(folder,'/')=0;remove(paths[i]);rmdir(folder);free(paths[i]);}free(paths);free(indices);rmdir(root);
 puts("PASS: 100 retained covers without reload, 1200-album bounded memory, concurrent rescan, queued write failure/retry, decoded-cover reuse and warm reuse");return 0;
}
