# Animated animals - Quaternius "Ultimate Animated Animals" (CC0 1.0 Universal)

By Quaternius (https://quaternius.com). License: CC0 1.0 Universal (public
domain) - free for personal, educational and commercial use, no attribution
required (credit appreciated).

These are the pack's ShibaInu, Wolf, Horse, Deer, Stag and Fox models (via the
.glb conversions in github.com/sekaikx/woods, same meshes, materials and
animations), slimmed for the web build:
- only the clips the game uses are kept (Idle, Walk, Gallop, Attack /
  Attack_Headbutt, Death, Idle_HitReact1);
- animation channels that never change are stored as 2 keys;
- the remaining keyframes are thinned to every other key (the engine
  interpolates between them).

Used by the 3D wilderness (see DrawAnimal / WildAnimalsUpdateDraw in main.cpp).
