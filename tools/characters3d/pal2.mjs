// any embedded image -> 256-colour palette PNG (raylib's web build reads PNG only)
import { NodeIO } from '@gltf-transform/core'; import { ALL_EXTENSIONS } from '@gltf-transform/extensions'; import sharp from 'sharp';
const io = new NodeIO().registerExtensions(ALL_EXTENSIONS);
for (const f of process.argv.slice(2)) {
  const doc = await io.read(f);
  for (const t of doc.getRoot().listTextures()) t.setImage(new Uint8Array(await sharp(Buffer.from(t.getImage())).png({ palette: true, quality: 90, effort: 10, dither: 0.6 }).toBuffer())).setMimeType('image/png');
  await io.write(f, doc);
}
