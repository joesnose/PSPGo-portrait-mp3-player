#ifndef MUSIC_MP3_INFO_H
#define MUSIC_MP3_INFO_H
#include <stddef.h>
/* Duration from Xing/Info or VBRI in the first audio frame, zero if absent. */
unsigned mp3_header_duration(const unsigned char *data,size_t size);
#endif
