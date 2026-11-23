from pathlib import Path
import os,sys,subprocess,time,configparser,shutil,json,re,hashlib
r=Path(__file__).resolve().parents[1];out=r/'build/marquee-check';memory=out/'config/ppsspp';game=memory/'PSP/GAME/JoeMusic';system=memory/'PSP/SYSTEM'
long_folder='An exceptionally long album folder name that needs horizontal scrolling'
long_name='01 - A Very Long Selected Song Name That Extends Far Beyond The Player Width'
for p in [game,system,memory/'MUSIC'/long_folder,memory/'MUSIC/Short Album']:p.mkdir(parents=True,exist_ok=True)
base=out/'tone.mp3'
if not base.exists():subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-f','lavfi','-i','sine=frequency=440:duration=90','-ac','2','-ar','44100','-c:a','libmp3lame','-b:a','192k',str(base),'-y'],check=True)
for name,title,folder in [(long_name,'The Long Browsed Song',long_folder),('02 Short','A long currently playing title with enough words to overflow the top of the player screen',long_folder),('01 Other','Other','Short Album')]:
 dest=memory/'MUSIC'/folder/(name+'.mp3');subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-i',str(base),'-c','copy','-metadata','title='+title,'-metadata','artist=A very long artist name with multiple collaborators and featured musicians','-metadata','album='+folder,str(dest),'-y'],check=True)
(game/'JOE_MUSIC_LOG.txt').unlink(missing_ok=True)
(game/'folders.cfg').write_text('defaults_checked=1\nroot=ms0:/MUSIC\n');(game/'position.txt').write_text('ms0:/MUSIC/'+long_folder+'/02 Short.mp3\n');shutil.copyfile(r/'EBOOT.PBP',game/'EBOOT.PBP')
c=configparser.ConfigParser();c.optionxform=str;c['General']={'FirstRun':'False','AutoRun':'True'};c['CPU']={'CPUCore':'1'};c['SystemParam']={'PSPModel':'0'};c['Graphics']={'InternalResolution':'1','FrameSkip':'0'}
with (system/'ppsspp.ini').open('w') as f:c.write(f)
(system/'controls.ini').write_text('[ControlMapping]\nUp = 1-19\nDown = 1-20\nLeft = 1-21\nRight = 1-22\nCross = 1-52\nCircle = 1-54\nSquare = 1-47\nTriangle = 1-29\nL = 1-45\nR = 1-51\nStart = 1-66\nSelect = 1-62\n')
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
assert 'BUILD=0.15.2' in log();tap('play');time.sleep(.7);a=shot('player-start.png');time.sleep(2);b=shot('player-scrolling.png')
for box in [(16,34,256,59),(16,61,256,79),(16,322,256,341)]:assert a.crop(box).tobytes()!=b.crop(box).tobytes(),box
# Thumbnail space and unselected rows must not move with the selected name.
tap('start');time.sleep(.3);a=shot('library-start.png');time.sleep(2);b=shot('library-scrolling.png')
assert a.crop((73,107,254,125)).tobytes()!=b.crop((73,107,254,125)).tobytes()
assert a.crop((73,126,254,142)).tobytes()!=b.crop((73,126,254,142)).tobytes()
assert a.crop((17,101,61,145)).tobytes()==b.crop((17,101,61,145)).tobytes(),'Text overwrote thumbnail'
assert a.crop((73,161,254,196)).tobytes()==b.crop((73,161,254,196)).tobytes(),'Unselected row moved'
tap('play');time.sleep(.2);a=shot('selected-start.png');time.sleep(2);b=shot('selected-scrolling.png');assert a.crop((22,396,242,417)).tobytes()!=b.crop((22,396,242,417)).tobytes()
# Non-text pixels beside the selected row remain identical.
assert a.crop((0,389,20,418)).tobytes()==b.crop((0,389,20,418)).tobytes()
assert a.crop((244,389,272,418)).tobytes()==b.crop((244,389,272,418)).tobytes()
time.sleep(2.5);assert 'SCROLL_IDLE_RETURN' not in log(),'Long name returned before its first pass'
end=time.monotonic()+35
while time.monotonic()<end and 'SCROLL_IDLE_RETURN' not in log():time.sleep(.1)
assert 'SCROLL_IDLE_RETURN' in log();assert log().count('PLAY_REQUEST')==1,'Reading a name changed playback'
# A short pending song still returns on the ordinary four-second timer.
time.sleep(1.5);tap('up');time.sleep(.7);assert 'SELECT kind=track index=0' in log();tap('play');time.sleep(.7);tap('down');before=log().count('SCROLL_IDLE_RETURN');end=time.monotonic()+7
while time.monotonic()<end and log().count('SCROLL_IDLE_RETURN')==before:time.sleep(.1)
assert log().count('SCROLL_IDLE_RETURN')==before+1
assert 'AUDIO state=5' not in log();shutil.copyfile(game/'JOE_MUSIC_LOG.txt',out/'app-log.txt')
report={'version':'0.15.2','eboot_sha256':hashlib.sha256((r/'EBOOT.PBP').read_bytes()).hexdigest(),'selected_library_name_and_path_scroll':True,'player_title_album_artist_scroll':True,'selected_song_scroll':True,'thumbnails_and_adjacent_rows_preserved':True,'text_clipped_to_selected_row':True,'long_name_completes_before_idle_return':True,'short_name_idle_return_retained':True,'no_browsing_playback_change':True,'hardware_test':False}
(out/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
