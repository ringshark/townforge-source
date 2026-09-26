# Writes terrain_data.inc: the C++ tables to paste over the
# "Generated wilderness terrain data" block in main.cpp.
import json
R = json.load(open('relaxed.json')); RW = json.load(open('rivers_out.json'))
exec(open('spec.py').read())
out = ['// ---- Generated wilderness terrain data (2026-09-26) ----',
       '// Produced offline by tools/wild_terrain (routes each river/ridge around every',
       '// gameplay position and checks everything stays reachable from the Emberhold',
       '// gate). Rivers carry a per-point half-width (they narrow where squeezed).',
       '// Re-run the tool, do not hand-edit, if spots or plots move.',
       'struct WildPathPt { float x, z, hw; };']
def arr(name, pts, ws):
    rows = ['{%g,%g,%g}' % (round(x), round(z), round(w)) for (x, z), w in zip(pts, ws)]
    return 'static const WildPathPt %s[] = {\n' % name + ''.join('    ' + ', '.join(rows[i:i + 8]) + ',\n' for i in range(0, len(rows), 8)) + '};'
rivers = [('w_river', 'kWildRiverSilverrun'), ('w_outflow', 'kWildRiverOutflow'), ('w_creek', 'kWildRiverCreek')]
for k, name in rivers: out.append(arr(name, RW[k]['pl'], RW[k]['w']))
ridges = [k for k in SPEC if k.startswith('r_')]
for i, k in enumerate(ridges): out.append(arr('kWildRidge%d' % i, R[k]['pl'], [SPEC[k][1]] * len(R[k]['pl'])))
ref = lambda n: '{%s, (int)(sizeof(%s)/sizeof(WildPathPt))}' % (n, n)
out.append('struct WildPathRef { const WildPathPt* pts; int n; };')
out.append('static const WildPathRef kWildRivers[] = { ' + ', '.join(ref(n) for _, n in rivers) + ' };')
out.append('static const WildPathRef kWildRidges[] = { ' + ', '.join(ref('kWildRidge%d' % i) for i in range(len(ridges))) + ' };')
out.append("// Branch roads (control points, Catmull-Rom smoothed at load). The King's Road")
out.append('// itself is kKingsRoadWaypoints, smoothed the same way.')
for i, c in enumerate(ROADCTRL[1:]): out.append('static const Vector2 kWildBranchRoad%d[] = { %s };' % (i, ', '.join('{%d,%d}' % p for p in c)))
out.append('static const WildRoadRef kWildBranchRoads[] = { ' + ', '.join('{kWildBranchRoad%d, %d}' % (i, len(c)) for i, c in enumerate(ROADCTRL[1:])) + ' };')
out.append('static const Vector2 kWildFords[] = { %s };' % ', '.join('{%d,%d}' % f for f in FORDS))
out.append('static const float kWildLakeX = %g, kWildLakeZ = %g, kWildLakeRX = %g, kWildLakeRZ = %g; // Mirrormere' % LAKE)
open('terrain_data.inc', 'w').write('\n'.join(out) + '\n')
print('wrote terrain_data.inc')
