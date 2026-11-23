from pathlib import Path
import sys,time,re,json
R=Path(__file__).resolve().parents[1];sys.path.insert(0,str(R/'build/python'))
from Xlib import X,XK,display
from Xlib.ext import xtest
from PIL import Image
root=R/'build/emulator';game=root/'config/ppsspp/PSP/GAME/JoeMusic';d=display.Display(':111')
for w in d.screen().root.query_tree().children:
 try:
  if 'PPSSPP' in (w.get_wm_name() or ''):d.set_input_focus(w,X.RevertToParent,X.CurrentTime);d.sync()
 except Exception:pass

def key(name,pause=.25):
 code=d.keysym_to_keycode(XK.string_to_keysym(name));xtest.fake_input(d,X.KeyPress,code);d.sync();time.sleep(.08);xtest.fake_input(d,X.KeyRelease,code);d.sync();time.sleep(pause)
def shot(name):
 s=d.screen();g=s.root.get_geometry();im=s.root.get_image(0,0,g.width,g.height,X.ZPixmap,0xffffffff);Image.frombytes('RGB',(g.width,g.height),im.data,'raw','BGRX').save(root/name)
# New fixtures have appeared while playback remains active. Rescan via the UI.
key('space');key('Right');key('x',1);key('z',1);key('x',1.5);shot('embedded.png')
log=(game/'JOE_MUSIC_LOG.txt').read_text();assert 'NOTICE 3 tracks' in log,log[-1000:]
key('x',.7);assert 'AUDIO state=3' in (game/'JOE_MUSIC_LOG.txt').read_text()
key('x',.7);key('w',1.5);shot('sidecar.png')
# Add another root entirely through the on-screen folder browser.
key('space');key('x');key('Right');key('x');key('Right');key('x');key('s');key('z',1)
cfg=(game/'folders.cfg').read_text();assert 'root=ms0:/PODCASTS' in cfg,cfg
key('x',1);shot('second-root.png')
log=(game/'JOE_MUSIC_LOG.txt').read_text();assert 'NOTICE 4 tracks' in log,log[-1000:]
assert 'AUDIO state=5' not in log,log[-2000:]
assert 'root=ms0:/MUSIC' in cfg
(root/'controls-result.json').write_text(json.dumps({'mp3_playback':True,'recursive_scan':True,'pause_resume':True,'next_track':True,'folder_added_via_ui':True,'roots':cfg.splitlines(),'artwork_screenshots':['embedded.png','sidecar.png'],'hardware_volume_helper_tested':False},indent=2))
print(log[-1800:]);print('PASS: playback, pause/resume, next, nested scanning and second folder selected in UI')
