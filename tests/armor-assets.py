"""Check shipped plate armor exports: dimensions, lighting, mirroring and budget."""
import json
import math
from pathlib import Path
import struct

root = Path('assets/armor')
manifest = json.loads((root / 'meshy-armor.json').read_text())
assert manifest['credits'] == 60
formats = {5126: 'f', 5125: 'I', 5123: 'H', 5121: 'B'}
sizes = {'SCALAR': 1, 'VEC3': 3, 'VEC4': 4}
def read(path, skinned=False):
    b = path.read_bytes()
    assert b[:4] == b'glTF' and struct.unpack_from('<II', b, 4) == (2, len(b))
    n = struct.unpack_from('<I', b, 12)[0]
    g = json.loads(b[20:20+n]); binary = b[28+n:]
    assert bool(g.get('skins')) == skinned
    assert not g.get('animations') and not g.get('images')
    if skinned:
        hero = Path('assets/characters3d/hero-neutral.glb').read_bytes()
        hn = struct.unpack_from('<I', hero, 12)[0]
        h = json.loads(hero[20:20+hn])
        assert g['skins'][0]['joints'] == h['skins'][0]['joints']
        for i in g['skins'][0]['joints']:
            assert g['nodes'][i] == h['nodes'][i], 'Exact existing bind skeleton required'
    def values(i):
        a = g['accessors'][i]; v = g['bufferViews'][a['bufferView']]
        fmt = '<' + formats[a['componentType']] * sizes[a['type']]
        stride = v.get('byteStride', struct.calcsize(fmt))
        offset = v.get('byteOffset', 0) + a.get('byteOffset', 0)
        return [struct.unpack_from(fmt, binary, offset + j*stride) for j in range(a['count'])]
    points = []; count = 0
    for mesh in g['meshes']:
        for p in mesh['primitives']:
            attrs = p['attributes']; assert {'POSITION', 'NORMAL', 'COLOR_0'} <= attrs.keys()
            pos = values(attrs['POSITION']); normals = values(attrs['NORMAL'])
            assert len(pos) == len(normals) < 65536
            assert all(math.isfinite(x) for point in pos for x in point)
            assert all(abs(sum(x*x for x in normal)-1) < .005 for normal in normals)
            if skinned:
                assert {'JOINTS_0', 'WEIGHTS_0'} <= attrs.keys()
                assert all(max(j) < len(g['skins'][0]['joints']) for j in values(attrs['JOINTS_0']))
                assert all(abs(sum(w)-1) < .001 and min(w) >= 0 for w in values(attrs['WEIGHTS_0']))
            indices = [i[0] for i in values(p['indices'])]
            assert len(indices) % 3 == 0 and min(indices) >= 0 and max(indices) < len(pos)
            count += len(indices) // 3; points += pos
    return points, count, len(b)

total_triangles = total_bytes = 0
for name, piece in manifest['pieces'].items():
    meshes = [read(root / f) for f in piece['files']]
    for points, count, size in meshes:
        assert count == piece['triangles'] and count <= 3000
        low = [min(v[i] for v in points) for i in range(3)]
        high = [max(v[i] for v in points) for i in range(3)]
        for i in range(3):
            assert abs(low[i]-piece['bounds'][0][i]) < 1e-5
            assert abs(high[i]-piece['bounds'][1][i]) < 1e-5
        total_triangles += count; total_bytes += size
    if len(meshes) == 2:
        a = sorted(tuple(round(x, 5) for x in v) for v in meshes[0][0])
        b = sorted((round(-v[0],5),round(v[1],5),round(v[2],5)) for v in meshes[1][0])
        assert a == b, 'Left armor must mirror the right geometry'
assert total_triangles <= 4000 and total_bytes < 200_000
points, count, size = read(root / 'steel-cuirass-skinned.glb', True)
assert count == manifest['pieces']['chest']['triangles']
assert all(.98 <= v[1] <= 1.50 and abs(v[0]) < .26 and abs(v[2]) < .26 for v in points)
assert total_bytes + size < 300_000
print(f'PASS armor: {total_triangles} equipped triangles, {total_bytes+size:,} asset bytes, lighting normals, mirrored limbs, exact hero skeleton, normalized skin weights, 60-credit provenance')
# Cloth draft must keep the exact hero rig and survive the same weight checks.
tunic = root / 'armored-tunic-skinned.glb'
if tunic.exists():
    points, count, size = read(tunic, True)
    assert count < 3500 and size < 500_000
    assert all(.83 <= v[1] <= 1.50 and abs(v[0]) < .26 and abs(v[2]) < .27 for v in points)
    print(f'PASS fitted tunic: {count} triangles, {size:,} bytes, exact hero skeleton and normalized weights')
