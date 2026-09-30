// Optimize an unrigged GLB without changing its rest pose or generating new art.
// node optimize-base.mjs source.glb output.glb triangles textureSize
import { NodeIO } from '@gltf-transform/core';
import { ALL_EXTENSIONS } from '@gltf-transform/extensions';
import { prune, dedup, textureCompress, compactPrimitive } from '@gltf-transform/functions';
import { MeshoptSimplifier } from 'meshoptimizer';
import sharp from 'sharp';
const [input, output, target='20000', texSize='1024'] = process.argv.slice(2);
const io=new NodeIO().registerExtensions(ALL_EXTENSIONS);
const doc=await io.read(input);
if(doc.getRoot().listSkins().length) throw Error('Use the rig-aware optimizer for skinned models.');
await MeshoptSimplifier.ready;
let before=0,after=0;
for(const mesh of doc.getRoot().listMeshes()) for(const p of mesh.listPrimitives()) {
  const index=p.getIndices(),positions=p.getAttribute('POSITION');
  const ia=new Uint32Array(index.getArray()),pa=new Float32Array(positions.getArray());
  const [reduced,error]=MeshoptSimplifier.simplify(ia,pa,3,Math.min(ia.length,+target*3),.015,[]);
  before+=ia.length/3;after+=reduced.length/3;
  index.setArray(new Uint32Array(reduced));compactPrimitive(p);
  console.log('surface error',error);
}
for(const m of doc.getRoot().listMaterials()) {
  m.setNormalTexture(null);m.setMetallicRoughnessTexture(null);m.setOcclusionTexture(null);m.setEmissiveTexture(null);
  m.setMetallicFactor(0);m.setRoughnessFactor(1);
}
await doc.transform(prune(),dedup(),textureCompress({encoder:sharp,targetFormat:'png',resize:[+texSize,+texSize]}));
await io.write(output,doc);
console.log(JSON.stringify({before,after,textureSize:+texSize}));
