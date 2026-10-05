"""Fit a cloth tunic to the existing hero torso with its original skin weights.

No remote generation or rigging. Run from the repository root.
"""
import copy,json,struct
from pathlib import Path
import numpy as np
from scipy.spatial import cKDTree
from rig_armor import glb,accessor

def main():
    g,b=glb('assets/characters3d/hero-neutral.glb'); a=g['meshes'][0]['primitives'][0]['attributes']
    pos=accessor(g,b,a['POSITION']);normal=accessor(g,b,a['NORMAL'])
    weights=accessor(g,b,a['WEIGHTS_0']);joints=accessor(g,b,a['JOINTS_0'])
    faces=accessor(g,b,g['meshes'][0]['primitives'][0]['indices']).reshape(-1,3)
    torso=[i for i,n in enumerate(g['skins'][0]['joints']) if g['nodes'][n].get('name') in ('Hips','Spine','Spine01','Spine02')]
    covered=(pos[:,1]>1.025)&(pos[:,1]<1.455)&(np.abs(pos[:,0])<.225)
    chosen=faces[np.all(covered[faces],axis=1)].reshape(-1)
    p=pos[chosen]+normal[chosen]*.018;n=normal[chosen];ji=joints[chosen];we=weights[chosen]
    # Continuous waist and flared hem, with a leather belt and bound cloth edges.
    rings=[(.85,.193,.13),(.865,.191,.129),(.94,.173,.122),(1.02,.179,.15),(1.035,.179,.15),(1.068,.176,.149),(1.10,.173,.147)]
    verts=[];normals=[];colors=[]
    for y,rx,rz in rings:
        for k in range(32):
            angle=2*np.pi*k/32
            verts.append([rx*np.sin(angle),y,rz*np.cos(angle)-.015])
            v=np.array([np.sin(angle)/rx,0,np.cos(angle)/rz]);normals.append(v/np.linalg.norm(v))
            colors.append([79,51,28,255] if 1.03<y<1.07 else [159,126,65,255] if y<.87 else [39,65,84,255])
    ids=[]
    for r in range(len(rings)-1):
        for k in range(32):
            aa=r*32+k;bb=r*32+(k+1)%32;cc=bb+32;dd=aa+32
            ids.extend([aa,bb,cc,aa,cc,dd])
    ids=np.array(ids);vp=np.array(verts)[ids];vn=np.array(normals)[ids];vc=np.array(colors,dtype='u1')[ids]
    nearest=cKDTree(pos).query(vp)[1]
    c=np.tile([39,65,84,255],(len(p),1)).astype('u1')
    # A restrained center placket and neckline binding.
    seam=(np.abs(p[:,0])<.014)&(p[:,2]>.055)
    c[seam]=[136,110,65,255]
    p=np.concatenate([p,vp]);n=np.concatenate([n,vn]);c=np.concatenate([c,vc]);ji=np.concatenate([ji,joints[nearest]]);we=np.concatenate([we,weights[nearest]])
    doc={'asset':{'version':'2.0','generator':'Town Forge body-fitted tunic'},'nodes':copy.deepcopy(g['nodes']),'scenes':copy.deepcopy(g['scenes']),'scene':g.get('scene',0),'bufferViews':[],'accessors':[],'materials':[{'pbrMetallicRoughness':{'baseColorFactor':[1,1,1,1],'metallicFactor':0,'roughnessFactor':1}}]}
    data=bytearray()
    def add(array,typ,ctype,normalized=False):
        while len(data)%4:data.append(0)
        array=np.ascontiguousarray(array);vi=len(doc['bufferViews']);doc['bufferViews'].append({'buffer':0,'byteOffset':len(data),'byteLength':array.nbytes});data.extend(array.tobytes())
        ac={'bufferView':vi,'componentType':ctype,'count':len(array),'type':typ}
        if normalized:ac['normalized']=True
        if typ=='VEC3':ac.update(min=array.min(axis=0).tolist(),max=array.max(axis=0).tolist())
        doc['accessors'].append(ac);return len(doc['accessors'])-1
    attrs={'POSITION':add(p.astype('<f4'),'VEC3',5126),'NORMAL':add(n.astype('<f4'),'VEC3',5126),'COLOR_0':add(c,'VEC4',5121,True),'JOINTS_0':add(ji.astype('u1'),'VEC4',5121),'WEIGHTS_0':add(we.astype('<f4'),'VEC4',5126)}
    ix=add(np.arange(len(p),dtype='<u2').reshape(-1,1),'SCALAR',5123)
    doc['meshes']=[{'name':'Fitted armored tunic','primitives':[{'attributes':attrs,'indices':ix,'material':0}]}];doc['skins']=[copy.deepcopy(g['skins'][0])]
    doc['skins'][0]['inverseBindMatrices']=add(accessor(g,b,g['skins'][0]['inverseBindMatrices']).astype('<f4'),'MAT4',5126)
    for node in doc['nodes']:
        if 'mesh' in node:node.update(mesh=0,skin=0,name='Armored tunic')
    doc['buffers']=[{'byteLength':len(data)}];encoded=json.dumps(doc,separators=(',',':')).encode();encoded+=b' '*((-len(encoded))%4);data+=b'\0'*((-len(data))%4)
    out=struct.pack('<4sII',b'glTF',2,28+len(encoded)+len(data))+struct.pack('<I4s',len(encoded),b'JSON')+encoded+struct.pack('<I4s',len(data),b'BIN\0')+data
    path=Path('assets/armor/armored-tunic-skinned.glb');path.write_bytes(out)
    print('Tunic:',len(p)//3,'triangles;',len(out),'bytes')
if __name__=='__main__':main()
