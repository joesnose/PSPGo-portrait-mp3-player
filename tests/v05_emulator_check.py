from pathlib import Path
import os,subprocess,time,sys,configparser,json,re,hashlib,shutil,urllib.request
r=Path(__file__).resolve().parents[1];root=r/'build/emulator-v05';memory=root/'config/ppsspp';system=memory/'PSP/SYSTEM';game=memory/'PSP/GAME/JoeMusic'
for p in [system,game,memory/'MUSIC/Album A',memory/'MUSIC/Album B']:p.mkdir(parents=True,exist_ok=True)
base=root/'tone.mp3'
if not base.exists():subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-f','lavfi','-i','sine=frequency=440:duration=60','-ac','2','-ar','44100','-c:a','libmp3lame','-b:a','192k',str(base),'-y'],check=True)
for folder,titles in [('Album A',['01 First','02 Second','03 Third']),('Album B',['01 Another','02 Last'])]:
 for title in titles:
  dest=memory/'MUSIC'/folder/(title+'.mp3')
  if not dest.exists():subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-i',str(base),'-c','copy','-metadata','title='+title,'-metadata','artist=Control test','-metadata','album='+folder,str(dest),'-y'],check=True)
shutil.copyfile(r/'EBOOT.PBP',game/'EBOOT.PBP');(game/'folders.cfg').write_text('defaults_checked=1\nroot=ms0:/MUSIC\n');shutil.copyfile(r/'DIRECT_TEST.txt',game/'DIRECT_TEST.txt')
cfg=configparser.ConfigParser();cfg.optionxform=str
cfg['General']={'FirstRun':'False','AutoRun':'True','AutoLoadSaveState':'0','IgnoreBadMemAccess':'False','EnableCheats':'False','RemoteDebuggerOnStartup':'False','RemoteDebuggerLocal':'True','RemoteISOPort':'15006'}
cfg['CPU']={'CPUCore':'1','FastMemoryAccess':'False'};cfg['SystemParam']={'PSPModel':'1'};cfg['Graphics']={'FrameSkip':'0','ShowFPSCounter':'0','InternalResolution':'1'}
with (system/'ppsspp.ini').open('w') as f:cfg.write(f)
(system/'controls.ini').write_text('[ControlMapping]\nUp = 1-19\nDown = 1-20\nLeft = 1-21\nRight = 1-22\nCross = 1-52\nCircle = 1-54\nSquare = 1-47\nTriangle = 1-29\nL = 1-45\nR = 1-51\nStart = 1-66\nSelect = 1-62\n')
env=os.environ.copy();env.update(DISPLAY=':111',XDG_CONFIG_HOME=str(root/'config'),XDG_CACHE_HOME=str(root/'cache'),SDL_AUDIODRIVER='dummy',LIBGL_ALWAYS_SOFTWARE='1')
console=(root/'console.log').open('w');proc=subprocess.Popen(['/workspace/work/squashfs-root/AppRun','--windowed','--xres','480','--yres','272','--graphics=software','--log='+str(root/'ppsspp.log'),str(game/'EBOOT.PBP')],env=env,cwd=game,stdout=console,stderr=subprocess.STDOUT,start_new_session=True);(root/'pid').write_text(str(proc.pid));time.sleep(3)
sys.path.insert(0,str(r/'build/python'))
from Xlib import X,XK,display
from Xlib.ext import xtest
from PIL import Image
d=display.Display(':111')
for w in d.screen().root.query_tree().children:
 try:
  if 'PPSSPP' in (w.get_wm_name() or ''):d.set_input_focus(w,X.RevertToParent,X.CurrentTime);d.sync()
 except Exception:pass
keys={'cross':'x','rtrigger':'w','ltrigger':'q','vol_up':'plus'}
def button(name,down):
 code=d.keysym_to_keycode(XK.string_to_keysym(keys[name]));xtest.fake_input(d,X.KeyPress if down else X.KeyRelease,code);d.sync()
def tap(name):button(name,True);time.sleep(.2);button(name,False);time.sleep(.5)
def log():return (game/'JOE_MUSIC_LOG.txt').read_text()
def shot(name):
 screen=d.screen();g=screen.root.get_geometry();im=screen.root.get_image(0,0,g.width,g.height,X.ZPixmap,0xffffffff);Image.frombytes('RGB',(g.width,g.height),im.data,'raw','BGRX').crop((160,164,640,436)).rotate(-90,expand=True).save(root/name)
assert 'BUILD=0.5' in log() and 'NOTICE 5 tracks' in log()
assert 'DIRECT_BEGIN' not in log(),'Automatic diagnostics should be disabled'
tap('cross');time.sleep(1);assert 'AUDIO state=2' in log();assert log().count('PLAY_REQUEST')==1
button('rtrigger',True);time.sleep(.25);assert 'SELECT kind=track' not in log(),'Short action fired before release'
button('rtrigger',False);time.sleep(.6);assert re.search('SELECT kind=track .*Album A/02 Second.mp3',log(),re.I)
assert log().count('PLAY_REQUEST')==1,'Browsing started playback'
# Hold: select another album once, without a short track action on release.
before=log().count('SELECT kind=track');button('rtrigger',True);start=time.monotonic()
while time.monotonic()-start<5 and 'SELECT kind=folder' not in log():time.sleep(.1)
assert re.search('SELECT kind=folder .*Album B/01 Another.mp3',log(),re.I),log()[-1000:]
time.sleep(.4);button('rtrigger',False);time.sleep(.5)
assert log().count('SELECT kind=folder')==1 and log().count('SELECT kind=track')==before
assert log().count('PLAY_REQUEST')==1;shot('selection-preview.png')
tap('rtrigger');assert re.search('SELECT kind=track .*Album B/02 Last.mp3',log(),re.I)
button('ltrigger',True);time.sleep(2);button('ltrigger',False);time.sleep(.6);assert re.search('SELECT kind=folder .*Album A/01 First.mp3',log(),re.I)
assert log().count('PLAY_REQUEST')==1
button('rtrigger',True);time.sleep(2);button('rtrigger',False);time.sleep(.6)
tap('cross');assert log().count('PLAY_REQUEST')==2
assert re.search('PLAY_REQUEST .*Album B/01 Another.mp3',log(),re.I)
shot('now-playing.png');assert 'AUDIO state=5' not in log()
shutil.copyfile(game/'JOE_MUSIC_LOG.txt',root/'app-log.txt')
report={'version':'0.5','eboot_sha256':hashlib.sha256((r/'EBOOT.PBP').read_bytes()).hexdigest(),'ppsspp':'1.20.4','short_shoulder_action_on_release':True,'short_shoulder_selects_within_folder':True,'hold_selects_adjacent_folder_once':True,'long_release_has_no_short_action':True,'browsing_does_not_start_playback':True,'explicit_selection_playback':True,'automatic_file_test_removed':True,'volume_input_injected':False,'real_volume_getter_tested':False,'native_home_tap_and_menu_tested':False,'hardware_test':False}
(root/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
