from pathlib import Path
import sys,time,json
r=Path(__file__).resolve().parents[1];root=r/'build/emulator-v013';sys.path.insert(0,str(r/'build/python'))
from Xlib import X,XK,display
from Xlib.ext import xtest
from PIL import Image
D=display.Display(':113')
for w in D.screen().root.query_tree().children:
 try:
  if 'PPSSPP' in (w.get_wm_name() or ''):D.set_input_focus(w,X.RevertToParent,X.CurrentTime);D.sync()
 except Exception:pass
code=D.keysym_to_keycode(XK.string_to_keysym('Return'))
def snap():
 s=D.screen();g=s.root.get_geometry();a=s.root.get_image(0,0,g.width,g.height,X.ZPixmap,0xffffffff);return Image.frombytes('RGB',(g.width,g.height),a.data,'raw','BGRX').crop((160,164,640,436)).rotate(-90,expand=True)
start=snap();xtest.fake_input(D,X.KeyPress,code);D.sync();frames=[]
for i in range(45):frames.append(snap());time.sleep(.02)
xtest.fake_input(D,X.KeyRelease,code);D.sync();time.sleep(.5);end=snap()
# The middle of the screen must contain a vertical split with player at left
# and opaque library panel at right, rather than a whole-screen fade/cut.
found=[]
for i,f in enumerate(frames):
 if f.crop((0,100,20,400)).tobytes()==start.crop((0,100,20,400)).tobytes() and f.crop((252,100,272,400)).tobytes()==end.crop((252,100,272,400)).tobytes() and f.crop((0,32,272,400)).tobytes()!=start.crop((0,32,272,400)).tobytes() and f.crop((0,32,272,400)).tobytes()!=end.crop((0,32,272,400)).tobytes():found.append(i)
# Save several distinct frames for visual review even if edge regions share BG.
distinct=[];last=None
for i,f in enumerate(frames):
 b=f.crop((0,32,272,400)).tobytes()
 if b!=last:distinct.append(i);last=b
for i in distinct:frames[i].save(root/f'panel-frame-{i:02}.png')
print(json.dumps({'distinct_frame_indices':distinct,'split_frame_candidates':found}))
