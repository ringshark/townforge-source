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

### Armored tunic review draft

The rejected plate study remains available in branch history. The next draft
uses `build_armored_tunic.py`: the hero's own torso surface and original skin
weights form the cloth body, with a fitted waist, leather belt and short bound
hem. The tunic uses diffuse cloth lighting; steel stays at the shoulders,
forearms and shins. The heavy thigh plates are omitted on the hero. Plain fabric
replaces the noisy chain atlas. The original hero GLB remains unchanged.

Generate with `python tools/characters3d/build_armored_tunic.py` and
`python tools/characters3d/build_armor_underlayer.py`. No additional Meshy jobs
or credits are involved. This is a review draft, not a live deployment.

### Complete knight skin prototype

The next review step switches the hero to the existing `hero_knight.glb`, a
complete Meshy outfit with sixteen retargeted clips. No new generation credits
were spent. `DrawEquippedHero` is shared by the world and try-on preview; it
first draws the complete skin, with only held weapons and shields supplied by
equipment. Armor overlays, procedural boots and the separate cape are disabled
for this path. Equipment data, stats and equip rules remain unchanged. If the
skin cannot load, the previous fitted character remains the fallback.

This is one fixed cosmetic skin for review, not a wardrobe UI or live release.
Its baked helmet and cloth cannot be individually toggled. Dense embedded
textures now use mipmaps and trilinear filtering. Run
`python tests/complete-skin.py` to validate its rig, core clips, budget and
held-equipment boundary.

### All eight skins: material and shading refinement

All complete `hero_` outfits now use a dedicated `skin.fs` finish: restrained
contrast compression quiets baked scratches, low-saturation steel receives
stronger directional highlights, and the dark leather/cloth profiles gain a
small brightness lift. Warm facial detail remains protected. Filtering remains
trilinear; neighboring atlas texels are not sampled, preserving color boundaries
between UV islands. Nearly parallel normals at identical positions are welded
across UV seams, while sharper plate creases remain intact.

The source meshes, texture atlases, rigs and animation clips are unchanged.
This is a material/shading pass, not a remodel. Each loaded model owns its finish
shader/profile, and camera, fog and time-of-day uniforms are refreshed during
rendering. Eight profiles are reviewed at the same camera and noon lighting;
`tests/complete-skin.py` checks every outfit's core clips, weights and budget.
The work remains on the review branch pending visual acceptance.
