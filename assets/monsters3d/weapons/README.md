# Weapons

Fourteen static weapon and shield models to attach to a hand bone (`RightHand` / `LeftHand`,
or the forearm for the buckler) on any of the rigged characters: the hero, the armor
sets in `../armor/`, and the humanoid monsters.

Every file is set up for attaching:

- **Units are metres** and the size is realistic (below).
- **Origin = the grip** (the middle of the shield). The blade, head or tip points **+Y**
  and the weapon's flat face looks toward +Z. Attach to the hand bone with just a rotation
  to point it forward.
- One mesh, one material, 20k triangles, 512px textures, about 1.4 MB each.

| File | In-game name it fits | Length | Grip at (from the bottom) |
|---|---|---|---|
| `longsword.glb` | Longsword | 1.00 m | 13% |
| `broadsword.glb` | Broadsword, Viking Sword | 1.05 m | 13% |
| `scimitar.glb` | Scimitar (Katana can share it) | 0.90 m | 15% |
| `dagger.glb` | Dagger | 0.40 m | 18% |
| `mace.glb` | Mace, Club | 0.75 m | 22% |
| `war_hammer.glb` | War Hammer | 1.00 m | 25% |
| `battle_axe.glb` | Warlord's Cleaver, axes | 1.15 m | 25% |
| `halberd.glb` | Halberd, Glaive | 2.00 m | 30% |
| `short_spear.glb` | Short Spear, Bone Spear | 1.70 m | 35% |
| `quarterstaff.glb` | Quarterstaff, Black Staff | 1.80 m | 50% |
| `gnarled_staff.glb` | Gnarled Staff (mage) | 1.90 m | 35% |
| `short_bow.glb` | Short Bow, Composite Bow | 1.20 m | 50% |
| `heavy_crossbow.glb` | Heavy Crossbow | 0.85 m tall | 40% |
| `buckler.glb` | Buckler, round shields | 0.40 m across | center |

`previews/fit_check.jpg` renders all of them with a red dot at the origin. The grip point is
an estimate from each weapon's proportions, so nudge it a centimetre or two in-game if a hand
sits a little off. Concept art for each is in `source/`.

Not made yet: Katana, Rapier, Great Mace, Heavy Sword, Wyrmfang Blade, kite and heater
shields. The 2D game's weapon list is longer than this first batch.
