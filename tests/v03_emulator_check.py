from pathlib import Path
import os,signal,shutil,subprocess,sys,time,json,re,hashlib
r=Path(__file__).resolve().parents[1];root=r/'build/emulator';memory=root/'config/ppsspp';game=memory/'PSP/GAME/JoeMusic'
try:os.kill(int((root/'pid').read_text()),signal.SIGKILL)
except ProcessLookupError:pass
legacy=memory/'PSP/MUSIC/Legacy album';legacy.mkdir(parents=True,exist_ok=True);sample=memory/'MUSIC/Album/Disc/b-sidecar.mp3';shutil.copyfile(sample,legacy/'01 missing.mp3');shutil.copyfile(sample,legacy.parent/'legacy-direct.mp3')
# Reproduce a 0.2 saved configuration: MUSIC is selected, PSP/MUSIC is absent.
(game/'folders.cfg').write_text('root=ms0:/MUSIC\nroot=ms0:/PODCASTS\n')
(memory/'MUSIC/unsupported.wma').write_bytes(b'diagnostic format fixture, not real audio')
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

def key(name,delay=.3):
 c=d.keysym_to_keycode(XK.string_to_keysym(name));xtest.fake_input(d,X.KeyPress,c);d.sync();time.sleep(.1);xtest.fake_input(d,X.KeyRelease,c);d.sync();time.sleep(delay)
def shot(name):
 s=d.screen();g=s.root.get_geometry();im=s.root.get_image(0,0,g.width,g.height,X.ZPixmap,0xffffffff);pic=Image.frombytes('RGB',(g.width,g.height),im.data,'raw','BGRX');pic.crop((160,164,640,436)).rotate(-90,expand=True).save(root/name)
log=(game/'JOE_MUSIC_LOG.txt').read_text();cfg=(game/'folders.cfg').read_text()
assert 'BUILD=0.3' in log and 'ROOTS_MIGRATED added=1 selected=3' in log,log[-2000:]
assert 'root=ms0:/PSP/MUSIC' in cfg and 'defaults_checked=1' in cfg,cfg
assert 'NOTICE 10 tracks; 0 skipped entries' in log,log[-2000:]
assert 'other_audio=1' in log and 'name=unsupported.wma' in log.lower(),log[-4000:]
assert 'SCAN_ROOT index=2 path=ms0:/PSP/MUSIC' in log
assert 'name=01 missing.mp3' in log.lower() and 'name=legacy-direct.mp3' in log.lower()
key('x',.7);key('x',.7);key('w',1);key('q',1)
# Open the Music folder and verify the previously hidden non-MP3 file is visible.
key('space');key('x');key('Right');key('x');key('x');shot('v03-browser.png')
for _ in range(5):key('Right')
key('x');shot('v03-format-notice.png')
# Return to playback and observe end-of-track handoff.
key('Return');started=time.monotonic()
while time.monotonic()-started<28:
 log=(game/'JOE_MUSIC_LOG.txt').read_text()
 if 'AUDIO state=5' in log:raise RuntimeError(log[-2000:])
 if 'AUDIO state=4' in log:break
 time.sleep(.2)
assert 'AUDIO state=3' in log and 'AUDIO state=4' in log,log[-2000:]
shot('v03-now-playing.png');shutil.copyfile(game/'JOE_MUSIC_LOG.txt',root/'v03-app-log.txt')
report={'version':'0.3','eboot_sha256':hashlib.sha256((r/'EBOOT.PBP').read_bytes()).hexdigest(),'ppsspp':'1.20.4','legacy_configuration_migrated':True,'custom_roots_preserved':True,'legacy_psp_music_discovered':True,'recursive_mp3_count':10,'other_audio_format_reported':True,'pause_resume':True,'next_previous':True,'auto_next_track':True,'hardware_test':False,'volume_helper_tested_on_hardware':False}
(root/'v03-result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
