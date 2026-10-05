"""Bake a plain fabric underlayer into the hero's existing UV atlas.

Preserves the base model, skeleton and uncovered head/hands/feet. No remote jobs.
"""
from io import BytesIO
from pathlib import Path
import numpy as np
from PIL import Image, ImageDraw, ImageFilter
from rig_armor import glb, accessor

def main():
    g,b=glb('assets/characters3d/hero-neutral.glb')
    view=g['bufferViews'][g['images'][0]['bufferView']]
    image=Image.open(BytesIO(b[view['byteOffset']:view['byteOffset']+view['byteLength']])).convert('RGB')
    w,h=image.size;p=g['meshes'][0]['primitives'][0];a=p['attributes']
    pos=accessor(g,b,a['POSITION']);uv=accessor(g,b,a['TEXCOORD_0']);faces=accessor(g,b,p['indices']).reshape(-1,3)
    # Sleeve ends stop above the hands; neckline and exposed feet stay skin.
    cloth=(pos[:,1]>.24)&(pos[:,1]<1.47)&~((np.abs(pos[:,0])>.36)&(pos[:,1]<1.09))
    mask=Image.new('L',(w,h));draw=ImageDraw.Draw(mask)
    for face in faces:
        if not np.all(cloth[face]):continue
        points=[(float(uv[i,0]*w),float(uv[i,1]*h)) for i in face]
        draw.polygon(points,fill=255)
    mask=mask.filter(ImageFilter.MaxFilter(3))
    original=np.array(image).astype(float);lum=original.mean(axis=2)
    # Broad original folds remain; no repeating chain rings or speckle.
    smooth=np.asarray(image.filter(ImageFilter.GaussianBlur(2))).astype(float).mean(axis=2)
    value=np.clip(185+(smooth-110)*.45,110,235)
    fabric=np.stack([value,value,value],axis=2)
    out=np.where(np.array(mask)[:,:,None]>0,fabric,original).astype('uint8')
    path=Path('assets/armor/tunic-underlayer.png');Image.fromarray(out).quantize(colors=224,dither=Image.Dither.NONE).save(path,optimize=True)
    print('Fabric underlayer:',path.stat().st_size,'bytes; uncovered skin and source model preserved')
if __name__=='__main__':main()
