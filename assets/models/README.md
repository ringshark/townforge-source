# 3D building models — Quaternius Medieval Village MegaKit (CC0 1.0 Universal)

Source: https://quaternius.com/packs/medievalvillagemegakit.html
(via the CC0 glTF mirror at https://github.com/j-ponzo/gltf-medieval-village-megakit)
License: CC0 1.0 Universal — free for personal, educational and commercial use,
no attribution required. Full license text: see the LICENSE file at the mirror repo.

The kit is modular (walls/roofs/props snap to a 2m grid); main.cpp assembles one
house per town building from these pieces (see Town3DDrawHouse/Town3DDrawBuilding).

Textures were downscaled from 2048px to 512px to keep the web build small.
Total: 12 glTF models (+ .bin) + 19 PNGs = ~4.5MB.

## Props pass additions (2026-09-24) — all CC0, ~0.2MB

- Kenney Nature Kit (https://kenney.nl/assets/nature-kit) — CC0:
  tree_oak.glb, tree_pineDefaultA.glb, tree_detailed.glb, tree_default.glb,
  tree_fat.glb, plant_bush.glb (self-contained GLBs, vertex-colored).
- KayKit Dungeon Remastered 1.0
  (https://github.com/KayKit-Game-Assets/KayKit-Dungeon-Remastered-1.0) — CC0:
  barrel_small.glb (renamed from barrel_small.gltf.glb), chest.glb
  (self-contained GLBs, vertex-colored).
- Quaternius Medieval Village MegaKit (same kit/source as above):
  Prop_WoodenFence_Single.gltf/.bin, Prop_WoodenFence_Extension1.gltf/.bin
  (uses the T_WoodTrim_* textures already vendored).

Note: the Quaternius Stylized Nature MegaKit and Fantasy Props MegaKit were the
first choice for trees/lamps/stalls, but both are itch.io "name your own price"
downloads (free, but the $0 checkout requires an email address). Kenney/KayKit
direct downloads were used instead — same stylized low-poly look, zero friction.
If Mark wants the exact Quaternius trees/lanterns later, grab
https://quaternius.itch.io/stylized-nature-megakit and
https://quaternius.itch.io/fantasy-props-megakit (Standard zips are free).
