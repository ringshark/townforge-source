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
// ---- clips, retargeted onto this rig ----
// Each clip file carries its own rig's rest pose. A joint's motion is taken as a
// world-space delta from that rest (G_src(t) * inv(Rest_src)) and applied to this
// rig's rest (delta * Rest_dst), then turned back into local rotations. Only the
// hips keep a translation (scaled by the hip height ratio); every other joint keeps
// this rig's own bone lengths. For a clip made on this same rig it changes nothing.
const qmul = (a, b) => [a[3]*b[0] + a[0]*b[3] + a[1]*b[2] - a[2]*b[1], a[3]*b[1] - a[0]*b[2] + a[1]*b[3] + a[2]*b[0], a[3]*b[2] + a[0]*b[1] - a[1]*b[0] + a[2]*b[3], a[3]*b[3] - a[0]*b[0] - a[1]*b[1] - a[2]*b[2]];
const qinv = q => [-q[0], -q[1], -q[2], q[3]];
const qnorm = q => { const l = Math.hypot(...q) || 1; return q.map(v => v / l); };
const qrot = (q, v) => { const p = qmul(qmul(q, [v[0], v[1], v[2], 0]), qinv(q)); return [p[0], p[1], p[2]]; };
function sampler(s, t) {
  const inp = s.getInput().getArray(), out = s.getOutput().getArray(), n = s.getOutput().getElementSize();
  let i = 0; while (i < inp.length - 2 && inp[i + 1] <= t) i++;
  const j = Math.min(i + 1, inp.length - 1), t0 = inp[i], t1 = inp[j];
  const a = t1 > t0 ? Math.min(1, Math.max(0, (t - t0) / (t1 - t0))) : 0;
  const A = Array.from(out.slice(i * n, i * n + n)), B = Array.from(out.slice(j * n, j * n + n));
  if (n === 4) { const d = A[0]*B[0] + A[1]*B[1] + A[2]*B[2] + A[3]*B[3]; if (d < 0) for (let k = 0; k < 4; k++) B[k] = -B[k]; return qnorm(A.map((v, k) => v * (1 - a) + B[k] * a)); }
  return A.map((v, k) => v * (1 - a) + B[k] * a);
}
const dstSkin = root.listSkins()[0];
const dstJoints = dstSkin.listJoints();
// global rest rotations (and hips position) of a node set, walking up to the root
function restGlobals(nodes) {
  const G = new Map();
  const g = n => { if (G.has(n)) return G.get(n); const p = n.getParentNode(); const pr = p ? g(p) : [0, 0, 0, 1]; const r = qmul(pr, n.getRotation()); G.set(n, r); return r; };
  for (const n of nodes) g(n);
  return G;
}
const dstRest = restGlobals(dstJoints);
const dstHips = dstJoints.find(j => j.getName() === 'Hips');
for (const f of clipF) {
  const src = await io.read(f);
  const sNodes = new Map(src.getRoot().listNodes().map(n => [n.getName(), n]));
  const sJoints = dstJoints.map(j => sNodes.get(j.getName()));
  const srcRest = restGlobals(sJoints.filter(Boolean));
  const sHips = sNodes.get('Hips');
  const hipScale = (dstHips && sHips) ? dstHips.getTranslation()[1] / (sHips.getTranslation()[1] || 1) : 1;
  for (const a of src.getRoot().listAnimations()) {
    const parts = a.getName().split('|');
    const name = parts.length >= 2 ? parts[1] : a.getName();
    const ch = new Map();
    for (const c of a.listChannels()) if (c.getTargetNode()) ch.set(c.getTargetNode().getName() + '.' + c.getTargetPath(), c.getSampler());
    let times = null;
    for (const [k, s] of ch) if (k.endsWith('.rotation')) { const t = s.getInput().getArray(); if (!times || t.length > times.length) times = t; }
    if (!times) continue;
    const T = Array.from(times), nJ = dstJoints.length;
    const outR = dstJoints.map(() => new Float32Array(T.length * 4));
    const outH = new Float32Array(T.length * 3);
    for (let ti = 0; ti < T.length; ti++) {
      const t = T[ti];
      // source globals at t
      const sG = new Map();
      const sg = n => { if (sG.has(n)) return sG.get(n); const p = n.getParentNode(); const pr = p ? sg(p) : [0, 0, 0, 1]; const key = n.getName() + '.rotation'; const lr = ch.has(key) ? sampler(ch.get(key), t) : n.getRotation(); const r = qnorm(qmul(pr, lr)); sG.set(n, r); return r; };
      const dG = new Map();
      for (let k = 0; k < nJ; k++) {
        const dj = dstJoints[k], sj = sJoints[k];
        let G;
        if (sj && srcRest.has(sj)) G = qnorm(qmul(qmul(sg(sj), qinv(srcRest.get(sj))), dstRest.get(dj)));
        else G = dstRest.get(dj);
        dG.set(dj, G);
      }
      const pg = n => { const p = n.getParentNode(); if (!p) return [0, 0, 0, 1]; if (dG.has(p)) return dG.get(p); return restGlobals([p]).get(p); };
      for (let k = 0; k < nJ; k++) {
        const dj = dstJoints[k];
        const L = qnorm(qmul(qinv(pg(dj)), dG.get(dj)));
        outR[k].set(L, ti * 4);
      }
      if (dstHips) {
        const key = 'Hips.translation';
        const sp = ch.has(key) ? sampler(ch.get(key), t) : (sHips ? sHips.getTranslation() : dstHips.getTranslation());
        const s0 = sHips ? sHips.getTranslation() : sp, d0 = dstHips.getTranslation();
        outH.set([d0[0] + (sp[0] - s0[0]) * hipScale, d0[1] + (sp[1] - s0[1]) * hipScale, d0[2] + (sp[2] - s0[2]) * hipScale], ti * 3);
      }
    }
    const na = doc.createAnimation(name);
    const inp = doc.createAccessor().setType('SCALAR').setArray(new Float32Array(T)).setBuffer(buf);
    for (let k = 0; k < nJ; k++) {
      const smp = doc.createAnimationSampler().setInput(inp).setOutput(doc.createAccessor().setType('VEC4').setArray(outR[k]).setBuffer(buf)).setInterpolation('LINEAR');
      na.addSampler(smp); na.addChannel(doc.createAnimationChannel().setTargetNode(dstJoints[k]).setTargetPath('rotation').setSampler(smp));
    }
    if (dstHips) {
      const smp = doc.createAnimationSampler().setInput(inp).setOutput(doc.createAccessor().setType('VEC3').setArray(outH).setBuffer(buf)).setInterpolation('LINEAR');
      na.addSampler(smp); na.addChannel(doc.createAnimationChannel().setTargetNode(dstHips).setTargetPath('translation').setSampler(smp));
    }
    console.log('clip', name);
  }
}
// Bake the Armature's 0.01 scale (Meshy rigs are in centimetres under a scaled
// root) into the joints, clips and inverse bind matrices, so loaders that apply
// node transforms to the mesh but not the skin (raylib) agree.
{
  const skin = root.listSkins()[0];
  const joints = new Set(skin ? skin.listJoints() : []);
  const arm = root.listNodes().find(n => n.getName() === 'Armature');
  const k = arm ? arm.getScale()[0] : 1;
  if (skin && arm && Math.abs(k - 1) > 1e-6) {
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
  let o;
  if (process.env.PERMISSIVE) { // collapse across UV seams, but weigh UV error so texels stay put
    const uv = p.getAttribute('TEXCOORD_0').getArray();
    const attr = new Float32Array(uv.length); for (let i = 0; i < uv.length; i++) attr[i] = uv[i];
    [o] = MeshoptSimplifier.simplifyWithAttributes(ia, pa, 3, attr, 2, [+(process.env.UVW || 1.0), +(process.env.UVW || 1.0)], null, want, +(process.env.ERR || 0.02), ['Permissive']);
  } else [o] = MeshoptSimplifier.simplify(ia, pa, 3, want, 0.03, []); // keeps UV seams (sloppy smeared the texture)
  idx.setArray(new Uint32Array(o));
  compactPrimitive(p);
}
await doc.transform(resample({ tolerance: +(process.env.RESAMPLE || 0.0005) }), prune(), dedup(),
  textureCompress({ encoder: sharp, targetFormat: process.env.JPG ? 'jpeg' : 'png', resize: [+texSize, +texSize], quality: 85 }));
await io.write(out, doc);
