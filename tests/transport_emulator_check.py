from pathlib import Path
import os,sys,subprocess,time,configparser,shutil,json,re,hashlib
r=Path(__file__).resolve().parents[1];out=r/'build/transport-check';memory=out/'config/ppsspp';game=memory/'PSP/GAME/JoeMusic';system=memory/'PSP/SYSTEM'
long_folder='An exceptionally long album folder name that needs horizontal scrolling'
long_name='01 - A Very Long Selected Song Name That Extends Far Beyond The Player Width'
for p in [game,system,memory/'MUSIC'/long_folder,memory/'MUSIC/Short Album']:p.mkdir(parents=True,exist_ok=True)
base=out/'tone.mp3'
if not base.exists():subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-f','lavfi','-i','sine=frequency=440:duration=90','-ac','2','-ar','44100','-c:a','libmp3lame','-b:a','192k',str(base),'-y'],check=True)
for name,title,folder in [(long_name,'The Long Browsed Song',long_folder),('02 Short','A long currently playing title',long_folder),('01 Other','Other','Short Album')]:
 dest=memory/'MUSIC'/folder/(name+'.mp3');subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-i',str(base),'-c','copy','-metadata','title='+title,'-metadata','artist=A very long artist name with multiple collaborators and featured musicians','-metadata','album='+folder,str(dest),'-y'],check=True)
(game/'JOE_MUSIC_LOG.txt').unlink(missing_ok=True)
(game/'folders.cfg').write_text('defaults_checked=1\nroot=ms0:/MUSIC\n');(game/'position.txt').write_text('ms0:/MUSIC/'+long_folder+'/02 Short.mp3\n');shutil.copyfile(r/'EBOOT.PBP',game/'EBOOT.PBP')
c=configparser.ConfigParser();c.optionxform=str;c['General']={'FirstRun':'False','AutoRun':'True'};c['CPU']={'CPUCore':'1'};c['SystemParam']={'PSPModel':'0'};c['Graphics']={'InternalResolution':'1','FrameSkip':'0'}
with (system/'ppsspp.ini').open('w') as f:c.write(f)
(system/'controls.ini').write_text('[ControlMapping]\nAn.Up = 1-37\nAn.Down = 1-39\nAn.Left = 1-38\nAn.Right = 1-40\nUp = 1-19\nDown = 1-20\nLeft = 1-21\nRight = 1-22\nCross = 1-52\nCircle = 1-54\nSquare = 1-47\nTriangle = 1-29\nL = 1-45\nR = 1-51\nStart = 1-66\nSelect = 1-62\n')
env=os.environ.copy();env.update(DISPLAY=':116',XDG_CONFIG_HOME=str(out/'config'),XDG_CACHE_HOME=str(out/'cache'),SDL_AUDIODRIVER='dummy',LIBGL_ALWAYS_SOFTWARE='1')
logfile=(out/'console.log').open('w');proc=subprocess.Popen(['/workspace/work/squashfs-root/AppRun','--windowed','--xres','480','--yres','272','--graphics=software',str(game/'EBOOT.PBP')],env=env,cwd=game,stdout=logfile,stderr=subprocess.STDOUT,start_new_session=True);(out/'pid').write_text(str(proc.pid));time.sleep(3)
sys.path.insert(0,os.environ.get('JOE_TEST_PYTHONPATH','/workspace/work/joesnose-music/build/python'))
from Xlib import X,XK,display
from Xlib.ext import xtest
from PIL import Image
D=display.Display(':116')
for w in D.screen().root.query_tree().children:
 try:
  if 'PPSSPP' in (w.get_wm_name() or ''):D.set_input_focus(w,X.RevertToParent,X.CurrentTime);D.sync()
 except Exception:pass
keys={'play':'z','start':'Return','up':'Left','down':'Right'}
def tap(name):
 code=D.keysym_to_keycode(XK.string_to_keysym(keys[name]));xtest.fake_input(D,X.KeyPress,code);D.sync();time.sleep(.15);xtest.fake_input(D,X.KeyRelease,code);D.sync();time.sleep(.4)
def shot(name):
 s=D.screen();g=s.root.get_geometry();a=s.root.get_image(0,0,g.width,g.height,X.ZPixmap,0xffffffff);im=Image.frombytes('RGB',(g.width,g.height),a.data,'raw','BGRX').crop((160,164,640,436)).rotate(-90,expand=True);im.save(out/name);return im
def log():return (game/'JOE_MUSIC_LOG.txt').read_text()

assert 'BUILD=0.16' in log()
def hold(key,seconds=.12):
 code=D.keysym_to_keycode(XK.string_to_keysym(key));xtest.fake_input(D,X.KeyPress,code);D.sync();time.sleep(seconds);xtest.fake_input(D,X.KeyRelease,code);D.sync();time.sleep(.8)
def wait_for(test,timeout=8):
 end=time.monotonic()+timeout
 while time.monotonic()<end:
  if test():return
  time.sleep(.1)
 assert test(),audio()
def audio():
 lines=re.findall(r'AUDIO state=(\d+) seconds=(\d+).*?generation=(\d+) duration=(\d+) volume=(\d+)',log());return tuple(map(int,lines[-1]))
tap('play');time.sleep(1);assert audio()[0]==2;assert 88<=audio()[3]<=92,audio()
# Physical left/right become portrait up/down.
hold('l');assert 'STICK_VOLUME value=29' in log();hold('j');assert 'STICK_VOLUME value=30' in log()
# Physical up/down become portrait right/left: seek the active track.
before=audio()[1];hold('i');assert 'SEEK delta=5' in log();wait_for(lambda:audio()[1]>=before+4)
before=audio()[1];hold('k');assert 'SEEK delta=-5' in log();wait_for(lambda:audio()[1]<before)
tap('play');assert audio()[0]==3;before=audio()[1];hold('i');wait_for(lambda:audio()[0]==3 and audio()[1]>=before+4)
held=audio()[1];time.sleep(.7);assert audio()[1]==held;shot('paused-seek.png');tap('play')
# Once the top title reaches its end, it remains there; selected song keeps looping.
time.sleep(8);a=shot('once-end.png');time.sleep(2);b=shot('once-still.png');assert a.crop((16,34,256,59)).tobytes()==b.crop((16,34,256,59)).tobytes()
# Bar has a visible filled portion between timer and icon.
assert len(set(a.getpixel((x,469)) for x in range(50,233)))>=2,'Progress fill missing'
assert 'AUDIO state=5' not in log();assert log().count('PLAY_REQUEST')==1
# The currently playing long filename must keep moving without an idle return.
tap('up');time.sleep(.7);tap('play');time.sleep(2);c=shot('loop-start.png');time.sleep(2);d=shot('loop-moving.png');assert c.crop((22,396,242,417)).tobytes()!=d.crop((22,396,242,417)).tobytes()
time.sleep(16);shot('loop-separator.png');assert 'SCROLL_IDLE_RETURN' not in log();assert 'AUDIO state=5' not in log()
shutil.copyfile(game/'JOE_MUSIC_LOG.txt',out/'app-log.txt')
report={'version':'0.16','eboot_sha256':hashlib.sha256((r/'EBOOT.PBP').read_bytes()).hexdigest(),'portrait_analogue_volume':True,'portrait_analogue_forward_backward_seek':True,'paused_seek_preserves_pause':True,'duration_90_second_fixture':True,'top_title_stops_after_once':True,'currently_playing_long_filename_loops':True,'progress_bar_screenshot':'paused-seek.png','hardware_test':False}
(out/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
