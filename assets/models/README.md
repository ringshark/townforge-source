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

## Wilderness 3D additions (2026-09-24) — all CC0, ~0.05MB

- Kenney Nature Kit v2.1 (https://kenney.nl/assets/nature-kit) — CC0 1.0
  Universal (see License.txt in the pack; verified at download time):
  rock_largeA.glb, rock_largeB.glb, rock_largeC.glb (ore-vein clusters +
  mountain scatter), rock_smallA.glb, rock_smallB.glb, rock_smallC.glb
  (small scatter), stump_roundDetailed.glb (wood-gather node marker).
  Self-contained GLBs, vertex-colored via material baseColorFactor.

## Dungeons 3D additions (2026-09-24) — no new asset files, ~0MB

The dungeon 3D view reuses the existing 2D themed tiles and bakes everything
else procedurally at runtime:

- `assets/dungeon_themed/<theme>_{floor,wall}.png` (already vendored for the 2D
  dungeon view; CC0 — lava/volcanic, orc-camp, tomb, rock, and mine themes per
  dungeon, see the "Dungeon Tileset" art entry in main.cpp's asset comments)
  are stamped into the baked 3D floor texture and onto the merged wall mesh.
- `assets/dungeon_themed/sunkencrypt_water.png` (Sunken Crypt boss-room pool)
  and `assets/dungeon_themed/hollowwarrens_rug.png` (Hollow Warrens boss-room
  rug) are baked into the floor texture at the same rects the 2D view uses.
- Torch flames are a procedural 64x64 radial-gradient sprite baked at runtime
  (`Dungeon3DEnsureTorch`); the wall mesh is merged geometry built from
  `kDungeonRoomLayouts` at runtime. Lighting is the new
  `assets/shaders/torchlight.vs/.fs` (written for this phase, dark ambient +
  flickering point lights, no shadowmaps indoors).

## Phase 3 creatures (2026-09-24) — 100% procedural, zero asset files

No downloaded creature models are used. The KayKit Adventurers pack (the first
choice for humanoid player/NPC models) could not be fetched from this
environment — https://github.com/KayKit-Game-Assets/KayKit-Adventurers-1.0
returned 404 and kaykit.com yielded nothing usable — and no unverified-license
asset was going to ship, so per the project directive ("do NOT leave creatures
as placeholders — build a PROCEDURAL CREATURE KIT in code instead") every
creature, player avatar, NPC, rival, innocent and companion in the town,
wilderness and dungeon 3D views is built at runtime by the procedural creature
kit in main.cpp (section "T3C-KIT", between the vector helpers and the
orbit-camera state).

What the kit builds (flat-shaded faceted primitives, CC0-by-construction —
there is no third-party artwork involved, so no license to verify):
- 11 quadruped archetypes: canine, feline, sabertooth, bulky, horned/bison,
  equine, winged dragon, winged griffin (beaked), wyvern, bat, serpent
- 2 humanoid archetypes: traveler (player/NPCs/rival/innocents/goblins/imps/
  bandits/undead/wraiths) and bulky orc (orcs/trolls/brutes)
- Articulated procedural animation driven by per-instance live speed tracking:
  diagonal-pair trot (FL+BR / FR+BL), idle bob, occasional head turns,
  counter-swinging arms, tail sway, bird-pattern wing flap, serpent slither

Draw-call budget: far quadrupeds (>750 units) and the shadow pass use a single
merged rest-pose mesh (1 draw per creature); near creatures use ~5-9 draws.
The 2D game and all game logic are untouched — the 3D views only read live
positions and facing from the existing simulation state.
