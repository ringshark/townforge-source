# Cohesive environment rendering

Implemented against `ringshark-patch-1` at `9328723`, on local branch
`codex/cohesive-2d-environments`. This is the current landscape game, including
its compact equipped hero and combat animations. The September reference source
in the original checkout is not the source of this build.

- Four outdoor towns retain their layouts and identities, with stable broad
  ground variation, shared contact shadows and a warmer exterior art tint.
- All six dungeons use related stone materials with individual palettes,
  darker walls, ground detail, shaded exposed edges and fixture light. Corridor
  joins remain open. Props remain along room edges, outside the entrance route.
- All thirteen interiors use world-aligned floor detail, wall and furnishing
  contact shadows, warm fixture light and cooler window light. Existing room
  layouts, furniture, station interactions and camera behavior are retained.
- Exterior loaders remove edge-connected white mattes, while retaining the
  existing low-alpha cleanup for Frostmere and Cragmoor. The asset audit also
  checks older building sets and town scenery. White plaster inside silhouettes
  is retained.

`building-transparency.json` records all 58 inspected images and no suspected
white backgrounds. `browser-results.json` records desktop and phone viewport
checks. The PNGs show the corresponding current-version views.

Build using the repository's existing `web/build_web.sh` with raylib 6.0 and
Emscripten. In this workspace the relocatable Debian compiler requires
`source /workspace/.onboarding/tools/web-toolchain/env.sh`; set
`EMCC_CFLAGS=-sMINIFY_HTML=0`, `RAYLIB_DIR=/workspace/.onboarding/tools/raylib`,
and `TF_VARIANT=2d`. The packaged browser build is in `/workspace/townforge-current/site`.
The review entry point is separate and is not included in that shipping build.

The build is local; it has not been published to GitHub Pages. Production
online configuration is not copied into the review build.

Validation completed:

- 46 Chromium scene checks: four towns, six dungeons and thirteen interiors at
  desktop and phone viewport sizes, with live room movement and no exceptions.
- Packaged production game: desktop day/night and mobile touch menu, all eighteen
  town art assets, camera movement, minimap content and a 128 MiB startup heap.
- Existing road-clipping and actor-size/clearance tests, build-variant checks,
  landscape shell checks, and native landscape layout/interaction tests passed.
- New six-palette material test verifies deterministic textures, opacity,
  floor/wall contrast, open corridor seams and texture cleanup.
- Phone checks use Chromium touch emulation; these are not physical iPhone or
  Safari/WebKit validation.
