# Characters

- `Humanoid.glb` - Quaternius "Universal Animation Library" (free version), CC0 1.0
  Universal. Source: github.com/J-Ponzo/gltf-universal-animation-library (mirror of
  quaternius.itch.io/universal-animation-library). The mannequin mesh + rig, trimmed
  to the clips the game uses (Idle_Loop, Walk_Loop, Jog_Fwd_Loop, Sword_Attack,
  Sword_Idle, Spell_Simple_Shoot, Hit_Chest, Death01), constant channels collapsed,
  keys thinned, lightmap UVs removed. Clothing is painted per body region at runtime
  (see HumanRig in main.cpp), driven by the equipped armor.
- `gear/` - KayKit Character Pack: Adventurers 1.0 by Kay Lousberg, CC0 1.0 Universal.
  Source: github.com/KayKit-Game-Assets/KayKit-Character-Pack-Adventures-1.0.
  Weapons and shields attached to the hand/forearm bones.

A custom rigged character (e.g. a Meshy/Tripo model auto-rigged in Mixamo) can
replace Humanoid.glb. The loader (HumanEnsure in main.cpp) needs:
- Y-up, facing +Z, feet at the origin (height is normalised on load);
- clips named as above (or remap them in HumanEnsure);
- the bone names the attachments and region painting use: DEF-head, DEF-neck,
  DEF-spine.003, DEF-hand.R/L, DEF-forearm.L, DEF-upper_arm.*, DEF-thigh.*,
  DEF-shin.*, DEF-foot.* (a Mixamo rig's mixamorig:* names need a small name map).
A textured model can skip the region painting (its own texture is the outfit).
Grip/shield offsets are g_humanGripRot/Off and g_humanShieldRot/Off.
