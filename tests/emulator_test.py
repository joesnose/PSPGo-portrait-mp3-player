from pathlib import Path
import os,subprocess,time,sys,configparser,json
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'build/python'))
from Xlib import X,XK,display
from Xlib.ext import xtest
from PIL import Image
R=Path(__file__).resolve().parents[1];root=R/'build/emulator';system=root/'config/ppsspp/PSP/SYSTEM';game=root/'config/ppsspp/PSP/GAME/JoeMusic'
cfg=configparser.ConfigParser();cfg.optionxform=str
settings={'General':{'FirstRun':'False','AutoRun':'True','AutoLoadSaveState':'0','IgnoreBadMemAccess':'False','EnableCheats':'False'},'CPU':{'CPUCore':'1','FastMemoryAccess':'False'},'SystemParam':{'PSPModel':'1'},'Graphics':{'FrameSkip':'0','ShowFPSCounter':'0','InternalResolution':'1'}}
for s,v in settings.items():cfg[s]=v
with (system/'ppsspp.ini').open('w') as f:cfg.write(f)
(system/'controls.ini').write_text('[ControlMapping]\nUp = 1-19\nDown = 1-20\nLeft = 1-21\nRight = 1-22\nCross = 1-52\nCircle = 1-54\nSquare = 1-47\nTriangle = 1-29\nL = 1-45\nR = 1-51\nStart = 1-66\nSelect = 1-62\n')
env=os.environ.copy();env.update(DISPLAY=':111',XDG_CONFIG_HOME=str(root/'config'),XDG_CACHE_HOME=str(root/'cache'),SDL_AUDIODRIVER='dummy',LIBGL_ALWAYS_SOFTWARE='1')
out=(root/'console.log').open('w');p=subprocess.Popen(['/workspace/work/squashfs-root/AppRun','--windowed','--xres','480','--yres','272','--graphics=software','--log='+str(root/'ppsspp.log'),str(game/'EBOOT.PBP')],env=env,cwd=game,stdout=out,stderr=subprocess.STDOUT,start_new_session=True)
(root/'pid').write_text(str(p.pid));time.sleep(3)
d=display.Display(':111');windows=d.screen().root.query_tree().children
for w in windows:
 try:
  if 'PPSSPP' in (w.get_wm_name() or ''):d.set_input_focus(w,X.RevertToParent,X.CurrentTime);d.sync()
 except Exception:pass

def key(name):
 code=d.keysym_to_keycode(XK.string_to_keysym(name));xtest.fake_input(d,X.KeyPress,code);d.sync();time.sleep(.12);xtest.fake_input(d,X.KeyRelease,code);d.sync()
def shot(name):
 s=d.screen();g=s.root.get_geometry();im=s.root.get_image(0,0,g.width,g.height,X.ZPixmap,0xffffffff);Image.frombytes('RGB',(g.width,g.height),im.data,'raw','BGRX').save(root/name)
shot('library.png');key('x');time.sleep(4);shot('playing.png')
print('EMULATOR_PID',p.pid)
log=game/'JOE_MUSIC_LOG.txt';print(log.read_text() if log.exists() else 'NO APP LOG')
