## Anthology

Simplification of ZX Spectrum Fuse emulator with menu and joystick support for playing old games with kids.

<img src="https://github.com/dmikushin/anthology/blob/master/screen.jpg" width="300" />

```
$ sudo apt-get install gcc cmake spectrum-roms fuse-emulator-common libasound2-dev libspectrum-dev libgtk-3-dev libsdl1.2-dev liballegro5-dev
$ git clone https://github.com/dmikushin/anthology.git
$ cd anthology
$ mkdir build
$ cd build
$ cmake ..
$ make -j48
$ ./anthology
```

Main screen uses 8-bit electromusic by [Yerzmyey](http://yerzmyey.i-demo.pl/):

 * [Evelynn (ATARI ST / ZX SPECTRUM)](http://yerzmyey.i-demo.pl/death_squad/04_Yerzmyey-Evelynn.mp3)
 * [A very odd adventure (ZX Spectrum)](http://yerzmyey.i-demo.pl/chiptunes/03_Yerzmyey-A_very_odd_adventure.mp3)

