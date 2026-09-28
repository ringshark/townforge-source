# 3D monster models

Textured, game-ready GLB models generated with Meshy image-to-3D from the 2D concept
art in `source/`. Each has one mesh (~31k triangles), one PBR material (base color,
normal, metallic-roughness) and 1024x1024 textures (JPEG color/MR, PNG normal).
No glTF extensions are required, so the files load with raylib's `LoadModel`.
The models are static: no rig or animations.

Scale is normalized to roughly 2 units on the longest axis, centered on the origin,
so scale and position each model in-engine.

Filenames follow the in-game monster names (`kDungeons` in `main.cpp`) where one exists.
The spider line is not in the game yet and uses the names from its concept art.

| File | In game | Source art |
|---|---|---|
| `web_spinner.glb` | new: spider line, Lvl 1 | Spider line, Lvl 1 |
| `silk_stalker.glb` | new: spider line, Lvl 2 | Spider line, Lvl 2 |
| `venom_weaver.glb` | new: spider line, Lvl 3 | Spider line, Lvl 3 |
| `brood_hunter.glb` | new: spider line, Lvl 4 | Spider line, Lvl 4 |
| `nest_guardian.glb` | new: spider line, Lvl 5 | Spider line, Lvl 5 |
| `spider_broodmother.glb` | new: spider line boss | Red/black spider with green egg sac |
| `orc_warbringer.glb` | Orc Warbringer (Bloodtusk Hold) | Leather-and-iron orc |
| `orc_overlord.glb` | Orc Overlord (Bloodtusk Hold boss) | Horned, black-armored orc |
| `stoneborn.glb` | Stoneborn (Emberveil Hollow) | Mossy stone golem with amber crystals |
| `bonewalker.glb` | Bonewalker (Sunken Crypt) | Crypt lineup 1 |
| `rotbound_corpse.glb` | Rotbound Corpse (Sunken Crypt) | Crypt lineup 2 |
| `gravewretch.glb` | Gravewretch (Sunken Crypt) | Crypt lineup 3 |
| `grave_warden.glb` | Grave Warden (Sunken Crypt) | Crypt lineup 4 |
| `crypt_sovereign.glb` | Crypt Sovereign (Sunken Crypt) | Crypt lineup 5 |
| `glacier_wight.glb` | new: ice undead line, Lvl 1 | Ice lineup, Lvl 1* |
| `rimebound_horror.glb` | new: ice undead line, Lvl 2 | Ice lineup, Lvl 2* |
| `hoarfrost_revenant.glb` | new: ice undead line, Lvl 3 | Ice lineup, Lvl 3* |
| `winters_maw.glb` | new: ice undead line, Lvl 4 | Ice lineup, Lvl 4* |
| `ice_undead_lvl5.glb` | new: ice undead line, Lvl 5 (placeholder name) | Ice lineup, Lvl 5* |
| `fishfolk_scout.glb` | new: fish-folk line (placeholder name) | Fish-folk lineup 1 |
| `fishfolk_archer.glb` | new: fish-folk line (placeholder name) | Fish-folk lineup 2 |
| `fishfolk_warlord.glb` | new: fish-folk line (placeholder name) | Fish-folk lineup 3 |
| `fishfolk_shaman.glb` | new: fish-folk line (placeholder name) | Fish-folk lineup 4 |
| `fishfolk_huntress.glb` | new: fish-folk line (placeholder name) | Fish-folk lineup 5 |
| `world_boss_dragon.glb` | new: world boss (placeholder name) | Three-headed dragon from the battle scene** |
| `hero.glb` | main hero (static, full PBR) | Hero turnaround: front/side/back views |
| `hero_animated.glb` | main hero (rigged + animated) | Same model, auto-rigged by Meshy |

\* The ice lineup's figures overlap too much to crop apart, so each was redrawn in
isolation from the lineup with Meshy image-to-image (`nano-banana-pro`) before 3D
generation. All other inputs are direct crops of the concept art.

`previews/` holds Meshy's render of each model.

Generation settings: `ai_model: latest`, `topology: triangle`,
`target_polycount: 30000`, `should_remesh`, `should_texture`, `enable_pbr`,
`symmetry_mode: auto`. The textures were then downsized from 2048 to 1024 with
`gltf-transform resize` and `prune`.

\*\* The dragon was isolated from the battle scene with Meshy image-to-image (wings raised
so the whole silhouette fits in frame, no breath effects), then generated at a 50k
triangle target instead of 30k.

## Hero

- `hero.glb` was built with Meshy multi-image-to-3D from all three turnaround views
  (`source/hero_front|side|back.jpg`), so the backpack and cape match the art.
- `hero_animated.glb` is the same character run through Meshy auto-rigging: a
  24-bone humanoid skeleton (`Armature`) with two clips, `Walk` and `Run`, merged into
  one file. Load it with raylib's `LoadModel` + `LoadModelAnimations`. Meshy's rigging
  output keeps only the base color texture (no normal/metallic-roughness maps), so it
  looks slightly flatter than `hero.glb`.
