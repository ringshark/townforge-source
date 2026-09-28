// Dye regions for the hero (in place): marks cloak / shirt / trousers vertices in
// COLOR_0 (pure red / green / blue markers, white elsewhere) and turns those parts
// of the texture to neutral grey detail, so the game's vertex tint dyes them.
// usage: node paint_hero.mjs hero.glb
import { NodeIO } from '@gltf-transform/core';
import { ALL_EXTENSIONS } from '@gltf-transform/extensions';
import sharp from 'sharp';
const [f] = process.argv.slice(2);
const io = new NodeIO().registerExtensions(ALL_EXTENSIONS);
const doc = await io.read(f);
const p = doc.getRoot().listMeshes()[0].listPrimitives()[0];
const pos = p.getAttribute('POSITION').getArray(), uv = p.getAttribute('TEXCOORD_0').getArray(), idx = p.getIndices().getArray();
const tex = p.getMaterial().getBaseColorTexture();
const r = await sharp(Buffer.from(tex.getImage())).removeAlpha().raw().toBuffer({ resolveWithObject: true });
const T = r.data, tw = r.info.width, th = r.info.height;
const px = (u, v) => [Math.min(tw - 1, Math.max(0, Math.floor((u - Math.floor(u)) * tw))), Math.min(th - 1, Math.max(0, Math.floor((v - Math.floor(v)) * th)))];
const colAt = (u, v) => { const [x, y] = px(u, v); const i = (y * tw + x) * 3; return [T[i], T[i + 1], T[i + 2]]; };
function classify(x, y, z, c) {
  const [R, G, B] = c, mx = Math.max(R, G, B), mn = Math.min(R, G, B), sat = mx ? (mx - mn) / mx : 0, lum = (R + G + B) / 3;
  const skin = R > 150 && R > G + 25 && G > B && sat > 0.2 && sat < 0.55;
  if (y > 1.52 || skin) return 0;
  const pack = y > 0.95 && Math.abs(x) < 0.21 && z < -0.12;
  const cloth = sat < 0.30 && lum > 50;
  if (!pack && !cloth && z < -0.06 && y > 0.3 && y < 1.45) return 1; // cloak
  if (cloth && y < 0.98 && y > 0.25) return 3;                         // trousers
  if (cloth && y >= 0.98) return 2;                                    // shirt
  return 0;
}
const n = pos.length / 3, reg = new Uint8Array(n);
for (let i = 0; i < n; i++) reg[i] = classify(pos[i * 3], pos[i * 3 + 1], pos[i * 3 + 2], colAt(uv[i * 2], uv[i * 2 + 1]));
// texture mask per region from its triangles' UVs
const mask = new Uint8Array(tw * th);
for (let t = 0; t < idx.length; t += 3) {
  const I = [idx[t], idx[t + 1], idx[t + 2]];
  const rg = reg[I[0]]; if (!rg || reg[I[1]] !== rg || reg[I[2]] !== rg) continue;
  const P = I.map(i => [(uv[i * 2] - Math.floor(uv[i * 2])) * tw, (uv[i * 2 + 1] - Math.floor(uv[i * 2 + 1])) * th]);
  const x0 = Math.floor(Math.min(...P.map(q => q[0]))) - 2, x1 = Math.ceil(Math.max(...P.map(q => q[0]))) + 2;
  const y0 = Math.floor(Math.min(...P.map(q => q[1]))) - 2, y1 = Math.ceil(Math.max(...P.map(q => q[1]))) + 2;
  const e = (a, b, x, y) => (b[0] - a[0]) * (y - a[1]) - (b[1] - a[1]) * (x - a[0]);
  const area = e(P[0], P[1], P[2][0], P[2][1]); if (Math.abs(area) < 1e-6) continue;
  for (let y = Math.max(0, y0); y <= Math.min(th - 1, y1); y++) for (let x = Math.max(0, x0); x <= Math.min(tw - 1, x1); x++) {
    const cx = x + 0.5, cy = y + 0.5, pad = 2.5 * Math.hypot(tw, th) / 1000;
    const w0 = e(P[1], P[2], cx, cy) / area, w1 = e(P[2], P[0], cx, cy) / area, w2 = e(P[0], P[1], cx, cy) / area;
    if (w0 > -0.06 && w1 > -0.06 && w2 > -0.06) mask[y * tw + x] = rg;
  }
}
// average colour per region (the undyed look), then grey the masked texels
const sum = [[0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0], [0, 0, 0, 0]];
for (let i = 0; i < tw * th; i++) { const m = mask[i]; if (!m) continue; sum[m][0] += T[i * 3]; sum[m][1] += T[i * 3 + 1]; sum[m][2] += T[i * 3 + 2]; sum[m][3]++; }
const avg = sum.map(s => s[3] ? [s[0] / s[3], s[1] / s[3], s[2] / s[3]] : [128, 128, 128]);
const K = 0.82;
for (let i = 0; i < tw * th; i++) {
  const m = mask[i]; if (!m) continue;
  const l = (T[i * 3] + T[i * 3 + 1] + T[i * 3 + 2]) / 3, al = (avg[m][0] + avg[m][1] + avg[m][2]) / 3;
  const g = Math.max(0, Math.min(255, Math.round(l / al * K * 255)));
  T[i * 3] = T[i * 3 + 1] = T[i * 3 + 2] = g;
}
const png = await sharp(T, { raw: { width: tw, height: th, channels: 3 } }).png({ compressionLevel: 9 }).toBuffer();
tex.setImage(new Uint8Array(png)).setMimeType('image/png');
const colors = new Uint8Array(n * 4);
const marker = [[255, 255, 255], [255, 0, 0], [0, 255, 0], [0, 0, 255]];
for (let i = 0; i < n; i++) { const m = marker[reg[i]]; colors.set([m[0], m[1], m[2], 255], i * 4); }
p.setAttribute('COLOR_0', doc.createAccessor().setType('VEC4').setArray(colors).setNormalized(true).setBuffer(doc.getRoot().listBuffers()[0]));
await io.write(f, doc);
const cnt = [0, 0, 0, 0]; for (const v of reg) cnt[v]++;
console.log('regions', cnt, 'undyed tints (x1/0.82):', avg.slice(1).map(a => a.map(v => Math.min(255, Math.round(v / K)))).map(a => '{' + a.join(', ') + '}').join(' '));
