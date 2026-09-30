"""Exercise the production cape transform and independent actor animation tracks.

RAYLIB_DIR must point at raylib 6.0; no window or GPU is needed.
"""
import os
import subprocess
import tempfile
from pathlib import Path

s = Path('main.cpp').read_text()
start = s.index('static Matrix SkinCloakLocal(')
cape = s[start:s.index('\nstatic bool DrawSkinChar(', start)]
start = s.index('struct SkinAnimState {')
state = s[start:s.index('// Meshy\'s walk', start)]
code = '''#include <algorithm>
#include <cassert>
#include <cmath>
#include <map>
#include <utility>
#include "raymath.h"
'''+cape+state+'''
int main() {
    // Every sampled vertex stays behind the spine, at rest and full run.
    for (float u : {0.5f, 1.0f, 100.0f}) {
        Vector3 anchor{};
        float oldHem = 0;
        for (int speed=0; speed<=10; ++speed) {
            auto m = SkinCloakLocal(u, speed/10.0f, MatrixIdentity());
            auto shoulder = Vector3Transform({0,0.22f,0},m);
            if (!speed) anchor=shoulder;
            assert(Vector3Distance(shoulder,anchor)<1e-4f); // tilt never moves the collar
            for (int col=0; col<=6; ++col) for (int row=0; row<=4; ++row) {
                float x=-1.0f+col/3.0f, t=row/4.0f;
                float y=0.22f-1.08f*t;
                float z=-0.12f-0.07f*t-0.04f*(1-x*x)+0.10f*x*x*(1-0.4f*t);
                auto p=Vector3Transform({x*(0.15f+0.09f*t),y,z},m);
                assert(p.z<0); // front is +Z
            }
            auto hem=Vector3Transform({0,-0.86f,-0.23f},m);
            if (speed) assert(hem.z<oldHem); // movement lifts the hem away from the legs
            oldHem=hem.z;
        }
        // Rest correction followed by the bone's bind rotation preserves the fit.
        Matrix bind=MatrixRotateXYZ({0.09f,0.02f,-0.02f});
        auto corrected=MatrixMultiply(SkinCloakLocal(u,1,MatrixInvert(bind)),bind);
        auto p=Vector3Transform({0,-0.86f,-0.23f},corrected);
        auto q=Vector3Transform({0,-0.86f,-0.23f},SkinCloakLocal(u,1,MatrixIdentity()));
        assert(Vector3Distance(p,q)<1e-4f);
    }
    // These actors collided in the old track*4+id table.
    g_skinAnim[{20,8}].atkVariant=9;
    g_skinAnim[{21,4}].atkVariant=3;
    g_skinAnim[{22,0}].atkVariant=1;
    assert(g_skinAnim.size()==3);
    assert((g_skinAnim[{20,8}].atkVariant==9));
    assert((g_skinAnim[{21,4}].atkVariant==3));
}
'''
with tempfile.TemporaryDirectory() as temp:
    cpp=Path(temp)/'avatar.cpp'; exe=Path(temp)/'avatar'
    cpp.write_text(code)
    subprocess.run(['g++','-std=c++17','-I',str(Path(os.environ['RAYLIB_DIR'])/'src'),str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS cape behind torso, anchored collar, backward lift, bind correction and independent model/actor tracks')
