from PIL import Image,ImageDraw,ImageFont
from pathlib import Path
root=Path(__file__).resolve().parents[1]
header='''/* Generated bitmap glyphs from DejaVu Sans; see licenses/DejaVu-font-LICENSE.txt. */
#include "ui_fonts.h"
'''
out=header;fonts=[]
for size in (10,13,16,20):
 font=ImageFont.truetype('/usr/share/fonts/truetype/dejavu/DejaVuSans'+('-Bold' if size==20 else '')+'.ttf',size,layout_engine=ImageFont.Layout.BASIC)
 data=[];glyphs=[];height=size+5
 for code in range(256):
  ch=chr(code) if 32<=code<127 or code>=160 else '?'
  advance=int(font.getlength(ch)+.7);width=advance+1
  im=Image.new('L',(width,height));ImageDraw.Draw(im).text((0,-2),ch,font=font,fill=255)
  glyphs.append((len(data),width,advance));data.extend(im.tobytes())
 out+=f'static const unsigned char pixels_{size}[]={{'+','.join(map(str,data))+'};\n'
 out+=f'static const UiGlyph glyphs_{size}[]={{'+','.join('{%d,%d,%d}'%g for g in glyphs)+'};\n'
 fonts.append('{%d,%d,glyphs_%d,pixels_%d}'%(size,height,size,size))
out+='const UiFont ui_fonts[4]={'+','.join(fonts)+'};\n'
(root/'src/ui_fonts.c').write_text(out)
