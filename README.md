**Note that this is still work-in-progress, PR's welcome!**

# top2ogg

top2ogg is a tool to convert Topheavy video files (*.top) to *.ogg video files.

These kind of files are used in the 2004 classic "THE GuY GAME" (video-game for PC, PS2 and Xbox).
The format internally uses Vorbis for audio and DivX for video.
*.top appears to be a custom-made container-format by Topheavy Studios, the developer of the game.

## Dependencies

* libogg
* libvorbis

## Compilation

```
clang -O0 -g convert.c `pkg-config --libs --cflags ogg vorbis` -o top2ogg
```

## Usage

To convert ggintro.top to ggintro.ogg:

```
top2ogg ggintro.top ggintro.ogg
```

## License

Licensed under GPLv2 (Version 2 only).
Refer to the LICENSE file included.
