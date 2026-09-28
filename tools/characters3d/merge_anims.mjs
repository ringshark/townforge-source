// Merge a Meshy rigged character and its separate animation GLBs into one small
// file. usage: node merge_anims.mjs out.glb targetTris texSize rig.glb clip1.glb [clip2.glb ...]
import { NodeIO } from '@gltf-transform/core';
import { ALL_EXTENSIONS } from '@gltf-transform/extensions';
import { prune, dedup, textureCompress, resample, compactPrimitive } from '@gltf-transform/functions';
import { MeshoptSimplifier } from 'meshoptimizer';
import sharp from 'sharp';
const [out, tris, texSize, rigF, ...clipF] = process.argv.slice(2);
const io = new NodeIO().registerExtensions(ALL_EXTENSIONS);
const doc = await io.read(rigF);
const root = doc.getRoot();
const byName = new Map(root.listNodes().map(n => [n.getName(), n]));
for (const a of root.listAnimations()) a.dispose(); // the rig's own 0.3s "clip0"
const buf = root.listBuffers()[0];
for (const f of clipF) {
  const src = await io.read(f);
  for (const a of src.getRoot().listAnimations()) {
    const parts = a.getName().split('|');
    const name = parts.length >= 2 ? parts[1] : a.getName();
    const na = doc.createAnimation(name);
    for (const ch of a.listChannels()) {
      const tn = ch.getTargetNode(); if (!tn) continue;
      const target = byName.get(tn.getName()); if (!target) continue;
      const s = ch.getSampler();
      const inp = doc.createAccessor().setType('SCALAR').setArray(s.getInput().getArray().slice()).setBuffer(buf);
      const o = s.getOutput();
      const outp = doc.createAccessor().setType(o.getType()).setArray(o.getArray().slice()).setBuffer(buf);
      const ns = doc.createAnimationSampler().setInput(inp).setOutput(outp).setInterpolation(s.getInterpolation());
      na.addSampler(ns);
      na.addChannel(doc.createAnimationChannel().setTargetNode(target).setTargetPath(ch.getTargetPath()).setSampler(ns));
    }
    console.log('clip', name);
  }
}
// Bake the Armature's 0.01 scale (Meshy rigs are in centimetres under a scaled
// root) into the joints, clips and inverse bind matrices, so loaders that apply
// node transforms to the mesh but not the skin (raylib) agree.
{
  const skin = root.listSkins()[0];
  const joints = new Set(skin.listJoints());
  const arm = root.listNodes().find(n => n.getName() === 'Armature');
  const k = arm ? arm.getScale()[0] : 1;
  if (arm && Math.abs(k - 1) > 1e-6) {
    arm.setScale([1, 1, 1]);
    for (const j of joints) { const t = j.getTranslation(); j.setTranslation([t[0] * k, t[1] * k, t[2] * k]); }
    const done = new Set();
    for (const a of root.listAnimations()) for (const ch of a.listChannels()) {
      if (ch.getTargetPath() !== 'translation' || !joints.has(ch.getTargetNode())) continue;
      const o = ch.getSampler().getOutput(); if (done.has(o)) continue; done.add(o);
      const arr = o.getArray().slice(); for (let i = 0; i < arr.length; i++) arr[i] *= k; o.setArray(arr);
    }
    const ibm = skin.getInverseBindMatrices(); const m = ibm.getArray().slice();
    for (let j = 0; j < m.length; j += 16) for (let c = 0; c < 4; c++) for (let r = 0; r < 3; r++) m[j + c * 4 + r] *= k;
    ibm.setArray(m);
    console.log('baked armature scale', k);
  }
}
for (const m of root.listMaterials()) {
  m.setNormalTexture(null); m.setMetallicRoughnessTexture(null); m.setOcclusionTexture(null); m.setEmissiveTexture(null);
  m.setMetallicFactor(0); m.setRoughnessFactor(1);
}
for (const e of root.listExtensionsUsed()) if (e.extensionName !== 'KHR_texture_transform') e.dispose();
await MeshoptSimplifier.ready;
for (const mesh of root.listMeshes()) for (const p of mesh.listPrimitives()) {
  const idx = p.getIndices(), pos = p.getAttribute('POSITION');
  const ia = new Uint32Array(idx.getArray()), pa = new Float32Array(pos.getArray());
  const want = Math.min(ia.length, Math.floor(+tris) * 3);
  const [o] = MeshoptSimplifier.simplify(ia, pa, 3, want, 0.03, []); // keeps UV seams (sloppy smeared the texture)
  idx.setArray(new Uint32Array(o));
  compactPrimitive(p);
}
await doc.transform(resample({ tolerance: 0.0005 }), prune(), dedup(),
  textureCompress({ encoder: sharp, targetFormat: 'png', resize: [+texSize, +texSize], quality: 85 }));
await io.write(out, doc);
