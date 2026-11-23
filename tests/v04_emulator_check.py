from pathlib import Path
import os,signal,shutil,subprocess,sys,time,json,re,hashlib
r=Path(__file__).resolve().parents[1];root=r/'build/emulator';memory=root/'config/ppsspp';game=memory/'PSP/GAME/JoeMusic'
try:os.kill(int((root/'pid').read_text()),signal.SIGKILL)
except ProcessLookupError:pass
album=memory/'MUSIC/(1985) Little Creatures';album.mkdir(parents=True,exist_ok=True)
shutil.copyfile(memory/'MUSIC/Album/Disc/b-sidecar.mp3',album/'09. Road to Nowhere.mp3')
# This file is outside every selected library root. Direct playback must work.
(game/'folders.cfg').write_text('defaults_checked=1\nroot=ms0:/PODCASTS\n')
shutil.copyfile(r/'DIRECT_TEST.txt',game/'DIRECT_TEST.txt');shutil.copyfile(r/'EBOOT.PBP',game/'EBOOT.PBP')
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

def key(name,delay=.6):
 c=d.keysym_to_keycode(XK.string_to_keysym(name));xtest.fake_input(d,X.KeyPress,c);d.sync();time.sleep(.1);xtest.fake_input(d,X.KeyRelease,c);d.sync();time.sleep(delay)
def screenshot(source,dest):
 im=Image.open(root/source);im.crop((160,164,640,436)).rotate(-90,expand=True).save(root/dest)
screenshot('library.png','v04-direct-check.png');screenshot('playing.png','v04-direct-playing.png')
log=(game/'JOE_MUSIC_LOG.txt').read_text();exact='ms0:/MUSIC/(1985) Little Creatures/09. Road to Nowhere.mp3'
assert 'BUILD=0.4' in log and 'NOTICE 1 tracks' in log
assert 'DIRECT_FILE path='+exact in log and 'read=16 header=494433' in log,log[-2500:]
assert 'DIRECT_PARENT path=ms0:/MUSIC/(1985) Little Creatures listed=1' in log
assert 'DIRECT_PLAY path='+exact in log and 'AUDIO state=2' in log
key('x');key('x');started=time.monotonic()
while time.monotonic()-started<25:
 log=(game/'JOE_MUSIC_LOG.txt').read_text()
 if 'AUDIO state=5' in log:raise RuntimeError(log[-1500:])
 if 'AUDIO state=4' in log:break
 time.sleep(.2)
assert 'AUDIO state=3' in log and 'AUDIO state=4' in log
shutil.copyfile(game/'JOE_MUSIC_LOG.txt',root/'v04-app-log.txt')
report={'version':'0.4','eboot_sha256':hashlib.sha256((r/'EBOOT.PBP').read_bytes()).hexdigest(),'ppsspp':'1.20.4','synthetic_mp3_at_requested_path':True,'automatic_direct_probe':True,'direct_open_and_header_read':True,'alternate_storage_failure_reported':True,'parent_listing_comparison':True,'playback_outside_selected_library':True,'pause_resume':True,'end_of_track':True,'host_mock_hidden_entry_and_failed_stat_passed':True,'hardware_test':False,'actual_user_song_tested':False,'volume_helper_changed':False}
(root/'v04-result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
