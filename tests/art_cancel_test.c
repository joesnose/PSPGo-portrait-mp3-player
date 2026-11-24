#include "core.h"
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
static int calls;static int cancel(void){return ++calls<2;}
int main(void){char path[]="/tmp/portrait-tags-XXXXXX";int fd=mkstemp(path);assert(fd>=0);close(fd);FILE *f=fopen(path,"wb");assert(f);unsigned size=40000;unsigned char header[10]={'I','D','3',3,0,0,0,0,0,0};for(int i=0;i<4;i++)header[9-i]=(unsigned char)((size>>(7*i))&127);unsigned char *body=calloc(size,1);assert(body);memcpy(body,"TIT2",4);body[7]=5;memcpy(body+11,"Tune",4);assert(fwrite(header,1,10,f)==10&&fwrite(body,1,size,f)==size);fclose(f);free(body);
 Tags t;assert(!tags_read_interruptible(path,&t,cancel));assert(calls==2&&!t.art);tags_free(&t);tags_read(path,&t);assert(!strcmp(t.title,"Tune"));tags_free(&t);remove(path);puts("PASS: interrupted large ID3 read cleans up and the normal tag reader retains its result");}
