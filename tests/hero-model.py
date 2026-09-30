"""Validate the shipped avatar's skin and clips without graphics dependencies."""
import json, math, struct
from pathlib import Path
p = Path('assets/characters3d/hero-neutral.glb')
b = p.read_bytes()
assert b[:4] == b'glTF' and struct.unpack_from('<I', b, 8)[0] == len(b)
assert len(b) < 3_000_000, 'Mobile avatar budget'
n = struct.unpack_from('<I', b, 12)[0]
g = json.loads(b[20:20+n]); binary = b[28+n:]
formats = {5126:'f', 5125:'I', 5123:'H', 5121:'B'}
sizes = {'SCALAR':1, 'VEC2':2, 'VEC3':3, 'VEC4':4, 'MAT4':16}
def values(i):
    a = g['accessors'][i]; v = g['bufferViews'][a['bufferView']]
    fmt = '<' + formats[a['componentType']] * sizes[a['type']]
    stride = v.get('byteStride', struct.calcsize(fmt))
    offset = v.get('byteOffset', 0) + a.get('byteOffset', 0)
    return [struct.unpack_from(fmt, binary, offset + j*stride) for j in range(a['count'])]
assert len(g['skins']) == 1
skin = g['skins'][0]; names = {g['nodes'][i]['name'] for i in skin['joints']}
assert {'Hips','Head','Spine','RightHand','LeftHand','RightFoot','LeftFoot'} <= names
triangles = 0
for mesh in g['meshes']:
    for p in mesh['primitives']:
        at = p['attributes']; triangles += g['accessors'][p['indices']]['count']//3
        assert g['accessors'][at['POSITION']]['count'] < 65536
        for j in values(at['JOINTS_0']): assert max(j) < len(skin['joints'])
        for w in values(at['WEIGHTS_0']): assert abs(sum(w)-1) < 0.002
        for xyz in values(at['POSITION']): assert all(math.isfinite(x) for x in xyz)
        assert 'COLOR_0' in at, 'Equipment dye regions'
assert triangles <= 20000
clips = {a['name'] for a in g['animations']}
assert {'Idle','walking_man','running','Combat_Stance','Right_Hand_Sword_Slash','Sword_Parry','Hit_Reaction','dying_backwards','mage_soell_cast'} <= clips
for animation in g['animations']:
    for sampler in animation['samplers']:
        times = [x[0] for x in values(sampler['input'])]
        assert len(times) >= 2 and times == sorted(times) and times[-1] > times[0]
        for frame in values(sampler['output']): assert all(math.isfinite(x) for x in frame)
print(f'PASS hero: {len(b):,} bytes, {triangles:,} triangles, {len(clips)} clips, valid skin weights and equipment masks')
