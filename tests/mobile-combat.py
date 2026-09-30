"""Run production mobile geometry, gesture and cover-aware Hiding rules."""
from pathlib import Path
import subprocess,tempfile
s=Path('main.cpp').read_text()
def section(a,b):
    start=s.index(a);return s[start:s.index(b,start)]
code='''#include <algorithm>
#include <cmath>
#include <cassert>
struct Vector2 {float x,y;};struct Rectangle {float x,y,width,height;};
constexpr Rectangle kViewport={0,110,540,790};
bool overlap(Rectangle a,Rectangle b){return a.x<b.x+b.width && b.x<a.x+a.width && a.y<b.y+b.height && b.y<a.y+a.height;}
'''
code+=section('static Rectangle CombatHotbarSlotRect(', 'static int DrawCombatHotbarRow(')
code+=section('static Rectangle TargetButtonRect()', '// ---------------------------------------------------------------------')
code+=section('struct DenGroundGesture {', '// Blackwake Den:')
code+=section('static float HidingSuccessChance(', 'static float HideChance(')
code+='''int main(){
Rectangle target=TargetButtonRect(),stick={0,730,170,170},belt={166,600,342,76};
assert(target.width>=64 && target.height>=64);
assert(!overlap(target,stick) && !overlap(target,belt));
for(int i=0;i<8;i++){
 Rectangle a=CombatHotbarSlotRect(i,190,700);
 assert(a.width>=72 && a.height>=72 && a.x>=170 && a.x+a.width<=540 && a.y+a.height<=900);
 assert(!overlap(a,target) && !overlap(a,stick) && !overlap(a,belt));
 for(int j=0;j<i;j++) assert(!overlap(a,CombatHotbarSlotRect(j,190,700)));
}
DenGroundGesture g;
assert(!g.Update({300,400},true,true,false,true));
assert(g.Update({302,402},false,false,true,true));
assert(!g.Update({302,402},false,false,true,true));
g.Update({300,400},true,true,false,true);g.Update({350,400},false,true,false,true);
assert(!g.Update({300,400},false,false,true,true));
g.Update({80,800},true,true,false,false);
assert(!g.Update({300,400},false,false,true,true));
g.Update({300,400},true,true,false,true);g.Update({300,400},false,true,false,false);
assert(!g.Update({300,400},false,false,true,true));
g.Update({300,400},true,true,false,true);
assert(!g.Update({500,400},false,false,true,true));
for(float skill:{0.f,10.f,20.f,30.f}) {
 assert(HidingSuccessChance(skill,false,false)==0);
 assert(HidingSuccessChance(skill,true,false)>0);
}
for(float skill:{40.f,60.f,80.f,100.f}) {
 float open=HidingSuccessChance(skill,false,false),cover=HidingSuccessChance(skill,true,false);
 assert(open>0 && cover>open && cover<=97);
 assert(HidingSuccessChance(skill,true,true)<cover);
}
assert(HidingSuccessChance(100,false,false)==91);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'mobile.cpp';exe=Path(tmp)/'mobile';cpp.write_text(code)
    subprocess.run(['g++','-std=c++17',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS production combat layout, Den gestures, and cover-aware Hiding chances')
