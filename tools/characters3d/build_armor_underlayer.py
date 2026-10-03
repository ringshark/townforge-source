"""Bake a chain underlayer into the hero's existing UV atlas.

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
    yy,xx=np.mgrid[:h,:w];row=yy//5
    dx=((xx+(row%2)*3)%6)-2.5;dy=(yy%5)-2
    radius=(dx/2.5)**2+(dy/1.7)**2
    link=np.where((radius>.42)&(radius<1.45),1.,0.)
    relief=np.where((radius>.60)&(radius<1.30)&(dy<0),1.,0.)
    value=np.clip(70+link*34+relief*20+(lum-110)*.30,40,178)
    fabric=np.stack([value*.92,value*.96,value],axis=2)
    out=np.where(np.array(mask)[:,:,None]>0,fabric,original).astype('uint8')
    path=Path('assets/armor/chain-underlayer.png');Image.fromarray(out).quantize(colors=224,dither=Image.Dither.NONE).save(path,optimize=True)
    print('Chain underlayer:',path.stat().st_size,'bytes; uncovered skin and source model preserved')
if __name__=='__main__':main()
