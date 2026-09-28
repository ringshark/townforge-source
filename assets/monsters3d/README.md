# 3D monster models

Textured, game-ready GLB models generated with Meshy image-to-3D from the 2D concept
art in `source/`. Each has one mesh (~31k triangles), one PBR material (base color,
normal, metallic-roughness) and 1024x1024 textures (JPEG color/MR, PNG normal).
No glTF extensions are required, so the files load with raylib's `LoadModel`.
The models are static: no rig or animations.

Scale is normalized to roughly 2 units on the longest axis, centered on the origin,
so scale and position each model in-engine.

Filenames follow the monster names in Town Forge 3D (the build in
`ringshark/townforge-3d-preview`), not the 2D `main.cpp`, whose rosters differ. The five
`scalekin_raider_*` files are role variants of the Sunken Vault's single Scalekin
Raider monster.

| File | In Town Forge 3D |
|---|---|
| `web_spinner.glb` | Web Spinner (Weavers' Nest) |
| `silk_stalker.glb` | Silk Stalker (Weavers' Nest) |
| `venom_weaver.glb` | Venom Weaver (Weavers' Nest) |
| `brood_hunter.glb` | Brood Hunter (Weavers' Nest) |
| `nest_guardian.glb` | Nest Guardian (Weavers' Nest) |
| `broodmother.glb` | The Broodmother (Weavers' Nest boss) |
| `orc_grunt.glb` | Orc Grunt, also used for Orc Archer, Shaman and Brute (Grimtusk Hold) |
| `orc_warlord.glb` | Orc Warlord (Grimtusk Hold) |
| `rock_golem.glb` | Rock Golem (Stonepeaks) |
| `bonewalker.glb` | Bonewalker (Whisper Crypt) |
| `rotbound_corpse.glb` | Rotbound Corpse (Whisper Crypt) |
| `gravewretch.glb` | Gravewretch (Whisper Crypt) |
| `grave_warden.glb` | Grave Warden (Whisper Crypt) |
| `crypt_sovereign.glb` | Crypt Sovereign (Whisper Crypt) |
| `glacier_wight.glb` | Glacier Wight (Frostbound Tomb) \* |
| `rimebound_horror.glb` | Rimebound Horror (Frostbound Tomb) \* |
| `hoarfrost_revenant.glb` | Hoarfrost Revenant (Frostbound Tomb) \* |
| `winters_maw.glb` | Winter's Maw (Frostbound Tomb) \* |
| `frostbound_king.glb` | The Frostbound King (Frostbound Tomb boss) \* |
| `scalekin_raider_scout.glb` | Scalekin Raider, scout variant (Sunken Vault) |
| `scalekin_raider_archer.glb` | Scalekin Raider, archer variant (Sunken Vault) |
| `scalekin_raider_warlord.glb` | Scalekin Raider, warlord variant (Sunken Vault) |
| `scalekin_raider_shaman.glb` | Scalekin Raider, shaman variant (Sunken Vault) |
| `scalekin_raider_huntress.glb` | Scalekin Raider, huntress variant (Sunken Vault) |
| `vyrathax.glb` | Vyrathax the Tri-Wyrm (world boss) \*\* |
| `hero.glb` | Main hero (static, full PBR) |
| `hero_animated.glb` | Main hero (rigged, 19 animation clips) |

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
  24-bone humanoid skeleton (`Armature`) with 19 clips from Meshy's animation library,
  merged into one file. Load it with raylib's `LoadModel` + `LoadModelAnimations`.
  Meshy's rigging output keeps only the base color texture (no normal/metallic-roughness
  maps), so it looks slightly flatter than `hero.glb`.

| Clip | Length | Use for | Meshy action |
|---|---|---|---|
| `Idle` | 4.0s | standing around | 0 Idle |
| `CombatIdle` | 1.7s | in a fight, between swings | 89 Combat Idle |
| `Walk` | 1.1s | walking | rig default |
| `Run` | 0.7s | running | rig default |
| `SwordSlash` | 1.5s | one-handed melee swing | 219 Right-hand Sword Slash |
| `ComboAttack` | 4.4s | special or crit melee | 105 Triple Combo Attack |
| `AxeChop` | 7.7s | axe swing, Lumberjacking | 237 Charged Axe Chop |
| `HammerSwing` | 1.9s | mace or hammer, Mining | 128 Heavy Hammer Swing |
| `BowShot` | 7.9s | Archery | 222 Draw and Shoot from Back |
| `Cast` | 2.3s | spellcasting | 129 Mage Spell Cast |
| `CastCharged` | 2.7s | big spells | 125 Charged Spell Cast |
| `Parry` | 1.9s | parry or block | 147 Sword Parry |
| `HitReact` | 1.7s | taking a hit | 178 Hit Reaction |
| `Death` | 2.3s | dying | 189 Dying Backwards |
| `PickUp` | 7.2s | looting | 276 Male Bend Over Pick Up |
| `Gather` | 6.0s | gathering, harvesting | 284 Collect Object |
| `Drink` | 8.9s | potions, bandaging | 342 Stand and Drink |
| `Sneak` | 2.9s | Hiding and Stealth | 559 Sneaky Walk |
| `Kneel` | 2.6s | Meditation, praying at shrines | 365 Kneel on One Knee and Stand |

Some library clips are long (`AxeChop`, `BowShot`, `PickUp`, `Drink`), so play part of
the clip or speed it up to fit the game's action timing. Fishing, playing instruments and
riding have no library clip and need code-driven motion.
