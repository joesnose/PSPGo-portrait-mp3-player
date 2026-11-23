# Joe's Portrait Music — PSP Go 0.15 — return to playing and saved position

A native MP3 player for ARK on 6.61. Hold the console **90 degrees clockwise** for the 272 × 480 portrait interface.

## Install or update

Version 0.15 returns to the actual playing track when the slider closes, shows the artist below the artwork, and returns after four seconds of idle song browsing. It retains the audio, helper, themes and album Library from the working builds.

Replace **both `EBOOT.PBP` and `music_buttons.prx`** in your existing `PSP/GAME/JoeMusic` directory. Keep `folders.cfg` and `position.txt`. Do not register the helper in seplugins or PLUGINS.TXT. It is now a read-only HOME/volume getter: no firmware patches, input hooks, volume setters or background threads.

Existing v0.12–v0.14 users only need to replace the EBOOT; the same working slide-switch helper is used. The included helper remains free of patches, input hooks and volume setters. Normal volume direction, library, shuffle, shoulder double taps, JPEG discovery and player layout are retained.

If startup still fails, rename `music_buttons.prx` to `music_buttons.off`, or place an empty `SAFE_MODE.txt` beside the EBOOT. The player will skip helper loading, use normal HOME/volume handling and allow physical Circle for play/pause. No uploaded log is required to try this. Startup screens distinguish decoder, HOME-control and storage setup.

On an internal-storage launch, ARK can redirect `ms0:` to internal storage. The player still probes the physical Memory Stick FAT driver (`fatms0:/`) when available. Internal-launch card access remains unconfirmed on hardware; use the Memory Stick if internal storage causes trouble.

The player restores your last selected song/folder with its artwork and track list. The selection is saved after browsing settles, periodically while playing, and immediately before normal exit. `position.txt` is written via a temporary file and backup so replacing an existing file also works on PSP FAT filesystems. A backup can be recovered if the main file is missing. Existing one-line position files remain supported. No music starts automatically.

The save file now also records the active track, playback seconds and pause state separately from the browsing selection. This build restores the highlighted song/folder; time-based audio resuming is not enabled yet.

The player starts normally. An old `DIRECT_TEST.txt` no longer opens the diagnostic automatically. Square on Music folders still opens the optional exact-file diagnostic.

## Sliding Library

The physical screen-position reading was confirmed on the PSP Go with v0.12. Opening the slider now opens Library with a slide-in animation from the **right of the portrait screen**. Closing it slides Library back out to the right and returns to the **currently playing track**, clearing a pending folder preview and centring that track in the song list. If nothing is playing, the saved folder preview can remain visible. Music continues; selection/playback is not changed by moving the slider. An open screen at startup also brings Library in.

The animation lasts about 240 ms and reverses from its current position if the slider changes quickly. It is triggered by the binary open/closed sensor; the sensor does not measure continuous physical travel. START and the existing navigation still work, using the same Library transition; automatic navigation happens on a sensor state change. A close event also takes precedence over stale button input from that frame. The diagnostic screen-position text has been removed from Settings. Sensor reading and the Library animation remain active; unknown readings do not trigger navigation.

## Themes

Press **SELECT** to open Settings, highlight **Theme**, and press physical **Circle** to cycle:

- **Sage** — the original colours and default.
- **Blue** — muted slate and soft blue.
- **Red** — muted burgundy and dusty rose.
- **Yellow** — muted charcoal olive and soft gold.
- **Neon** — near-black/navy with bright cyan and magenta accents.

The player, library, settings, browser, volume overlay and placeholder artwork follow the palette. Actual album artwork keeps its own colours. Each choice takes effect immediately and saves to `theme.cfg` beside the EBOOT. Switching does not interrupt playback. No new button shortcut is needed. Invalid or missing theme settings use Sage.

## Controls

Button names refer to the original printed labels, with the PSP held clockwise.

| Control | Action |
|---|---|
| L / R short press | Select previous / next song within the current folder, wrapping at the ends |
| L / R double tap (within 350 ms) | Play previous / next song in the playback queue |
| L / R hold for 0.8 seconds | Select the first song in the previous / next folder, wrapping at the ends |
| HOME short press | Play the selected song; pause/resume if already active |
| HOME hold for 0.8 seconds | Open the native PSP HOME / exit menu |
| Circle | Play/pause on the player; open a folder or play a file elsewhere |
| Original D-pad Left / Right | Up / down in portrait lists |
| Triangle | Back / parent folder |
| START | Folder library / Now Playing |
| SELECT | Settings: music folders and Theme |
| Cross in browser | Add current folder and its descendants |
| Cross in selected roots | Remove that root |
| Cross on Now Playing | Stop |
| Volume + / − | Normal firmware direction; never changes tracks |
| D-pad portrait Up / Down | Select previous / next song in the current folder |
| D-pad portrait Left / Right | Select previous / next folder |
| Square in Library | Start Shuffle all music; press again in Library to disable it |
| Square elsewhere | Finish browsing / optional diagnostic |

**Single-tap and held L/R browsing only changes the selection; double taps change playback.** Music keeps playing until you press HOME or Circle. Short shoulder presses act on release; a long press changes folder once and does not also move one song when released. “Album” means the containing folder, ordered by path, rather than grouping ID3 album tags.

Volume changes show a temporary bar and measured level out of 30 when the helper is available. **Volume reversal remains disabled.** No firmware controller imports are changed and the helper does not set volume.

Face-button actions have moved one position counterclockwise: physical Circle performs former Cross actions, Triangle performs former Circle actions, Cross performs former Square actions, and Square performs former Triangle actions. The D-pad directions above refer to the clockwise portrait orientation: physical Left/Right scroll songs; physical Down/Up scroll folders.

Opening a Library folder, or using folder navigation on the player, enters folder preview: the top text and artwork now describe that folder and the highlighted track. Its artwork remains cached as you scroll MP3s. Playback continues independently until HOME or physical Circle commits the selection. Once playing, ordinary song browsing retains the playing track artwork until another folder is explicitly opened. Below the artwork, the browsed album name and five cyclic song rows show your selection independently. The centre song is highlighted in a larger 16-pixel font; it animates towards the centre when browsing. The bottom contains only the timer at left and a small play/pause symbol at right; transport buttons and HOME/START/SELECT hints have been removed. Small folders repeat songs around the drum. Root-level files directly inside MUSIC or PSP/MUSIC use original cassette/mixtape artwork. Albums without embedded or folder artwork use an original glass-style music-note icon; neither icon requires external assets or attribution.

## Four-second return and artist display

When a track is playing or paused, scrolling the player starts a four-second idle timer. Each further scroll restarts it. When it expires, the display returns to the active song, artwork and list position without restarting audio. Opening a folder starts the same timer; Library and Settings are not forcibly dismissed by it. Selecting a song to play cancels the pending return. The active track is looked up when returning, so automatic next-track playback cannot send the display back to an old song.

The line immediately below the artwork now shows the artist for the displayed playing track or folder-preview selection. Missing tags show `Unknown artist`.

## Library and Shuffle all

START shows one row per folder containing MP3s, with its full path to distinguish duplicate names and storage devices. Rows now have 44 × 44 album-art tiles and larger matching folder icons when no cover is available. Six larger rows fit on screen; the list scrolls normally. Thumbnails use embedded or folder artwork and a bounded cache, without replacing the full-size player image. Select a folder with portrait Up/Down and press HOME or Circle to open its MP3-only list and preview its artwork on the player. Nested folders are separate Library entries; the player list stays within the selected directory. Opening a folder does not start music; press HOME or Circle again to play your chosen song. Nested folders with tracks have their own entries. Selected music roots and recursive scanning are unchanged.

**Physical Square on Library** starts a shuffled queue containing every track across all selected folders. Each track appears once before the queue wraps. Double-tap L/R and automatic next-track follow that queue; previous returns through the same order. Press Square again on Library to turn shuffle off without interrupting the current track. Refreshing the library also clears shuffle. Shuffle mode is not persisted across launches.

## Choose music folders

Press SELECT, choose Add music folder, open a directory on `ef0:` or `ms0:`, and press Cross to add the directory shown at the top. All nested folders are included. Repeat to add more roots, then Triangle to rescan. Cross on a selected root removes it. Existing choices persist across upgrades; overlapping roots do not duplicate tracks.

Limits: 16 roots, 20,000 MP3s/browser entries, paths below 1,024 bytes and 64 directory levels. Playback is MP3-only. The directory browser now shows folders and MP3 files only; images, text files and unsupported audio types are hidden. Automatic next-track playback remains enabled.

## Version 0.15 verification

Host gesture tests passed for tap/hold distinction, single long actions, cancellation, timer wrap and folder/track navigation. PPSSPP 1.20.4 checks passed for shoulder browsing without interrupting playback, folder changes, explicit selected-song playback and normal startup with an old diagnostic file present. Portrait D-pad navigation, rotated face-button playback and selection persistence were also checked. Screenshot checks verify that opening another folder previews its art and that this does not start playback. Return-to-playing from a different preview, the four-second reset after more scrolling, artist display and cancellation after choosing a song are checked. Host tests cover the idle deadline/wrap and saved session round-trips, FAT replacement and backup/legacy recovery. MP3-only browser filtering and album thumbnails with an uncovered-folder fallback are also checked. Folder-library navigation and opening a folder without starting playback also passed in PPSSPP. Sanitized host checks cover unique nested/device folder groups, shuffle permutations and tap/hold/double-tap timing. Double-tap playback and shuffled queue navigation were checked. Mixed-case, arbitrarily named JPEGs and a CMYK JPEG were displayed and verified in screenshots. The screenshots and validation report use generated test audio.

Theme switching and restoring Neon after a fresh emulator launch passed. A sanitized host check covers saving all five themes, FAT-style rename replacement and invalid-config fallback. The normal-memory build is checked with PPSSPP configured as a PSP-1000, including standard-mixer playback of stereo and mono 44.1 kHz MP3s, artwork, shuffle and theme persistence. Host PCM tests cover 11.025/22.05/32/44.1/48 kHz conversion, exact duration, mono duplication, chunk continuity and tone frequency. Panel timing tests cover reversal and clock wrap; emulator frames of the visible transition were reviewed. The packaged SFO memory request is also inspected directly. Host firmware mocks check slide open/closed interpretation, unknown-model/error fallback and K1 restoration. **Bluetooth output and the physical-switch-triggered animation require your PSP Go test.** The switch reading itself was already confirmed in v0.12.

## Artwork and tags

Common ID3v2.3/v2.4 title, artist and album tags are read. Embedded JPEG/PNG is tried first. The folder search now checks cover/folder/front/album names with `.jpg`, `.jpeg` or `.png`, then discovers arbitrary image filenames case-insensitively in the same directory. Cover/front names are preferred; back/disc images have lower priority. Discovery examines at most 64 image candidates. Standard RGB and CMYK JPEG covers are supported. Valid art takes precedence over the root-directory mixtape placeholder. Missing or unsupported art gets a placeholder. The interface uses antialiased DejaVu Sans glyphs, including common Latin accented characters; characters outside that set display as question marks. Compressed/unsynchronised ID3 tags are not fully supported. PNG artwork is limited to 1,024 × 1,024; JPEG decoding uses scaled output; compressed artwork is capped at 4 MiB.

## Validation and current limits

- Built with PSPDEV/PSPSDK; install EBOOT and kernel helper compiled successfully.
- Native PSP directory-adapter regression fixtures cover mode-less directory attributes, hidden/read-only/archive directory flags, direct MUSIC files, FAT path comparisons and a driver that allows only one directory handle at a time.
- AddressSanitizer/UndefinedBehaviorSanitizer host checks cover recursive/overlapping folders, symlink loops, settings round-trips, tag/art bounds, malformed tags and long-press timing including timer wrap.
- PPSSPP checks cover MP3 playback, pause/resume, track switching, automatic next-track, recursive scanning, multiple folder selection through the UI, and embedded PNG/folder JPEG artwork.
- The standard mixer channel is drained before release/re-reservation; alternating output buffers preserve queued sample data.
- The new HOME tap/hold handling and measured volume indicator require real PSP Go testing; PPSSPP cannot certify these firmware features. Suspend/resume is not promised by this prototype.
- Playback resets the standby timer while active. No seek, shuffle, EQ or streaming in this first version.

For a problem report, attach `JOE_MUSIC_LOG.txt` from the app folder. It is replaced each launch.

## Build

With a current PSPDEV toolchain and PSP ports of libpng, libjpeg and zlib:

```
make -C helper
make -j2
```

Or from this source folder using the same Docker toolchain:

```
docker run --rm -v "$PWD:/src" -w /src pspdev/pspdev:latest sh -c 'make -C helper && make -j2'
```

Portable checks (Linux):

```
cc -std=gnu11 -g -O1 -fsanitize=address,undefined -fno-omit-frame-pointer -Isrc src/core.c src/fs.c tests/core_test.c -o /tmp/joemusic-core-test
/tmp/joemusic-core-test
```

Native PSP adapter test (host mock of directory metadata; no physical PSP):

```
cc -std=gnu11 -g -O1 -fsanitize=address,undefined -D__PSP__ -Itests/mock -Isrc src/core.c src/fs.c tests/native_directory_test.c -o /tmp/joemusic-native-dir-test
/tmp/joemusic-native-dir-test
```

`src/ui_fonts.c` is included, so font generation is not required to build. To regenerate it, run `python3 tools/generate_fonts.py` with Pillow and DejaVu Sans installed; see the bundled font license.

The emulator scripts are development harnesses with workspace-specific PPSSPP/X11 paths. Their generated MP3s and artwork are test fixtures and are not part of the release.

## Credits and license

The player is BSD-2-Clause licensed. The firmware MP3 pipeline is based on Alexander Berl's 2008 BSD-licensed [PSPSDK MP3 sample](https://github.com/pspdev/pspsdk/tree/master/src/samples/mp3); the interface font is rasterized from DejaVu Sans. See `licenses/PSPSDK-LICENSE` and `licenses/DejaVu-font-LICENSE.txt`. Uses the PSP firmware MP3 decoder, libpng, libjpeg and zlib; third-party notices are included in the release. ARK's published KUBridge/SystemCtrlForUser interfaces are used to load the application helper and resolve its syscall.
