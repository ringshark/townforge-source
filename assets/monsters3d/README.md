# Sculpted monsters (Meshy image-to-3D)

Textured, unrigged GLB models, generated with Meshy image-to-3D from concept art and
shrunk for the web build: about 6-22k triangles, one 512px palette PNG (base color
only: raylib's web build has no JPEG loader, and the lit shader uses no normal maps).
The game animates them in code (MeshMonDraw in main.cpp): a walking bob, a lunge,
the hit flash and a topple on death. Concept art and Meshy's renders are kept outside
the preloaded assets, in `art/monsters3d/`.

| File | Monsters in game |
|---|---|
| web_spinner, silk_stalker, venom_weaver, brood_hunter, nest_guardian | the Weavers' Nest line, in that order |
| spider_broodmother | The Broodmother (Weavers' Nest boss) |
| orc_warbringer | Orc Grunt, Orc Archer, Orc Shaman, Orc Brute (Grimtusk Hold) |
| orc_overlord | Orc Warlord |
| stoneborn | Rock Golem |
| bonewalker, rotbound_corpse, gravewretch, grave_warden, crypt_sovereign | the Whisper Crypt line; crypt_sovereign also plays The Whisper King |
| glacier_wight, rimebound_horror, hoarfrost_revenant, winters_maw, frostbound_king | the Frostbound Tomb; glacier_wight also plays the Frostbite Husk |
| scalekin_raider_scout / archer / huntress / shaman / warlord | Scalekin Raider (Sunken Vault) - each raider gets one of the five looks |
| vyrathax | Vyrathax the Tri-Wyrm, the world boss (faces +Z: `yawOff` 180) |

The second batch (#76) was decimated with meshoptimizer's sloppy simplifier
(`simplifySloppy`, ~7-14k triangles): Meshy's UV seams stop the normal simplifier.

Sizes and the name mapping live in `kMeshMons` (main.cpp). Meshy's generation
settings: image-to-3D, `target_polycount: 30000`, remesh, textured, PBR; then
`gltf-transform` simplify + texture resize and a palette PNG re-encode.
