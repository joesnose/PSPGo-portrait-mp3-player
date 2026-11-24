from pathlib import Path
import os,sys,subprocess,time,configparser,shutil,json,re,hashlib,signal
from PIL import Image
r=Path(__file__).resolve().parents[1];out=r/'build/fixes-v018';memory=out/'config/ppsspp';game=memory/'PSP/GAME/JoeMusic';system=memory/'PSP/SYSTEM'
for p in [game,system]:p.mkdir(parents=True,exist_ok=True)
for p in [memory/'MUSIC',game/'cover-cache-v1']:
 if p.exists():shutil.rmtree(p)
base=out/'tone.mp3'
if not base.exists():subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-f','lavfi','-i','sine=frequency=440:duration=90','-ac','2','-ar','44100','-c:a','libmp3lame','-b:a','192k',str(base),'-y'],check=True)
for i in range(72):
 folder=memory/'MUSIC'/('Album %03d'%i);folder.mkdir(parents=True)
 os.link(base,folder/'01 Song.mp3')
 color=(190,40,40) if i==0 else (40,70,180) if i==1 else ((40+i*13)%220+20,(30+i*19)%220+20,(20+i*23)%220+20)
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
 assert 'BUILD=0.18' in log()
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
launch('cold');tap('z');wait_for(lambda:audio()[0]==2)
# Preview a different album, then sample every displayed cover throughout the play request.
tap('Return');tap('Right');tap('z');time.sleep(.8);assert frame().getpixel((130,170))[2]>140
start=len(log());key('z',True);samples=[]
for i in range(60):
 if i==8:key('z',False)
 samples.append(frame().getpixel((130,170)));time.sleep(.01)
assert all(not (r>120 and r>g*1.8) for r,g,b in samples),'Previous red artwork flashed'
wait_for(lambda:audio()[2]==2 and audio()[0]==2);frame('new-song-art.png')
# Forward far enough that a held rewind must retarget a still-running decoder seek.
key('i',True);time.sleep(3.1);key('i',False)
forward=int(re.findall(r'SEEK delta=5 from=\d+ target=(\d+)',log())[-1]);assert forward>=25
wait_for(lambda:not audio()[4] and audio()[1]>=forward,timeout=30)
start=len(log());key('k',True);time.sleep(1.2);key('k',False);back=targets(start);assert len(back)>=2,back
assert all(a-b==5 for a,b in zip(back,back[1:])),back
assert back[-1]>0,back
wait_for(lambda:not audio()[4] and back[-1]<=audio()[1]<=back[-1]+2,timeout=30)
seek_lines=[(int(t),int(active)) for t,active in re.findall(r'AUDIO state=\d+ seconds=(\d+).*?seeking=(\d+)',log()[start:])]
assert all(t in back for t,active in seek_lines if active),'Decoder replay leaked into displayed time'
assert audio()[2]==2 and log().count('PLAY_REQUEST')==2;frame('rewound-target.png');tap('z');wait_for(lambda:audio()[0]==3)
# More than 64 albums: revisiting the first page must use persistent thumbnails.
tap('Return');tap('Left');wait_for(lambda:len(list((game/'cover-cache-v1').glob('*.thumb')))>=6)
frame('library-first.png');start=len(log());key('Right',True);seen_end=False;deadline=time.monotonic()+55
while time.monotonic()<deadline:
 rows=[int(v) for v in re.findall(r'LIBRARY_SELECT index=(\d+)',log()[start:])]
 if 71 in rows:seen_end=True
 if seen_end and rows and rows[-1]<3:break
 time.sleep(.05)
key('Right',False);assert seen_end
current=int(re.findall(r'LIBRARY_SELECT index=(\d+)',log())[-1])
while current:tap('Left');current-=1
began=time.monotonic();wait_for(lambda:stats()[1]>0 and is_red(frame().getpixel((30,120))),timeout=4);reload_time=time.monotonic()-began
frame('library-return.png');wait_for(lambda:len(list((game/'cover-cache-v1').glob('*.thumb')))>=65)
shutil.copyfile(game/'JOE_MUSIC_LOG.txt',out/'cold-app-log.txt');cold=stats();files=len(list((game/'cover-cache-v1').glob('*.thumb')))
os.killpg(proc.pid,signal.SIGTERM);proc.wait(timeout=5);time.sleep(.4)
(game/'position.txt').write_text('ms0:/MUSIC/Album 000/01 Song.mp3\n')
launch('warm');tap('Return');wait_for(lambda:stats()[1]>=6 and is_red(frame().getpixel((30,120))),timeout=10);assert stats()[0]==0,stats();frame('library-warm.png')
assert 'AUDIO state=5' not in log();shutil.copyfile(game/'JOE_MUSIC_LOG.txt',out/'warm-app-log.txt')
report={'version':'0.18','eboot_sha256':hashlib.sha256((r/'EBOOT.PBP').read_bytes()).hexdigest(),'old_cover_flash_absent':True,'held_rewind_targets':back,'decoder_replay_hidden':True,'rewind_keeps_song_and_target':True,'persistent_thumbnail_files':files,'cold_cache_decodes_and_disk_hits':cold,'warm_boot_decodes_and_disk_hits':stats(),'first_page_reload_seconds':round(reload_time,3),'hardware_test':False}
(out/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
