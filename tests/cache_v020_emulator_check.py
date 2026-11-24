from pathlib import Path
import os,sys,subprocess,time,configparser,shutil,json,re,hashlib,signal
from PIL import Image
r=Path(__file__).resolve().parents[1];out=r/'build/cache-v020';memory=out/'config/ppsspp';game=memory/'PSP/GAME/JoeMusic';system=memory/'PSP/SYSTEM'
for p in [game,system]:p.mkdir(parents=True,exist_ok=True)
for p in [memory/'MUSIC',game/'cover-cache-v1']:
 if p.exists():shutil.rmtree(p)
base=out/'tone.mp3'
if not base.exists():subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-f','lavfi','-i','sine=frequency=440:duration=180','-ac','2','-ar','44100','-c:a','libmp3lame','-b:a','192k',str(base),'-y'],check=True)
for i in range(80):
 folder=memory/'MUSIC'/('Album %03d'%i);folder.mkdir(parents=True);os.link(base,folder/'01 Song.mp3')
 color=(190,40,40) if i==0 else ((40+i*13)%220+20,(30+i*19)%220+20,(20+i*23)%220+20)
 Image.new('RGB',(512,512),color).save(folder/'cover.jpg')
(game/'folders.cfg').write_text('defaults_checked=1\nroot=ms0:/MUSIC\n');(game/'position.txt').write_text('ms0:/MUSIC/Album 000/01 Song.mp3\n');shutil.copyfile(r/'EBOOT.PBP',game/'EBOOT.PBP')
c=configparser.ConfigParser();c.optionxform=str;c['General']={'FirstRun':'False','AutoRun':'True'};c['CPU']={'CPUCore':'1'};c['SystemParam']={'PSPModel':'0'};c['Graphics']={'InternalResolution':'1','FrameSkip':'0'}
with (system/'ppsspp.ini').open('w') as f:c.write(f)
(system/'controls.ini').write_text('[ControlMapping]\nAn.Up = 1-37\nAn.Down = 1-39\nAn.Left = 1-38\nAn.Right = 1-40\nUp = 1-19\nDown = 1-20\nLeft = 1-21\nRight = 1-22\nCross = 1-52\nCircle = 1-54\nSquare = 1-47\nTriangle = 1-29\nL = 1-45\nR = 1-51\nStart = 1-66\nSelect = 1-62\n')
env=os.environ.copy();env.update(DISPLAY=':116',XDG_CONFIG_HOME=str(out/'config'),XDG_CACHE_HOME=str(out/'cache'),SDL_AUDIODRIVER='dummy',LIBGL_ALWAYS_SOFTWARE='1')

sys.path.insert(0,os.environ.get('JOE_TEST_PYTHONPATH','/workspace/work/joesnose-music/build/python'))
from Xlib import X,XK,display
from Xlib.ext import xtest
D=display.Display(':116')
def launch(name):
 global proc
 log_file=(out/('console-'+name+'.log')).open('w');proc=subprocess.Popen(['/workspace/work/squashfs-root/AppRun','--windowed','--xres','480','--yres','272','--graphics=software',str(game/'EBOOT.PBP')],env=env,cwd=game,stdout=log_file,stderr=subprocess.STDOUT,start_new_session=True);(out/'pid').write_text(str(proc.pid));time.sleep(3)
 for w in D.screen().root.query_tree().children:
  try:
   if 'PPSSPP' in (w.get_wm_name() or ''):D.set_input_focus(w,X.RevertToParent,X.CurrentTime);D.sync()
  except Exception:pass
 assert 'BUILD=0.20' in log()
def log():return (game/'JOE_MUSIC_LOG.txt').read_text()
def key(name,down):
 code=D.keysym_to_keycode(XK.string_to_keysym(name));xtest.fake_input(D,X.KeyPress if down else X.KeyRelease,code);D.sync()
def tap(name):key(name,True);time.sleep(.12);key(name,False);time.sleep(.35)
def frame(name=None):
 screen=D.screen();g=screen.root.get_geometry();data=screen.root.get_image(0,0,g.width,g.height,X.ZPixmap,0xffffffff).data
 im=Image.frombytes('RGB',(g.width,g.height),data,'raw','BGRX').crop((160,164,640,436)).rotate(-90,expand=True)
 if name:im.save(out/name)
 return im
def wait_for(test,timeout=20):
 end=time.monotonic()+timeout
 while time.monotonic()<end:
  if test():return
  time.sleep(.05)
 assert test(),log()[-1400:]
def audio():return tuple(map(int,re.findall(r'AUDIO state=(\d+) seconds=(\d+).*?generation=(\d+) duration=(\d+) seeking=(\d+)',log())[-1]))
def targets(start):return [int(v) for v in re.findall(r'SEEK delta=-5 from=\d+ target=(\d+)',log()[start:])]
def is_red(rgb):return rgb[0]>140 and rgb[1]<90 and rgb[2]<90
def stats():
 matches=re.findall(r'ART_CACHE_STATS decoded=(\d+) disk=(\d+)',log());return tuple(map(int,matches[-1])) if matches else (0,0)
def details():
 m=re.findall(r'ART_CACHE_STATS decoded=(\d+) disk=(\d+) retained=(\d+) bytes=(\d+) saved=(\d+) errors=(\d+)',log());return tuple(map(int,m[-1])) if m else (0,)*6
def move_to(target):
 current=int(re.findall(r'LIBRARY_SELECT index=(\d+)',log())[-1]) if 'LIBRARY_SELECT' in log() else 0
 while current!=target:
  direction='Right' if target>current else 'Left';key(direction,True);time.sleep(.06);key(direction,False);time.sleep(.07)
  current=int(re.findall(r'LIBRARY_SELECT index=(\d+)',log())[-1])
launch('cold');tap('z');wait_for(lambda:audio()[0]==2);tap('Return')
for target in range(0,73,8):
 move_to(target);wanted=min(80,target+9);wait_for(lambda:details()[2]>=wanted and details()[4]>=wanted,timeout=15)
assert details()[2]==80 and details()[5]==0,details();before=details()
move_to(0);wait_for(lambda:is_red(frame().getpixel((30,120))),timeout=2);after=details();assert after[:2]==before[:2],(before,after)
frame('return-first-page.png');tap('Return');tap('Return');wait_for(lambda:is_red(frame().getpixel((30,120))),timeout=2);assert details()[:2]==before[:2]
shutil.copyfile(game/'JOE_MUSIC_LOG.txt',out/'cold-app-log.txt');os.killpg(proc.pid,signal.SIGTERM);proc.wait(timeout=5);time.sleep(.3)
(game/'position.txt').write_text('ms0:/MUSIC/Album 000/01 Song.mp3\n');launch('warm');tap('Return');wait_for(lambda:details()[2]>=9 and is_red(frame().getpixel((30,120))),timeout=10);assert details()[0]==0 and details()[1]>=8,details();frame('warm-first-page.png');shutil.copyfile(game/'JOE_MUSIC_LOG.txt',out/'warm-app-log.txt')
report={'version':'0.20','eboot_sha256':hashlib.sha256((r/'EBOOT.PBP').read_bytes()).hexdigest(),'all_80_covers_retained':True,'first_page_return_has_no_disk_reads_or_decodes':True,'close_and_reopen_library_has_no_reload':True,'cold_details':before,'warm_details':details(),'hardware_test':False}
(out/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
