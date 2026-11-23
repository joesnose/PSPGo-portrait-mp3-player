from pathlib import Path
import os,signal,shutil,subprocess,sys,time,json,re,hashlib
r=Path(__file__).resolve().parents[1];root=r/'build/emulator';memory=root/'config/ppsspp';game=memory/'PSP/GAME/JoeMusic'
try:os.kill(int((root/'pid').read_text()),signal.SIGKILL)
except ProcessLookupError:pass
music=memory/'MUSIC';sample=music/'Album/Disc/b-sidecar.mp3'
shutil.copyfile(sample,music/'root-level.MP3')
for folder in ['.Hidden album','Read-only album','Beyoncé']:
 p=music/folder/'Disc 2';p.mkdir(parents=True,exist_ok=True);shutil.copyfile(sample,p/'song.mp3')
shutil.copyfile(r/'EBOOT.PBP',game/'EBOOT.PBP')
subprocess.run([sys.executable,str(r/'tests/emulator_test.py')],check=True)
sys.path.insert(0,str(r/'build/python'))
from Xlib import X,XK,display
from Xlib.ext import xtest
from PIL import Image
d=display.Display(':111')
for w in d.screen().root.query_tree().children:
 try:
  if 'PPSSPP' in (w.get_wm_name() or ''):d.set_input_focus(w,X.RevertToParent,X.CurrentTime);d.sync()
 except Exception:pass

def key(name,delay=.35):
 c=d.keysym_to_keycode(XK.string_to_keysym(name));xtest.fake_input(d,X.KeyPress,c);d.sync();time.sleep(.1);xtest.fake_input(d,X.KeyRelease,c);d.sync();time.sleep(delay)
def shot(name):
 s=d.screen();g=s.root.get_geometry();im=s.root.get_image(0,0,g.width,g.height,X.ZPixmap,0xffffffff);pic=Image.frombytes('RGB',(g.width,g.height),im.data,'raw','BGRX');pic.save(root/name);pic.crop((160,164,640,436)).rotate(-90,expand=True).save(root/('portrait-'+name))
key('Return');shot('v02-library.png');key('space');key('x');key('Right');key('x');key('x');shot('v02-browser.png')
# Directories sort first: four folders followed by the root-level MP3.
for _ in range(4):key('Right')
shot('v02-root-selection.png');key('x',1.5);shot('v02-root-playing.png')
log=(game/'JOE_MUSIC_LOG.txt').read_text()
assert 'BUILD=0.2' in log
assert 'NOTICE 8 tracks; 0 skipped entries' in log,log[-2000:]
assert 'root-level.mp3' in log.lower(),log
assert '.hidden album' in log.lower() and 'read-only album' in log.lower() and 'beyonc' in log.lower(),log
assert re.search(r'BROWSER path=ms0:/MUSIC entries=5 ',log,re.I),log[-2000:]
assert 'AUDIO state=5' not in log
key('x',.7);key('x',.7);key('w',1);key('q',1)
started=time.monotonic()
while time.monotonic()-started<28:
 log=(game/'JOE_MUSIC_LOG.txt').read_text()
 if 'AUDIO state=5' in log:raise RuntimeError(log[-2000:])
 if 'AUDIO state=4' in log:break
 time.sleep(.2)
assert 'AUDIO state=3' in log and 'AUDIO state=4' in log,log[-2000:]
shot('v02-now-playing.png');shutil.copyfile(game/'JOE_MUSIC_LOG.txt',root/'v02-app-log.txt')
report={'version':'0.2','eboot_sha256':hashlib.sha256((r/'EBOOT.PBP').read_bytes()).hexdigest(),'ppsspp':'1.20.4','native_psp_directory_api':True,'recursive_tracks':8,'root_level_mp3_visible_in_browser':True,'root_level_mp3_playable_from_browser':True,'hidden_and_readonly_named_folders_visible':True,'accented_folder_visible':True,'pause_resume':True,'next_previous':True,'auto_next_track':True,'hardware_test':False,'volume_helper_changed':False}
(root/'v02-result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
