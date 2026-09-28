# Lighting follow-up to the new character/building art

Based on source `2aa7fc03` on `ringshark-patch-1` and preview `2630f030`.
This is a targeted shader pass, not a replacement for the new models.

## Review

The latest source adds nine hero looks, equipment-specific weapons, rigged
monsters, better building walls/windows and scenery beyond the map boundary.
The in-game town view shows much more character and architectural detail than
the older preview. The new warm windows also give buildings a clear nighttime
identity.

The shared lighting still gave all surfaces the same strong white highlight,
and the dark hero's shadow-facing detail was hard to read against the ground.
The terrain water shader generated glints from ripple brightness without
considering the viewing direction. These are the focus of this pass.

## Implemented

- Softer, lower-intensity surface highlights to keep texture detail visible.
- A small increase in shadow-side ambient fill, leaving upward-facing ground
  and directly lit faces at their existing ambient level.
- Subtle sky reflection at grazing angles, scaled by the existing ambient
  lighting so it follows the day/night cycle.
- Direction-dependent glints on terrain water; this does not replace the
  separate fountain geometry/material.
- High precision in the lighting fragment shader where available, an ES2
  fallback, and safe handling for zero-length fragment normals.
- Normal vectors use homogeneous w=0 in standard, foliage and torch vertex
  shaders. The new procedural monster gait and emissive windows are retained.

## Verification

All five shader pairings compile and link in a WebGL1 context: lit, foliage,
grass, ground and torchlight. The current game also boots and renders with the
new assets. Daylight and night town views were visually inspected. Local before/after captures used the same save-free start and camera; NPC
positions and animation frames can differ. Screenshots are not included in this
repository because automatic approval review blocked their upload.

The preview was validated by replacing only the five shader assets in the
existing Emscripten data package, updating file offsets and package size.
The original model bytes and WebAssembly remain byte-for-byte identical. No
C++ rebuild is required for this shader-only comparison. Building glTF external
buffer/image references were also checked; none were missing.

Both the original and modified game report WebGL errors in this software
renderer, so this is not a claim that all rendering warnings are resolved.
Physical mobile GPU testing and gameplay checks across all monster variants
remain outside this pass. Water directionality and dungeon appearance have
shader coverage but have not received an exhaustive in-world visual review.

## Next visual priority

The simple tree silhouettes and flat fountain surface now stand out beside the
detailed hero and architecture. A subsequent geometry/material pass should bring
those closer in fidelity while preserving mobile draw-call and memory budgets.

## Run the shader check

Install Playwright and its Chromium browser, then run:

```sh
node tests/shader-smoke.cjs
```

`PLAYWRIGHT_MODULE` and `BROWSER_EXECUTABLE` can point to an existing installation.
