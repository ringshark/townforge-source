# 3D monster models

Textured, game-ready GLB models generated with Meshy image-to-3D from the 2D concept
art in `source/`. Each has one mesh (~31k triangles), one PBR material (base color,
normal, metallic-roughness) and 1024x1024 textures (JPEG color/MR, PNG normal).
No glTF extensions are required, so the files load with raylib's `LoadModel`.
The models are static: no rig or animations.

Scale is normalized to roughly 2 units on the longest axis, centered on the origin,
so scale and position each model in-engine.

| File | Source art |
|---|---|
| `web_spinner.glb` | Spider line, Lvl 1 |
| `silk_stalker.glb` | Spider line, Lvl 2 |
| `venom_weaver.glb` | Spider line, Lvl 3 |
| `brood_hunter.glb` | Spider line, Lvl 4 |
| `nest_guardian.glb` | Spider line, Lvl 5 |
| `spider_broodmother.glb` | Red/black spider with green egg sac |
| `orc_brute.glb` | Leather-and-iron orc |
| `orc_warlord.glb` | Horned, black-armored orc |
| `stone_golem.glb` | Mossy stone golem with amber crystals |

`previews/` holds Meshy's render of each model.

Generation settings: `ai_model: latest`, `topology: triangle`,
`target_polycount: 30000`, `should_remesh`, `should_texture`, `enable_pbr`,
`symmetry_mode: auto`. The textures were then downsized from 2048 to 1024 with
`gltf-transform resize` and `prune`.
