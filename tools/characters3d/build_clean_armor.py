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
# Rounded plate faces with a restrained central breast/shin ridge.
angles=np.linspace(0,2*np.pi,24,endpoint=False)
PROFILE=np.column_stack([np.sin(angles),np.cos(angles)])
PROFILE[:,1]*=.94
PROFILE[0,1]=1
N=len(PROFILE)

def shell(rings, front_only=False):
    vertices=[];faces=[]
    # Open ends leave room for neck/arms/leg. A recessed inner edge gives each
    # opening physical thickness without filling it with a solid cap.
    rings=[(y,w,d,z) for y,w,d,z in rings]
    for y,w,d,z in rings:
        vertices.extend([[x*w,y,pz*d+z] for x,pz in PROFILE])
    for j in range(len(rings)-1):
        for k in range(N):
            if front_only and PROFILE[k,1]<-.2:continue
            a=j*N+k;b=j*N+(k+1)%N;c=b+N;d=a+N
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
        parts=[shell([(-.015,.030,.038,0),(.005,.058,.060,0),(.030,.064,.062,0),(.065,.058,.056,0),(.085,.050,.048,0)])]
        parts.append(shell([(.060,.059,.057,0),(.085,.051,.049,0),(.115,.043,.044,0)]))
    else:
        # Slim shin plate with a central ridge, flared at the knee.
        parts=[shell([(.015,.051,.057,.004),(.045,.050,.055,.004),(.15,.042,.047,.004),(.29,.035,.041,.004),(.325,.037,.043,.004)])]
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
        colors=np.tile([228,234,242,255],(len(mesh.vertices),1))
        if name=='chest':
            low=mesh.vertices[:,1]<-.215
            colors[low]=[112,80,47,255]
            edge=(mesh.vertices[:,1]>.075)|(np.abs(mesh.vertices[:,1]+.15)<.005)
            colors[edge]=[191,198,210,255]
        elif name=='shoulder':colors[mesh.vertices[:,1]>.08]=[194,202,216,255]
        mesh.visual=trimesh.visual.ColorVisuals(mesh=mesh,vertex_colors=colors)
        mesh.export(ROOT/piece['files'][0],include_normals=True)
        if len(piece['files'])>1:
            mirror=mesh.copy();mirror.apply_transform(np.diag([-1,1,1,1]))
            mirror.export(ROOT/piece['files'][1],include_normals=True)
        piece['triangles']=len(mesh.faces);piece['bounds']=mesh.bounds.tolist()
        piece['geometry']='local plate rebuild'
        print(name,len(mesh.faces),'triangles')
    (ROOT/'meshy-armor.json').write_text(json.dumps(ledger,indent=2)+'\n')

if __name__=='__main__':main()
