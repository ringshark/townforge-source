# Rigged Meshy characters

`assets/characters3d/*.glb` are Meshy rigs merged with their Meshy animation
clips by `merge_anims.mjs` (gltf-transform + meshoptimizer + sharp):

    node merge_anims.mjs out.glb 2000 1024 rig.glb walking.glb running.glb clip1.glb ...

It drops the rig's placeholder clip, names each clip by its Meshy action
(`Armature|Idle|baselayer` -> `Idle`), bakes the Armature's 0.01 scale into the
joints, clips and inverse bind matrices (raylib otherwise skins the body to a
speck), simplifies the mesh while keeping UV seams, strips all but base colour
and shrinks the texture. main.cpp finds clips by name (kSkClipNames).
