# Meshy plate armor

The heavy armor appearance now uses locally rebuilt plate cuirass, mirrored pauldrons and
mirrored greaves. Existing equipment names, materials, dyes and stats select
the same slots. World and Me continue to share the equipment renderer.

The three Meshy-6 tasks consumed exactly 60 credits (20 each); task IDs and
triangle counts are recorded in `assets/armor/meshy-armor.json`. Meshy returned
display/full figures, so the requested pieces were extracted locally. The
display stand, other body parts and detached fragments were discarded.
Those extracted meshes produced jagged surfaces in game and have since been
retired. Clean, symmetric plate shells now replace their geometry. No new
Meshy jobs or credit charges were made.

The current hero model was not changed. The cuirass was fitted to its torso
and weighted locally to its existing Spine, Spine01, Spine02 and Hips bones.
The runtime copies the body's already blended pose to the cuirass, so walking,
attacks and riding use the same animation clock. This required no paid rigging.
Other body rigs retain the fitted static cuirass and procedural fallback.

Pauldrons and greaves attach to their existing limb bones. Mirroring is baked
into separate left GLBs, preserving winding and normals. Light armor retains
the tapered procedural shapes. A robe continues to cover armor.

Reproduce the current local replacements (the original task ledger is retained):

```
pip install numpy scipy trimesh shapely networkx
python tools/characters3d/build_clean_armor.py
python tools/characters3d/rig_armor.py
python tests/armor-assets.py
```

The generation job was a one-time approved push and has been removed after
completion. Never rerun the paid batch; resume its existing task IDs instead.

Validation: the web build, equipment operations, hero skin and cape checks
passed. Offline mesh rendering inspected idle, walk and sword-swing poses.
Live browser visual checking remains limited by the available browser's WebGL
startup failure, so phone playtesting is still needed for final visual approval.

## UO outfit study (review branch)

The `codex/uo-outfit-study` draft adds a locally baked chain underlayer while
keeping uncovered skin, the original model and its animations. Chest/shoulder/
shin plates use slimmer 24-sided profiles, a leather waist belt, and an armor
shader with stronger metal highlights. The cape has a narrower hem; equipped
colors and slots continue to use the shared world/preview renderer.

Rebuild the underlayer with `python tools/characters3d/build_armor_underlayer.py`
(requires Pillow in addition to the existing model tools).

The review board was rendered through `DrawEquippedHero` using raylib 6.0, an
offscreen EGL ES2 context, and the game's actual lit/armor shaders. It shows
front, side, back, and a distant perspective view. This is
an equipment rendering test scene, not a capture of the live browser game.
The visual draft still needs user review before replacing the live version.
