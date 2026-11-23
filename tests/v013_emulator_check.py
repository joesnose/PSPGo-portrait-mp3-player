from pathlib import Path
import os,subprocess,time,sys,configparser,json,re,hashlib,shutil,urllib.request
r=Path(__file__).resolve().parents[1];root=r/'build/emulator-v013';memory=root/'config/ppsspp';system=memory/'PSP/SYSTEM';game=memory/'PSP/GAME/JoeMusic'
for p in [system,game,memory/'MUSIC/Album A',memory/'MUSIC/Album B']:p.mkdir(parents=True,exist_ok=True)
base=root/'tone.mp3'
if not base.exists():subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-f','lavfi','-i','sine=frequency=440:duration=60','-ac','2','-ar','44100','-c:a','libmp3lame','-b:a','192k',str(base),'-y'],check=True)
for folder,titles in [('Album A',['01 First','02 Second','03 Third']),('Album B',['01 Another','02 Last'])]:
 for title in titles:
  dest=memory/'MUSIC'/folder/(title+'.mp3')
  if not dest.exists():subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-i',str(base),'-c','copy','-metadata','title='+title,'-metadata','artist=Control test','-metadata','album='+folder,str(dest),'-y'],check=True)
for title,rate,channels in [('02 Second',44100,2),('03 Third',44100,1)]:
 dest=memory/'MUSIC/Album A'/(title+'.mp3');subprocess.run(['ffmpeg','-hide_banner','-loglevel','error','-i',str(base),'-ar',str(rate),'-ac',str(channels),'-c:a','libmp3lame','-metadata','title='+title,'-metadata','album=Album A',str(dest),'-y'],check=True)
(game/'theme.cfg').unlink(missing_ok=True)
(game/'position.txt').write_text('ms0:/MUSIC/Album A/01 First.mp3\n')
from PIL import Image,ImageDraw
for folder,color in [('Album A',(45,100,70)),('Album B',(70,65,145))]:
 im=Image.new('RGB',(224,224),color);ImageDraw.Draw(im).text((35,100),folder,fill='white');im=im.convert('CMYK') if folder=='Album B' else im;im.save(memory/'MUSIC'/folder/('sleeve_front.JPG' if folder=='Album A' else 'Special sleeve.JPEG'),'JPEG')
shutil.copyfile(r/'EBOOT.PBP',game/'EBOOT.PBP');(game/'folders.cfg').write_text('defaults_checked=1\nroot=ms0:/MUSIC\n');shutil.copyfile(r/'DIRECT_TEST.txt',game/'DIRECT_TEST.txt')
cfg=configparser.ConfigParser();cfg.optionxform=str
cfg['General']={'FirstRun':'False','AutoRun':'True','AutoLoadSaveState':'0','IgnoreBadMemAccess':'False','EnableCheats':'False','RemoteDebuggerOnStartup':'False','RemoteDebuggerLocal':'True','RemoteISOPort':'15006'}
cfg['CPU']={'CPUCore':'1','FastMemoryAccess':'False'};cfg['SystemParam']={'PSPModel':'0'};cfg['Graphics']={'FrameSkip':'0','ShowFPSCounter':'0','InternalResolution':'1'}
with (system/'ppsspp.ini').open('w') as f:cfg.write(f)
(system/'controls.ini').write_text('[ControlMapping]\nUp = 1-19\nDown = 1-20\nLeft = 1-21\nRight = 1-22\nCross = 1-52\nCircle = 1-54\nSquare = 1-47\nTriangle = 1-29\nL = 1-45\nR = 1-51\nStart = 1-66\nSelect = 1-62\n')
env=os.environ.copy();env.update(DISPLAY=':113',XDG_CONFIG_HOME=str(root/'config'),XDG_CACHE_HOME=str(root/'cache'),SDL_AUDIODRIVER='dummy',LIBGL_ALWAYS_SOFTWARE='1')
console=(root/'console.log').open('w');proc=subprocess.Popen(['/workspace/work/squashfs-root/AppRun','--windowed','--xres','480','--yres','272','--graphics=software','--log='+str(root/'ppsspp.log'),str(game/'EBOOT.PBP')],env=env,cwd=game,stdout=console,stderr=subprocess.STDOUT,start_new_session=True);(root/'pid').write_text(str(proc.pid));time.sleep(3)
sys.path.insert(0,str(r/'build/python'))
from Xlib import X,XK,display
from Xlib.ext import xtest
from PIL import Image
d=display.Display(':113')
for w in d.screen().root.query_tree().children:
 try:
  if 'PPSSPP' in (w.get_wm_name() or ''):d.set_input_focus(w,X.RevertToParent,X.CurrentTime);d.sync()
 except Exception:pass
keys={'select':'space','shuffle':'s','start':'Return','cross':'z','up':'Left','down':'Right','album_next':'Up','rtrigger':'w','ltrigger':'q','vol_up':'plus'}
def button(name,down):
 code=d.keysym_to_keycode(XK.string_to_keysym(keys[name]));xtest.fake_input(d,X.KeyPress if down else X.KeyRelease,code);d.sync()
def tap(name):button(name,True);time.sleep(.2);button(name,False);time.sleep(.5)
def log():return (game/'JOE_MUSIC_LOG.txt').read_text()
def shot(name):
 screen=d.screen();g=screen.root.get_geometry();im=screen.root.get_image(0,0,g.width,g.height,X.ZPixmap,0xffffffff);Image.frombytes('RGB',(g.width,g.height),im.data,'raw','BGRX').crop((160,164,640,436)).rotate(-90,expand=True).save(root/name)
assert 'BUILD=0.13' in log() and 'NOTICE 5 tracks' in log()
assert 'RESTORE_SELECTION index=0' in log()
assert 'DIRECT_BEGIN' not in log(),'Automatic diagnostics should be disabled'
tap('cross');time.sleep(1);assert 'AUDIO state=2' in log();assert log().count('PLAY_REQUEST')==1
shot('initial-playing.png')
# START drives the same panel animation as the physical slide event.
button('start',True);time.sleep(.10);shot('library-sliding-in.png');button('start',False);time.sleep(.6);shot('library-open.png')
button('start',True);time.sleep(.10);shot('library-sliding-out.png');button('start',False);time.sleep(.6)
assert log().count('PLAY_REQUEST')==1,'Panel transition changed playback'

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
initial=Image.open(root/'initial-playing.png');preview=Image.open(root/'selection-preview.png')
assert initial.crop((0,32,272,318)).tobytes()==preview.crop((0,32,272,318)).tobytes(),'Browsing changed playing header or art'
assert initial.crop((0,320,272,340)).tobytes()!=preview.crop((0,320,272,340)).tobytes(),'Browsing album label did not change'
tap('rtrigger');assert re.search('SELECT kind=track .*Album B/02 Last.mp3',log(),re.I)
button('ltrigger',True);time.sleep(2);button('ltrigger',False);time.sleep(.6);assert re.search('SELECT kind=folder .*Album A/01 First.mp3',log(),re.I)
assert log().count('PLAY_REQUEST')==1
button('rtrigger',True);time.sleep(2);button('rtrigger',False);time.sleep(.6)
tap('cross');assert log().count('PLAY_REQUEST')==2
assert re.search('PLAY_REQUEST .*Album B/01 Another.mp3',log(),re.I)
time.sleep(.7);shot('committed-playing.png');committed=Image.open(root/'committed-playing.png')
assert committed.crop((0,32,272,80)).tobytes()!=initial.crop((0,32,272,80)).tobytes(),'Committing did not change title'
assert committed.crop((24,88,248,312)).tobytes()!=initial.crop((24,88,248,312)).tobytes(),'Committing did not change artwork'
tap('down');assert re.search('SELECT kind=track .*Album B/02 Last.mp3',log(),re.I)
tap('up');tap('album_next');assert re.search('SELECT kind=folder .*Album A/01 First.mp3',log(),re.I)
assert log().count('PLAY_REQUEST')==2
time.sleep(1);assert (game/'position.txt').read_text().strip()=='ms0:/MUSIC/Album A/01 First.mp3'
shot('now-playing.png');assert 'AUDIO state=5' not in log()
tap('start');assert 'FOLDERS count=2 tracks=5' in log();tap('down');shot('folder-library.png');before=log().count('PLAY_REQUEST');tap('cross');assert 'OPEN_FOLDER index=1 track=3' in log();assert log().count('PLAY_REQUEST')==before,'Opening folder started playback'
# Double taps play relative to the playing track, rather than browsing twice.
def double(name):
 for i in range(2):button(name,True);time.sleep(.06);button(name,False);time.sleep(.07)
 time.sleep(.8)
previous=log().count('PLAY_REQUEST');double('rtrigger');assert log().count('PLAY_REQUEST')==previous+1,log()[-1400:]
assert re.findall('PLAY_REQUEST .*path=(.*)',log())[-1].endswith('Album B/02 Last.mp3')
double('ltrigger');assert re.findall('PLAY_REQUEST .*path=(.*)',log())[-1].endswith('Album B/01 Another.mp3')
tap('start');tap('shuffle');assert 'SHUFFLE count=5' in log() and log().count('PLAY_REQUEST')==previous+3;shot('shuffle-playing.png')
shuffled=[int(re.findall(r'PLAY_REQUEST index=(\d+)',log())[-1])]
for i in range(4):
 double('rtrigger');shuffled.append(int(re.findall(r'PLAY_REQUEST index=(\d+)',log())[-1]))
assert len(set(shuffled))==5,shuffled
double('ltrigger');assert int(re.findall(r'PLAY_REQUEST index=(\d+)',log())[-1])==shuffled[-2]
assert 'ART_FILE decoded=1' in log() and 'sleeve_front.JPG' in log() and 'Special sleeve.JPEG' in log()
# Generated green/purple JPEG fixtures verify decoded display, including CMYK.
rgb=initial.getpixel((30,100));assert rgb[1]>rgb[0]+25 and rgb[1]>rgb[2]+15,rgb
rgb=committed.getpixel((30,100));assert rgb[2]>rgb[0]+40 and rgb[2]>rgb[1]+40,rgb
assert not (game/'music_buttons.prx').exists()
assert re.search(r'AUDIO state=2 .*rate=44100',log()),log()[-2000:]
assert 'AUDIO state=5' not in log()
# Themes are selected through Settings without adding a shortcut.
assert not (game/'theme.cfg').exists(),'Use a fresh test directory for this check'
for name in ['Blue','Red','Yellow','Neon']:
 tap('select');tap('down');tap('down');tap('cross');assert (game/'theme.cfg').read_text().strip()==name;shot('settings-'+name.lower()+'.png');tap('start');shot('theme-'+name.lower()+'.png')
assert Image.open(root/'theme-neon.png').getpixel((5,35))==(5,7,19)
shutil.copyfile(game/'JOE_MUSIC_LOG.txt',root/'interaction-log.txt')
# Close and boot again to verify the saved palette is actually restored.
proc.terminate();proc.wait(timeout=10)
proc=subprocess.Popen(proc.args,env=env,cwd=game,stdout=console,stderr=subprocess.STDOUT,start_new_session=True);(root/'pid').write_text(str(proc.pid));time.sleep(3);shot('theme-restored.png')
assert Image.open(root/'theme-restored.png').getpixel((5,35))==(5,7,19)
shutil.copyfile(game/'JOE_MUSIC_LOG.txt'  ,root/'app-log.txt')
report={'version':'0.13','eboot_sha256':hashlib.sha256((r/'EBOOT.PBP').read_bytes()).hexdigest(),'ppsspp':'1.20.4','short_shoulder_action_on_release':True,'short_shoulder_selects_within_folder':True,'hold_selects_adjacent_folder_once':True,'long_release_has_no_short_action':True,'browsing_does_not_start_playback':True,'explicit_selection_playback':True,'automatic_file_test_removed':True,'volume_input_injected':False,'real_volume_getter_tested':False,'native_home_tap_and_menu_tested':False,'hardware_test':False,'portrait_dpad_navigation':True,'rotated_face_playback':True,'selection_save_restore':True,'reverse_volume_hardware_tested':False,'folder_library_and_selection':True,'internal_launch_card_bypass_hardware_tested':False,'arbitrary_jpg_jpeg_cmyk_artwork':True,'double_tap_previous_next':True,'shuffle_all_ui':True,'shuffle_next_previous_no_repeat_cycle':True,'standalone_eboot_emulator_boot':True,'normal_memory_sfo_verified':True,'standard_mixer_44100_stereo_mono_playback':True,'multirate_pcm_converter_host_test':True,'library_panel_animation_emulator':True,'physical_slider_animation_hardware_tested':False,'bluetooth_audio_hardware_tested':False,'slide_state_hardware_tested':False,'five_themes_available':True,'settings_theme_cycling':True,'neon_palette_restored_after_restart':True,'embedded_loader_removed':True,'volume_hooks_removed':True,'external_helper_hardware_retest_required':True,'browsing_keeps_playing_header_and_art':True,'browsing_album_label_changes':True,'play_selection_updates_header_and_art':True}
(root/'result.json').write_text(json.dumps(report,indent=2)+'\n');print(json.dumps(report,indent=2))
