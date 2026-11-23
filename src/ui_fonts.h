#ifndef UI_FONTS_H
#define UI_FONTS_H
#include <stdint.h>
typedef struct {uint32_t offset;unsigned char width,advance;} UiGlyph;
typedef struct {int size,height;const UiGlyph *glyphs;const unsigned char *pixels;} UiFont;
extern const UiFont ui_fonts[4];
#endif
