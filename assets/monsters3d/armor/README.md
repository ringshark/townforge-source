# Player armor sets

Eight complete outfits for the player character, each a full rigged character that shares
the hero's body and skeleton (`../hero_animated.glb`). The game swaps the whole model when
gear changes instead of bolting small pieces onto one body.

| Set | For | Files |
|---|---|---|
| Leather Ranger | archery, taming, wilderness | `leather_ranger.glb`, `leather_ranger_animated.glb` |
| Chainmail | footman, mid-tier melee | `chainmail.glb`, `chainmail_animated.glb` |
| Plate Knight | heavy melee | `plate_knight.glb`, `plate_knight_animated.glb` |
| Mage Robes | Magery | `mage_robes.glb`, `mage_robes_animated.glb` |
| Necromancer | Necromancy | `necromancer.glb`, `necromancer_animated.glb` |
| Paladin | Chivalry | `paladin.glb`, `paladin_animated.glb` |
| Thief | Hiding, Stealth, Lockpicking | `thief.glb`, `thief_animated.glb` |
| Bard | Musicianship, Peacemaking | `bard.glb`, `bard_animated.glb` |

## How they were made

1. The concept for each set is Meshy image-to-image (`nano-banana-pro`) on the hero's front
   view, keeping the same body and A-pose (`source/*.jpg`).
2. Each concept became a 50k-triangle textured model (image-to-3D), textures shrunk to 1024px.
3. Each model was auto-rigged by Meshy at 1.8 m. All rigs share bone names with the hero's.
4. The hero's 21 clips were copied onto every rig by bone name, with hip translation scaled
   by the rig's hip height (about 5% here). No clip had to be regenerated.

`*_animated.glb` carries all 21 hero clips: Idle, CombatIdle, Walk, Run, Sneak, SwordSlash,
ComboAttack, AxeChop, HammerSwing, Parry, BowShot, Cast, CastCharged, HitReact, Death,
PickUp, Gather, Drink, Kneel, Jump and RunJump (see `../README.md` for what each is for).
`previews/animation_check.jpg` is a render of the seven new sets at three points in
Walk, SwordSlash and Cast in a headless three.js viewer: no stretching or collapse.

## Notes

- Weapons and shields aren't included. Attach them to the hand bones (the game already has
  weapon meshes in `assets/characters/gear/`).
- Like other Meshy rigs, these keep only the base color texture (no normal or
  metallic-roughness maps), so they look a little flatter than the static files.
- Long robes (Mage, Necromancer) and the Paladin's tabard move with the legs but don't
  simulate cloth, so wide leg swings can poke through them slightly.
- Only the front pose was checked; look at each set in-game at your camera distance.
