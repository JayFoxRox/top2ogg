**Note that this is still work-in-progress, PR's welcome!**

# top2avi

top2avi is a tool to convert Topheavy video files (*.top) to *.avi video files.

These kind of files are used in the [2004 classic "THE GuY GAME"](https://en.wikipedia.org/wiki/The_Guy_Game) (video-game for PC, PS2 and Xbox).
The format internally uses Vorbis for audio and DivX for video.
*.top appears to be a custom-made container-format by Topheavy Studios, the developer of the game.

## Dependencies

* FFmpeg A/V libraries

## Compilation

```
clang -O0 -g convert.c `pkg-config --cflags --libs libavformat libavcodec libavutil libswresample libswscale` -lm -o top2avi
```

## Usage

To convert ggintro.top to ggintro.avi:

```
top2avi ggintro.top ggintro.avi
```

## License

Licensed under GPLv2 (Version 2 only).
Refer to the LICENSE file included.
