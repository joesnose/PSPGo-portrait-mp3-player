from pathlib import Path
import os,sys,subprocess,time,configparser,shutil,json,re,hashlib
r=Path(__file__).resolve().parents[1];out=r/'build/regression-v020';memory=out/'config/ppsspp';game=memory/'PSP/GAME/JoeMusic';system=memory/'PSP/SYSTEM'
long_folder='An exceptionally long album folder name that needs horizontal scrolling'
long_name='01 - A Very Long Selected Song Name That Extends Far Beyond The Player Width'
if (memory/'MUSIC').exists():shutil.rmtree(memory/'MUSIC')
for p in [game,system,memory/'MUSIC'/long_folder,memory/'MUSIC/Short Album']:p.mkdir(parents=True,exist_ok=True)
base=out/'tone.mp3'
if not base.exists():subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-f','lavfi','-i','sine=frequency=440:duration=90','-ac','2','-ar','44100','-c:a','libmp3lame','-b:a','192k',str(base),'-y'],check=True)
for name,title,folder in [(long_name,'The Long Browsed Song',long_folder),('02 Short','A long currently playing title',long_folder),('01 Other','Other','Short Album')]:
 dest=memory/'MUSIC'/folder/(name+'.mp3');subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-i',str(base),'-c','copy','-metadata','title='+title,'-metadata','artist=A very long artist name with multiple collaborators and featured musicians','-metadata','album='+folder,str(dest),'-y'],check=True)
# Cold artwork fixture: 12 additional albums with nontrivial JPEG sleeves.
from PIL import Image
for i in range(12):
 folder=memory/'MUSIC'/('Z Album %02d'%i);folder.mkdir(parents=True,exist_ok=True)
 shutil.copyfile(base,folder/'01 Song.mp3');shutil.copyfile(base,folder/'02 Song.mp3')
 art=Image.effect_noise((2048,2048),100).convert('RGB');art.save(folder/'cover.jpg',quality=75)
for folder,color in [(memory/'MUSIC'/long_folder,(180,70,60)),(memory/'MUSIC/Short Album',(50,110,200))]:Image.new('RGB',(900,900),color).save(folder/'cover.jpg')
for name,rate,opts in [('03 Variable',44100,['-q:a','4']),('04 Other Bitrate',44100,['-b:a','128k'])]:
 subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-f','lavfi','-i','sine=frequency=650:duration=90','-ac','2','-ar',str(rate),'-c:a','libmp3lame',*opts,str(memory/'MUSIC'/long_folder/(name+'.mp3')),'-y'],check=True)
folder=memory/'MUSIC/Y Natural End';folder.mkdir(parents=True,exist_ok=True)
subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-f','lavfi','-i','sine=frequency=500:duration=12','-ac','2','-ar','44100','-c:a','libmp3lame','-b:a','192k',str(folder/'01 End.mp3'),'-y'],check=True)
shutil.copyfile(base,folder/'02 Next.mp3')
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


assert 'BUILD=0.20' in log()
def hold(key,seconds=.12):
 code=D.keysym_to_keycode(XK.string_to_keysym(key));xtest.fake_input(D,X.KeyPress,code);D.sync();time.sleep(seconds);xtest.fake_input(D,X.KeyRelease,code);D.sync();time.sleep(.5)
def audio():
 lines=re.findall(r'AUDIO state=(\d+) seconds=(\d+).*?generation=(\d+) duration=(\d+) seeking=(\d+)',log());return tuple(map(int,lines[-1]))
def wait_for(test,timeout=15):
 end=time.monotonic()+timeout
 while time.monotonic()<end:
  if test():return
  time.sleep(.1)
 assert test(),audio()
def elapse(seconds):
 start=audio()[1];wait_for(lambda:audio()[1]>=start+seconds,timeout=25)
def seek_checks():
 assert 88<=audio()[3]<=92,audio();generation=audio()[2];count=log().count('PLAY_REQUEST');before=audio()[1];hold('i');wait_for(lambda:not audio()[4] and audio()[1]>=before+4);assert audio()[2]==generation
 before=audio()[1];hold('k');wait_for(lambda:not audio()[4] and audio()[1]<before);assert audio()[2]==generation
 tap('play');wait_for(lambda:audio()[0]==3);before=audio()[1];hold('i');wait_for(lambda:audio()[0]==3 and not audio()[4] and audio()[1]>=before+4)
 position=audio()[1];time.sleep(.6);assert audio()[1]==position;tap('play');time.sleep(.8);assert audio()[0]==2 and audio()[2]==generation
 assert log().count('PLAY_REQUEST')==count
# CBR playback, then VBR and a second CBR bitrate: seeking must not select another song.
tap('play');wait_for(lambda:audio()[0]==2);seek_checks()
for n in range(2):tap('down');tap('play');wait_for(lambda:audio()[0]==2);seek_checks()
assert audio()[2]==3;shot('playing-after-seeks.png')
# Library root is never dismissed by inactivity or shoulder browsing.
tap('start');elapse(6);a=shot('library-stays.png');hold('w');assert log().count('PLAY_REQUEST')==3
b=shot('library-shoulder.png');assert a.crop((12,34,180,59)).tobytes()==b.crop((12,34,180,59)).tobytes()
# Scroll through cold, large sleeves. Repeated inputs must keep selecting rows.
before=log().count('LIBRARY_SELECT');hold('Right',1.4);assert log().count('LIBRARY_SELECT')>=before+6
shot('cold-scroll.png');time.sleep(2);shot('loaded-thumbnails.png')
# Open the selected folder, and leave its short pending track highlighted past four seconds.
tap('play');elapse(6);assert 'SCROLL_IDLE_RETURN' not in log();assert 'RETURN_PLAYER' not in log();assert log().count('PLAY_REQUEST')==3
shot('folder-stays.png');tap('play');wait_for(lambda:audio()[2]==4 and audio()[0]==2)
# Ordinary player song browsing still returns to the active song after four seconds.
time.sleep(3);tap('down');wait_for(lambda:'SCROLL_IDLE_RETURN' in log(),timeout=20);assert log().count('PLAY_REQUEST')==4
# Genuine end-of-track must still advance, while the Library stays open.
tap('start');current=int(re.findall(r'LIBRARY_SELECT index=(\d+)',log())[-1])
while current!=2:tap('up');current=(current-1)%15
tap('play');time.sleep(2);tap('play');wait_for(lambda:audio()[2]==5 and audio()[0]==2);time.sleep(3);tap('start');wait_for(lambda:audio()[2]==6 and audio()[0]==2,timeout=30)
assert int(re.findall(r'AUDIO state=4 seconds=(\d+).*?generation=5',log())[-1])<=14,'Stereo fixture duration disagrees with decoded playback'
shot('library-natural-next.png')
assert 'AUDIO state=5' not in log();shutil.copyfile(game/'JOE_MUSIC_LOG.txt',out/'app-log.txt')
report={'version':'0.20','eboot_sha256':hashlib.sha256((r/'EBOOT.PBP').read_bytes()).hexdigest(),'seek_cbr_128_192_and_vbr_44100':True,'playing_seek_keeps_same_song':True,'paused_seek_keeps_pause':True,'library_root_and_folder_remain_open':True,'shoulders_keep_library_open':True,'cold_large_art_navigation':True,'ordinary_player_idle_return':True,'natural_end_still_advances_without_closing_library':True,'system_volume':'sanitized kernel-API mock; real PSP test required','hardware_test':False}
(out/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
