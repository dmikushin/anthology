.SUFFIXES: 

CC = gcc

CFLAGS = -g -O3 -std=c99 -ffast-math -pthread -Wall -DHAVE_CONFIG_H -I./include -I./include/debugger -I./include/ui/gtk -I./include/ui/scaler -I./include/peripherals -I./include/peripherals/disk -I./include/peripherals/flash -I./include/peripherals/ide -I./include/peripherals/nic -I./include/machines -I./include/pokefinder -I./include/sound -I./include/timer -I./include/unittests -I./include/z80 -I/usr/include/gio-unix-2.0/ -I/usr/include/cairo -I/usr/include/pango-1.0 -I/usr/include/atk-1.0 -I/usr/include/cairo -I/usr/include/pixman-1 -I/usr/include/libpng12 -I/usr/include/gdk-pixbuf-2.0 -I/usr/include/libpng12 -I/usr/include/pango-1.0 -I/usr/include/harfbuzz -I/usr/include/pango-1.0 -I/usr/include/freetype2 -DFUSEDATADIR="\"/usr/share/anthology\"" -I/usr/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT -I../compat -I/home/marcusmae/apc/leska/fuse-1.1.1/build/../install/include -I/usr/include/SDL `pkg-config --cflags gtk+-3.0`

CXXFLAGS = -std=c++11

C_SOURCES = $(wildcard src/*.c)
CPP_SOURCES = $(wildcard src/*.cpp)
OBJECTS = $(C_SOURCES:src/%.c=%.o) $(CPP_SOURCES:src/%.cpp=%.o)

all: anthology

anthology: $(OBJECTS)
	$(CXX) $(CFLAGS) $(CXXFLAGS) $(OBJECTS) -o $@ -lspectrum -lpng12 -lasound -lSDL -lz -lm -lrt `pkg-config --libs gtk+-3.0` -lX11 -lallegro_audio -lallegro_acodec -lallegro

%.o: src/%.c
	$(CC) $(CFLAGS) -c $< -o $@

%.o: src/%.cpp
	$(CXX) $(CFLAGS) $(CXXFLAGS) -c $< -o $@

clean:
	rm -rf *.o anthology

