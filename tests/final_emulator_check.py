from pathlib import Path
import os,signal,shutil,subprocess,time,sys,re,json,hashlib
R=Path(__file__).resolve().parents[1];root=R/'build/emulator';game=root/'config/ppsspp/PSP/GAME/JoeMusic'
# Stop only the instance started by this project's harness; PPSSPP may defer SIGTERM.
pid=int((root/'pid').read_text())
try:os.kill(pid,signal.SIGKILL)
except ProcessLookupError:pass
shutil.copyfile(R/'EBOOT.PBP',game/'EBOOT.PBP')
subprocess.run([sys.executable,str(R/'tests/emulator_test.py')],check=True)
sys.path.insert(0,str(R/'build/python'))
from Xlib import X,XK,display
from Xlib.ext import xtest
from PIL import Image
d=display.Display(':111')
for w in d.screen().root.query_tree().children:
 try:
  if 'PPSSPP' in (w.get_wm_name() or ''):d.set_input_focus(w,X.RevertToParent,X.CurrentTime);d.sync()
 except Exception:pass

def key(name):
 code=d.keysym_to_keycode(XK.string_to_keysym(name));xtest.fake_input(d,X.KeyPress,code);d.sync();time.sleep(.1);xtest.fake_input(d,X.KeyRelease,code);d.sync();time.sleep(.8)
key('x');key('x');key('w');key('q')
log=game/'JOE_MUSIC_LOG.txt';started=time.monotonic()
while time.monotonic()-started<30:
 s=log.read_text()
 if 'AUDIO state=5' in s:raise RuntimeError(s)
 if 'AUDIO state=4' in s and len(re.findall('AUDIO state=2 seconds=0',s))>=4:break
 time.sleep(.2)
s=log.read_text();assert 'NOTICE 4 tracks' in s,s;assert 'AUDIO state=3' in s,s;assert 'AUDIO state=4' in s,s;assert 'AUDIO state=5' not in s,s
screen=d.screen();g=screen.root.get_geometry();im=screen.root.get_image(0,0,g.width,g.height,X.ZPixmap,0xffffffff);Image.frombytes('RGB',(g.width,g.height),im.data,'raw','BGRX').save(root/'final-screen.png')
result={'eboot_sha256':hashlib.sha256((R/'EBOOT.PBP').read_bytes()).hexdigest(),'ppsspp':'1.20.4','final_binary_playback':True,'saved_multiple_roots_loaded':True,'pause_resume':True,'next_previous':True,'end_of_track_auto_advance':True,'decoder_error':False,'volume_helper_tested_on_hardware':False,'hardware_test':False}
(root/'final-result.json').write_text(json.dumps(result,indent=2)+'\n');shutil.copyfile(log,root/'final-app-log.txt');print(json.dumps(result,indent=2))
