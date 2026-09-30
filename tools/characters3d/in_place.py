"""Keep a GLB's hips anchored horizontally; preserve vertical motion and all rotations.

Run after retargeting: python tools/characters3d/in_place.py <character.glb>
The game owns movement/collision. Animation root travel must not move the body
away from its hitbox, then teleport it back when a clip loops or changes.
"""
import json
import struct
import sys
from pathlib import Path


def anchor(path):
    data = bytearray(path.read_bytes())
    assert data[:4] == b'glTF'
    size = struct.unpack_from('<I', data, 12)[0]
    gltf = json.loads(data[20:20 + size])
    binary = 28 + size
    count = 0
    for animation in gltf.get('animations', []):
        for channel in animation['channels']:
            target = channel['target']
            node = gltf['nodes'][target['node']]
            if target['path'] != 'translation' or node.get('name') != 'Hips':
                continue
            sampler = animation['samplers'][channel['sampler']]
            accessor = gltf['accessors'][sampler['output']]
            assert accessor['componentType'] == 5126 and accessor['type'] == 'VEC3'
            assert sampler.get('interpolation', 'LINEAR') in ('LINEAR', 'STEP')
            view = gltf['bufferViews'][accessor['bufferView']]
            offset = binary + view.get('byteOffset', 0) + accessor.get('byteOffset', 0)
            stride = view.get('byteStride', 12)
            x, _, z = node.get('translation', [0, 0, 0])
            for i in range(accessor['count']):
                struct.pack_into('<f', data, offset + i * stride, x)
                struct.pack_into('<f', data, offset + i * stride + 8, z)
            count += 1
    path.write_bytes(data)
    return count


if __name__ == '__main__':
    path = Path(sys.argv[1])
    print(f'Anchored {anchor(path)} clips in {path}')
