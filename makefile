.SUFFIXES: 

CC = gcc

CFLAGS = -DHAVE_CONFIG_H -I./include -I./include/ui/gtk -I./include/debugger -I./include/ui/scaler -I./include/peripherals -I./include/peripherals/disk -I./include/peripherals/flash -I./include/peripherals/ide -I./include/peripherals/nic -I./include/machines -I./include/pokefinder -I./include/sound -I./include/timer -I./include/unittests -I./include/z80 -pthread -I/usr/include/gtk-2.0 -I/usr/lib/x86_64-linux-gnu/gtk-2.0/include -I/usr/include/gio-unix-2.0/ -I/usr/include/cairo -I/usr/include/pango-1.0 -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/pixman-1 -I/usr/include/libpng12 -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/libpng12 -I/usr/include/pango-1.0 -I/usr/include/harfbuzz -I/usr/include/pango-1.0 -I/usr/include/glib-2.0 -I/usr/lib/x86_64-linux-gnu/glib-2.0/include -I/usr/include/freetype2 -DFUSEDATADIR="\"/home/marcusmae/apc/leska/fuse-1.1.1/build/../install/share/fuse\"" -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT -I../compat -I/home/marcusmae/apc/leska/fuse-1.1.1/build/../install/include -I/usr/include/SDL  -g -O2 -pthread -Wall

SOURCES = $(wildcard src/*.c)
OBJECTS = $(SOURCES:src/%.c=%.o) scalers32.o

all: fuse

fuse: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@ -lspectrum -lpng12 -lasound -lSDL -lz -lm -lrt `pkg-config --libs gtk+-2.0` -lX11

%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

scalers.o: src/scalers.c
	$(CC) $(CFLAGS) -DSCALER_DATA_SIZE=2 -c $< -o $@

scalers32.o: src/scalers.c
	$(CC) $(CFLAGS) -DSCALER_DATA_SIZE=4 -c $< -o $@

clean:
	rm -rf *.o fuse

