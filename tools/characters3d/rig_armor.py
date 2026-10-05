"""Fit and skin the plate cuirass to the existing hero, without paid rigging.

Uses the hero's exact skeleton and torso weights. The base character is read-only.
Run from the repository root after build_clean_armor.py. Requires numpy, scipy, trimesh.
"""
import copy
import json
from pathlib import Path
import struct
import numpy as np
from scipy.spatial.transform import Rotation
from scipy.spatial import cKDTree
import trimesh

def glb(path):
    b=Path(path).read_bytes();n=struct.unpack_from('<I',b,12)[0]
    return json.loads(b[20:20+n]),b[28+n:]

def accessor(g,b,i):
    a=g['accessors'][i];v=g['bufferViews'][a['bufferView']]
    dtype={5126:'<f4',5125:'<u4',5123:'<u2',5121:'u1'}[a['componentType']]
    size={'SCALAR':1,'VEC2':2,'VEC3':3,'VEC4':4,'MAT4':16}[a['type']]
    off=v.get('byteOffset',0)+a.get('byteOffset',0)
    return np.ndarray((a['count'],size),dtype=dtype,buffer=b,offset=off,
        strides=(v.get('byteStride',np.dtype(dtype).itemsize*size),np.dtype(dtype).itemsize)).copy()

def main():
    hero='assets/characters3d/hero-neutral.glb'
    g,b=glb(hero);nodes=g['nodes'];names={n.get('name'):i for i,n in enumerate(nodes)}
    parents={c:i for i,n in enumerate(nodes) for c in n.get('children',[])}
    cache={}
    def world(i):
        if i not in cache:
            n=nodes[i];m=np.eye(4)
            m[:3,:3]=Rotation.from_quat(n.get('rotation',[0,0,0,1])).as_matrix()@np.diag(n.get('scale',[1,1,1]))
            m[:3,3]=n.get('translation',[0,0,0]);cache[i]=world(parents[i])@m if i in parents else m
        return cache[i]
    p=g['meshes'][0]['primitives'][0];a=p['attributes']
    body=accessor(g,b,a['POSITION']);weights=accessor(g,b,a['WEIGHTS_0']);jids=accessor(g,b,a['JOINTS_0'])
    skin=g['skins'][0];torso_ids=[skin['joints'].index(names[n]) for n in ['Spine','Spine01','Spine02','Hips']]
    # Exclude arms and hands from the fit and transferred bone weights.
    torso=np.isin(jids,torso_ids)*weights
    valid=(torso.sum(axis=1)>.85)&(body[:,1]>.97)&(body[:,1]<1.54)&(np.abs(body[:,0])<.22)
    ref=body[valid];tree=cKDTree(ref)
    m=trimesh.load('assets/armor/steel-cuirass.glb',force='scene').to_mesh()
    # Same rest placement as DrawSkinChar's chest attachment.
    center=world(names['Spine'])[:3,3]
    m.vertices=m.vertices*np.array([1.22*1.18,1.22,1.22])+center+np.array([0,-.12,.03])
    # Fit whole horizontal plate rings together. Per-vertex outward pushes
    # distort planar faces into random triangles and create crumpled lighting.
    for height in np.unique(m.vertices[:,1]):
        mask=np.abs(m.vertices[:,1]-height)<1e-7
        points=m.vertices[mask].copy()
        band=ref[np.abs(ref[:,1]-height)<.035]
        if not len(band):band=ref[np.argsort(np.abs(ref[:,1]-height))[:50]]
        cx=(band[:,0].min()+band[:,0].max())/2
        cz=(band[:,2].min()+band[:,2].max())/2
        old_center=(points[:,2].min()+points[:,2].max())/2
        old_rx=max(abs(points[:,0]).max(),.001)
        old_rz=max((points[:,2].max()-points[:,2].min())/2,.001)
        rx=max(old_rx,(band[:,0].max()-band[:,0].min())/2+.014)
        rz=max(old_rz,(band[:,2].max()-band[:,2].min())/2+.014)
        points[:,0]=cx+points[:,0]/old_rx*rx
        points[:,2]=cz+(points[:,2]-old_center)/old_rz*rz
        m.vertices[mask]=points
    # Transfer continuous torso weights using four neighboring body vertices.
    distances,near=tree.query(m.vertices,k=4)
    blend=1/np.maximum(distances,.002)**2;blend/=blend.sum(axis=1,keepdims=True)
    ref_weights=torso[valid];ref_ids=jids[valid]
    out_ids=np.zeros((len(m.vertices),4),dtype=np.uint8)
    out_weights=np.zeros((len(m.vertices),4),dtype=np.float32)
    for i in range(len(m.vertices)):
        accum={j:0.0 for j in torso_ids}
        for k in range(4):
            for joint,weight in zip(ref_ids[near[i,k]],ref_weights[near[i,k]]):
                if int(joint) in accum:accum[int(joint)]+=float(weight*blend[i,k])
        pairs=sorted(accum.items(),key=lambda p:-p[1])[:4];total=sum(v for _,v in pairs)
        for k,(j,w) in enumerate(pairs):out_ids[i,k]=j;out_weights[i,k]=w/total
    # Every ring uses one blend across its width, keeping broad plates from
    # wrinkling differently at each vertex as the character breathes or swings.
    dense=np.zeros((len(m.vertices),len(torso_ids)))
    for k,j in enumerate(torso_ids):dense[:,k]=(out_weights*(out_ids==j)).sum(axis=1)
    for height in np.unique(m.vertices[:,1]):
        mask=np.abs(m.vertices[:,1]-height)<1e-7
        avg=dense[mask].mean(axis=0);avg/=avg.sum()
        out_ids[mask]=torso_ids;out_weights[mask]=avg
    m.fix_normals(multibody=True)
    normals=m.face_normals.reshape(-1,2,3).mean(axis=1)
    normals/=np.linalg.norm(normals,axis=1)[:,None]
    m.vertex_normals=np.repeat(normals,6,axis=0)
    doc={'asset':{'version':'2.0','generator':'Town Forge local armor fit'},
         'nodes':copy.deepcopy(nodes),'scenes':copy.deepcopy(g['scenes']),'scene':g.get('scene',0),
         'bufferViews':[],'accessors':[],'materials':[{'pbrMetallicRoughness':{'baseColorFactor':[1,1,1,1],'metallicFactor':0,'roughnessFactor':1}}]}
    data=bytearray()
    def add(array,type_,ctype,normalized=False):
        while len(data)%4:data.append(0)
        array=np.ascontiguousarray(array);view=len(doc['bufferViews'])
        doc['bufferViews'].append({'buffer':0,'byteOffset':len(data),'byteLength':array.nbytes});data.extend(array.tobytes())
        a={'bufferView':view,'componentType':ctype,'count':len(array),'type':type_}
        if normalized:a['normalized']=True
        if type_=='VEC3':a.update(min=array.min(axis=0).tolist(),max=array.max(axis=0).tolist())
        doc['accessors'].append(a);return len(doc['accessors'])-1
    attrs={'POSITION':add(m.vertices.astype('<f4'),'VEC3',5126),
           'NORMAL':add(m.vertex_normals.astype('<f4'),'VEC3',5126),
           'COLOR_0':add(np.asarray(m.visual.vertex_colors,dtype='u1'),'VEC4',5121,True),
           'JOINTS_0':add(out_ids,'VEC4',5121),'WEIGHTS_0':add(out_weights,'VEC4',5126)}
    indices=add(m.faces.reshape(-1,1).astype('<u2'),'SCALAR',5123)
    doc['meshes']=[{'name':'Fitted plate cuirass','primitives':[{'attributes':attrs,'indices':indices,'material':0}]}]
    doc['skins']=[copy.deepcopy(skin)]
    doc['skins'][0]['inverseBindMatrices']=add(accessor(g,b,skin['inverseBindMatrices']).astype('<f4'),'MAT4',5126)
    for n in doc['nodes']:
        if 'mesh' in n:n['mesh']=0;n['skin']=0;n['name']='Fitted cuirass'
    doc['buffers']=[{'byteLength':len(data)}]
    encoded=json.dumps(doc,separators=(',',':')).encode();encoded+=b' '*((-len(encoded))%4);data+=b'\0'*((-len(data))%4)
    out=struct.pack('<4sII',b'glTF',2,28+len(encoded)+len(data))+struct.pack('<I4s',len(encoded),b'JSON')+encoded+struct.pack('<I4s',len(data),b'BIN\0')+data
    Path('assets/armor/steel-cuirass-skinned.glb').write_bytes(out)
    print('Locally skinned cuirass:',len(out),'bytes; original hero unchanged; no Meshy rigging request')

if __name__=='__main__':main()
