// A Meshy weapon -> the game's held-weapon model: grip at the origin, business end
// along +Y, true length in metres, simplified, palette-PNG texture.
// usage: node weapon.mjs in.glb out.glb lengthMetres gripFraction [tris]
import { NodeIO } from '@gltf-transform/core';
import { ALL_EXTENSIONS } from '@gltf-transform/extensions';
import { prune, dedup, textureCompress, compactPrimitive, transformPrimitive, flatten } from '@gltf-transform/functions';
import { MeshoptSimplifier } from 'meshoptimizer';
import sharp from 'sharp';
const [src, dst, len, frac, tris = '2500'] = process.argv.slice(2);
const io = new NodeIO().registerExtensions(ALL_EXTENSIONS);
const doc = await io.read(src);
const root = doc.getRoot();
for (const m of root.listMaterials()) { m.setNormalTexture(null); m.setMetallicRoughnessTexture(null); m.setOcclusionTexture(null); m.setEmissiveTexture(null); m.setMetallicFactor(0); m.setRoughnessFactor(1); }
await MeshoptSimplifier.ready;
const prims = [];
for (const mesh of root.listMeshes()) for (const p of mesh.listPrimitives()) {
  const ia = new Uint32Array(p.getIndices().getArray()), pa = new Float32Array(p.getAttribute('POSITION').getArray());
  const uv = p.getAttribute('TEXCOORD_0').getArray();
  const [o] = MeshoptSimplifier.simplifyWithAttributes(ia, pa, 3, new Float32Array(uv), 2, [1, 1], null, Math.min(ia.length, +tris * 3), 0.02, ['Permissive']);
  p.getIndices().setArray(new Uint32Array(o)); compactPrimitive(p); prims.push(p);
}
let mn = [1e9, 1e9, 1e9], mx = [-1e9, -1e9, -1e9];
for (const p of prims) { const a = p.getAttribute('POSITION').getArray(); for (let i = 0; i < a.length; i += 3) for (let k = 0; k < 3; k++) { mn[k] = Math.min(mn[k], a[i + k]); mx[k] = Math.max(mx[k], a[i + k]); } }
const H = mx[1] - mn[1], s = +len / H;
const gx = (mn[0] + mx[0]) / 2, gy = mn[1] + H * +frac, gz = (mn[2] + mx[2]) / 2;
const M = [s, 0, 0, 0, 0, s, 0, 0, 0, 0, s, 0, -gx * s, -gy * s, -gz * s, 1];
for (const p of prims) transformPrimitive(p, M);
for (const n of root.listNodes()) { n.setTranslation([0, 0, 0]); n.setRotation([0, 0, 0, 1]); n.setScale([1, 1, 1]); }
await doc.transform(prune(), dedup(), textureCompress({ encoder: sharp, targetFormat: 'png', resize: [256, 256] }));
for (const t of root.listTextures()) t.setImage(new Uint8Array(await sharp(Buffer.from(t.getImage())).png({ palette: true, quality: 90, effort: 10, dither: 0.6 }).toBuffer()));
await io.write(dst, doc);
console.log(dst.split('/').pop(), 'H', H.toFixed(3), 'extent', (mx[0]-mn[0]).toFixed(3), (mx[2]-mn[2]).toFixed(3));
