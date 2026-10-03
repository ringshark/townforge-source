"""Build clean plate replacements locally; consumes no Meshy credits.

The approved Meshy attempts remain recorded in the ledger, but their extracted
geometry is retired. These open shells use deliberate symmetric plate profiles.
Run this followed by rig_armor.py from the repository root.
"""
import json
from pathlib import Path
import numpy as np
import trimesh

ROOT=Path('assets/armor')
PROFILE=np.array([[0,1],[.65,.94],[.94,.66],[1,.1],[.94,-.66],[.65,-.94],[0,-1],[-.65,-.94],[-.94,-.66],[-1,.1],[-.94,.66],[-.65,.94]])

def shell(rings, front_only=False):
    vertices=[];faces=[]
    # Open ends leave room for neck/arms/leg. A recessed inner edge gives each
    # opening physical thickness without filling it with a solid cap.
    rings=[(y,w,d,z) for y,w,d,z in rings]
    for y,w,d,z in rings:
        vertices.extend([[x*w,y,pz*d+z] for x,pz in PROFILE])
    for j in range(len(rings)-1):
        for k in range(12):
            if front_only and PROFILE[k,1]<-.2:continue
            a=j*12+k;b=j*12+(k+1)%12;c=b+12;d=a+12
            faces.extend([[a,b,c],[a,c,d]])
    mesh=trimesh.Trimesh(vertices,faces,process=False)
    mesh.fix_normals()
    return mesh

def plate_set(kind):
    if kind=='chest':
        # Waist, lower ribs, broad breastplate, inward shoulder line.
        parts=[shell([(-.25,.116,.085,0),(-.23,.12,.089,0),(-.15,.119,.086,0),(-.03,.135,.10,0),(.08,.13,.09,0),(.13,.105,.07,0)])]
        # Belt-like overlapping waist plates, with controlled relief.
        parts.append(shell([(-.245,.118,.088,.002),(-.215,.121,.091,.002)]))
    elif kind=='shoulder':
        parts=[shell([(-.045,.040,.050,0),(-.015,.074,.079,0),(.025,.084,.084,0),(.080,.078,.074,0),(.110,.066,.062,0)])]
        parts.append(shell([(.080,.080,.077,0),(.118,.068,.065,0),(.155,.057,.052,0)]))
    else:
        # Slim shin plate with a central ridge, flared at the knee.
        parts=[shell([(.015,.062,.067,.008),(.045,.060,.065,.008),(.15,.050,.055,.008),(.29,.040,.048,.008),(.355,.042,.050,.008)])]
    return trimesh.util.concatenate(parts)

def main():
    ledger=json.loads((ROOT/'meshy-armor.json').read_text())
    ledger['geometry']='Locally rebuilt plate shells; extracted Meshy meshes retired'
    for name,piece in ledger['pieces'].items():
        mesh=plate_set(name)
        # Split faces for crisp plate edges and stable lighting, rather than
        # smoothing tiny generated triangles into a lumpy surface.
        mesh.unmerge_vertices()
        normals=mesh.face_normals.reshape(-1,2,3).mean(axis=1)
        normals/=np.linalg.norm(normals,axis=1)[:,None]
        mesh.vertex_normals=np.repeat(normals,6,axis=0)
        mesh.visual=trimesh.visual.ColorVisuals(mesh=mesh,vertex_colors=np.tile([240,240,240,255],(len(mesh.vertices),1)))
        mesh.export(ROOT/piece['files'][0],include_normals=True)
        if len(piece['files'])>1:
            mirror=mesh.copy();mirror.apply_transform(np.diag([-1,1,1,1]))
            mirror.export(ROOT/piece['files'][1],include_normals=True)
        piece['triangles']=len(mesh.faces);piece['bounds']=mesh.bounds.tolist()
        piece['geometry']='local plate rebuild'
        print(name,len(mesh.faces),'triangles')
    (ROOT/'meshy-armor.json').write_text(json.dumps(ledger,indent=2)+'\n')

if __name__=='__main__':main()
