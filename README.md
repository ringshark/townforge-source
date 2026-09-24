# Town Forge (2D source reference)

This is the current 2D version of Town Forge, an idle RPG built with C++ and
[raylib](https://www.raylib.com/). It's shared here as a reference for building
a 3D version.

## What's here

- `main.cpp` — the entire game (~8,600 lines, single file). Covers world
  state, save/load, town/craft/combat/wilderness screens, and all sprite
  rendering.
- `assets/` — the game's current 2D art (sprite sheets, icons, tiles,
  backgrounds) as actually used in the running game.

## Building it (desktop, Windows)

Requires a C++17 compiler and [raylib](https://www.raylib.com/) (6.0+).

```
g++ -O1 -std=c++17 -I <path to raylib includes> main.cpp -L <path to raylib libs> -lraylib -lopengl32 -lgdi32 -lwinmm -o townforge.exe
```

Run `townforge.exe` from a folder that has `assets/` next to it.

## Notes for reference

- Sprite sheets follow a common convention: 4 directions (Down/Left/Right/Up)
  x idle/walk/attack/cast columns, drawn via `DirSpriteSheet` /
  `DrawActorSprite` in `main.cpp`. Search for `DirSpriteSheet` to see how
  frames map to animations.
- Not every character has full 4-direction art — search for comments
  mentioning "mirrored" or "no usable native art" for known gaps.
