TARGET = joesnose_music
OBJS = src/main.o src/theme.o src/core.o src/fs.o src/controls.o src/marquee.o src/session.o src/probe.o src/player.o src/mp3_info.o src/pcm.o src/slide_panel.o src/gfx.o src/ui_fonts.o src/cfw.o
BUILD_PRX = 1
PSP_LARGE_MEMORY = 0
CFLAGS = -MMD -MP -O2 -G0 -Wall -Wextra -Wno-misleading-indentation -std=gnu11
ASFLAGS = -G0
LIBS = -lpng -ljpeg -lz -lpspmp3 -lpspaudio -lpsppower -lm
EXTRA_TARGETS = EBOOT.PBP
PSP_EBOOT_ICON = ICON0.PNG
PSP_EBOOT_TITLE = Portrait Player
PSPSDK := $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

# Refresh memory and title metadata whenever build settings change.
PARAM.SFO: Makefile

# Rebuild every consumer when shared playback/UI headers change.
-include $(OBJS:.o=.d)
