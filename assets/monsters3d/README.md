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

`previews/` holds Meshy's render of each model.

Generation settings: `ai_model: latest`, `topology: triangle`,
`target_polycount: 30000`, `should_remesh`, `should_texture`, `enable_pbr`,
`symmetry_mode: auto`. The textures were then downsized from 2048 to 1024 with
`gltf-transform resize` and `prune`.
