"""Check the complete knight skin's animation/rig budget and held-gear boundary."""
from pathlib import Path
import json,struct,subprocess,tempfile
s=Path('main.cpp').read_text();b=Path('assets/characters3d/hero_knight.glb').read_bytes()
n=struct.unpack_from('<I',b,12)[0];g=json.loads(b[20:20+n]);data=b[28+n:]
assert len(b)<1_000_000
assert {'walking_man','running','Right_Hand_Sword_Slash','mage_soell_cast','Sword_Parry','dying_backwards'} <= {a['name'] for a in g['animations']}
assert {'Hips','Head','RightHand','LeftHand','RightFoot','LeftFoot'} <= {g['nodes'][i]['name'] for i in g['skins'][0]['joints']}
for m in g['meshes']:
 for p in m['primitives']:
  attrs=p['attributes'];assert {'POSITION','TEXCOORD_0','JOINTS_0','WEIGHTS_0'}<=attrs.keys()
  a=g['accessors'][attrs['WEIGHTS_0']];v=g['bufferViews'][a['bufferView']];off=v.get('byteOffset',0)+a.get('byteOffset',0)
  for k in range(a['count']):
   w=struct.unpack_from('<4f',data,off+k*v.get('byteStride',16));assert min(w)>=0 and abs(sum(w)-1)<.005
start=s.index('static HumanOutfit SkinHeldGearFor(');helper=s[start:s.index('\n}',start)+2]
code='''#include <cassert>
const int kHhNone=0,kClStyleNone=0;
struct HumanOutfit {int armChest=2,armArms=2,armLegs=2,armGorget=2,helm=1,hat=1;bool cloak=true,robe=true,footwear=true;unsigned outfitMask=31,hideRegions=31;int weapon=5,meshWeapon=8,style=1;bool shield=true,meshShield=true;float weaponScale=.75f;};
'''+helper+'''
int main(){HumanOutfit original;auto held=SkinHeldGearFor(original);
assert(original.armChest==2&&original.cloak&&original.robe);
assert(held.weapon==original.weapon&&held.meshWeapon==original.meshWeapon&&held.style==original.style);
assert(held.shield&&held.meshShield&&held.weaponScale==original.weaponScale);
assert(!held.armChest&&!held.armArms&&!held.armLegs&&!held.armGorget&&!held.cloak&&!held.robe&&!held.footwear&&!held.outfitMask);
}
'''
with tempfile.TemporaryDirectory() as tmp:
 path=Path(tmp)/'skin.cpp';exe=Path(tmp)/'skin';path.write_text(code)
 subprocess.run(['g++','-std=c++17',str(path),'-o',str(exe)],check=True);subprocess.run([str(exe)],check=True)
print(f'PASS complete knight: {len(b):,} bytes, animation/rig compatibility, normalized weights; baked outfit keeps held weapons, shields and combat style')
