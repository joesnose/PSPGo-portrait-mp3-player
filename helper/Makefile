TARGET = music_buttons
OBJS = main.o
BUILD_PRX = 1
USE_KERNEL_LIBS = 1
USE_KERNEL_LIBC = 1
PRX_EXPORTS = exports.exp
CFLAGS = -O2 -G0 -Wall -Wextra
LIBS = -lpspctrl_driver -lpspsdk
PSPSDK := $(shell psp-config --pspsdk-path)
include $(PSPSDK)/lib/build.mak

main.o: main.c
