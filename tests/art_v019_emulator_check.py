from pathlib import Path
import os,sys,subprocess,time,configparser,shutil,json,re,hashlib,signal
from PIL import Image
r=Path(__file__).resolve().parents[1];out=r/'build/art-v019';memory=out/'config/ppsspp';game=memory/'PSP/GAME/JoeMusic';system=memory/'PSP/SYSTEM'
for p in [game,system]:p.mkdir(parents=True,exist_ok=True)
for p in [memory/'MUSIC',game/'cover-cache-v1']:
 if p.exists():shutil.rmtree(p)
base=out/'tone.mp3';sleeve=out/'sleeve.jpg';embedded=out/'embedded.mp3'
if not base.exists():subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-f','lavfi','-i','sine=frequency=440:duration=90','-ac','2','-ar','44100','-c:a','libmp3lame','-b:a','192k',str(base),'-y'],check=True)
if not sleeve.exists():Image.effect_noise((2048,2048),100).convert('RGB').save(sleeve,quality=75)
if not embedded.exists():subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-i',str(base),'-i',str(sleeve),'-map','0:a','-map','1:v','-c','copy','-id3v2_version','3','-metadata:s:v','title=Album cover','-metadata:s:v','comment=Cover (front)',str(embedded),'-y'],check=True)
for i in range(18):
 folder=memory/'MUSIC'/('Album %03d'%i);folder.mkdir(parents=True);os.link(base if i==0 else embedded,folder/'01 Song.mp3')
 if i==0:Image.new('RGB',(512,512),(190,40,40)).save(folder/'cover.jpg')
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
 assert 'BUILD=0.19' in log()
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
launch('cold');tap('z');wait_for(lambda:audio()[0]==2);tap('Return')
wait_for(lambda:len(list((game/'cover-cache-v1').glob('*.thumb')))>=6,timeout=25)
# Job counts should stay stable throughout held navigation, even with large embedded JPEGs.
wait_for(lambda:stats()[0]>=6);before=stats()[0];start=len(log());key('Right',True);time.sleep(1.8);key('Right',False)
rows=[int(v) for v in re.findall(r'LIBRARY_SELECT index=(\d+)',log()[start:])];assert len(rows)>=6,rows
while_moving=stats()[0]-before;assert while_moving<=1,while_moving
selected=rows[-1];y=101+(selected if selected<6 else 5)*54
began=time.monotonic()
def selected_art():
 colors=frame().crop((19,y+2,59,y+42)).getcolors(2000);return colors and len(colors)>16
wait_for(selected_art,timeout=15);selected_delay=time.monotonic()-began;frame('selected-art.png')
wait_for(lambda:stats()[0]>before,timeout=5);assert 'AUDIO state=5' not in log();assert log().count('PLAY_REQUEST')==1
shutil.copyfile(game/'JOE_MUSIC_LOG.txt',out/'cold-app-log.txt');cold=stats()
# Existing cache format must still survive restarting the application.
os.killpg(proc.pid,signal.SIGTERM);proc.wait(timeout=5);time.sleep(.3)
(game/'position.txt').write_text('ms0:/MUSIC/Album 000/01 Song.mp3\n');launch('warm');tap('Return')
wait_for(lambda:stats()[1]>=6 and is_red(frame().getpixel((30,120))),timeout=12);assert stats()[0]==0,stats();frame('warm-cache.png');shutil.copyfile(game/'JOE_MUSIC_LOG.txt',out/'warm-app-log.txt')
report={'version':'0.19','eboot_sha256':hashlib.sha256((r/'EBOOT.PBP').read_bytes()).hexdigest(),'large_embedded_jpeg_bytes':sleeve.stat().st_size,'navigation_events_while_held':len(rows),'art_jobs_completed_while_moving':while_moving,'selected_art_after_release_seconds':round(selected_delay,3),'warm_boot_decodes_and_disk_hits':stats(),'playback_continues':True,'hardware_test':False}
(out/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
