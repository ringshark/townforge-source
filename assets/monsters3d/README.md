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
| fen_serpent, brine_drake, stormwyrm, abyssal_wyrm | the Sunken Vault's serpent and drakes |
| magma_hound, ash_revenant | the Ember Depths |
| warren_rat | the Hollow |
| vyrathax | Vyrathax the Tri-Wyrm, the world boss |

Everything that walks on two legs is now rigged and animated instead: see
`assets/characters3d/` (orcs, the Whisper Crypt, the Frostbound Tomb, the Scalekin
raiders and the Sunken King, the Ember Depths' imps, golems and the Emberlord, the
Hollow's brutes and wraiths, the Rock Golem and the Sorrow Wraith). The 2026-09-28
batch was simplified with meshoptimizer's attribute-aware permissive mode (UV
seams kept honest) and re-encoded as 256-colour palette PNGs.
