"""Run the production movement function with deterministic keyboard/stick input."""
from pathlib import Path
import subprocess, tempfile
s = Path('main.cpp').read_text()
start = s.index('static Vector2 g_moveVelocity =')
end = s.index('static float Dist(', start)
code = '''#include <algorithm>
#include <cmath>
#include <cassert>
#include "combat_motion.h"
struct Vector2 { float x=0,y=0; };
enum {KEY_W,KEY_UP,KEY_S,KEY_DOWN,KEY_A,KEY_LEFT,KEY_D,KEY_RIGHT};
bool keys[8]={}; double now=0; Vector2 stick{};
bool IsKeyDown(int k){return keys[k];} double GetTime(){return now;}
Vector2 VirtualJoystickDir(){return stick;}
constexpr float kPlayerSpeed=220,kPlayerEdgeMargin=70,kWorldSize=2000;
float g_moveSpeedMul=1;
bool g_walkOn=false; const Vector2* g_walkFor=nullptr;
Vector2 g_walkTarget{},g_walkLastPos{};float g_walkStuckT=0;
void WalkTargetClear(){g_walkOn=false;g_walkFor=nullptr;}
Vector2 ClampToWorld(Vector2 p,float m,float w){return {std::clamp(p.x,m,w-m),std::clamp(p.y,m,w-m)};}
'''+s[start:end]+'''int main(){
Vector2 pos{500,500},face{0,1};
auto tick=[&](float dt){now+=dt;return UpdatePlayerMovement(pos,face,dt);};
keys[KEY_D]=true; for(int i=0;i<60;i++)tick(1.f/60);
float straight=pos.x-500;assert(straight>210 && straight<=220.01f);
keys[KEY_D]=false;float stopped=pos.x;assert(!tick(1.f/60));assert(pos.x==stopped);
pos={500,500};keys[KEY_D]=true;keys[KEY_S]=true;
for(int i=0;i<60;i++)tick(1.f/60);
float diagonal=std::hypot(pos.x-500,pos.y-500);assert(std::abs(diagonal-straight)<.02f);
keys[KEY_D]=false;keys[KEY_S]=false;tick(1.f/60);
stick={.5f,0};pos={500,500};for(int i=0;i<60;i++)tick(1.f/60);
assert(pos.x>600 && pos.x<611);stick={};tick(1.f/60);
g_walkOn=true;g_walkFor=&pos;g_walkTarget={pos.x+20,pos.y};g_walkLastPos=pos;
for(int i=0;i<60;i++)tick(1.f/60);assert(!g_walkOn && pos.x<=g_walkTarget.x+.001f);
keys[KEY_D]=true;float before=pos.x;tick(1.f);assert(pos.x-before<=22.001f);
keys[KEY_D]=false;tick(.016f);
// A slow device retains the same sustained walking pace as a 60 fps device.
pos={500,500};keys[KEY_D]=true;for(int i=0;i<10;i++)tick(.1f);
assert(pos.x-500>219 && pos.x-500<=220.01f);
keys[KEY_D]=false;tick(.016f);
// Camera-relative Den direction never rotates world-space tap destinations.
pos={500,500};keys[KEY_D]=true;UpdatePlayerMovement(pos,face,.1f,2000,-.694738f);
assert(pos.x>500 && pos.y<500);
keys[KEY_D]=false;tick(.016f);
g_walkOn=true;g_walkFor=&pos;g_walkTarget={pos.x+100,pos.y};g_walkLastPos=pos;
float tapY=pos.y;UpdatePlayerMovement(pos,face,.1f,2000,-.694738f);
assert(pos.y==tapY);WalkTargetClear();tick(.016f);
// Orbiting through a full turn preserves screen-relative stick directions.
for(float yaw : {0.f,.7f,1.5707963f,3.1415927f,4.712389f,6.1f}) {
    for(Vector2 input : {Vector2{0,-1},Vector2{1,0},Vector2{0,1},Vector2{-1,0},Vector2{.3f,-.4f}}) {
        stick={};tick(.016f);pos={500,500};stick=input;
        for(int i=0;i<60;i++){now+=1.f/60;UpdatePlayerMovement(pos,face,1.f/60,2000,-yaw);}
        Vector2 delta{pos.x-500,pos.y-500};
        float screenRight=delta.x*cosf(yaw)-delta.y*sinf(yaw);
        float screenDown=delta.x*sinf(yaw)+delta.y*cosf(yaw);
        float scale=(screenRight*input.x+screenDown*input.y)/(input.x*input.x+input.y*input.y);
        assert(scale>210 && scale<=220.01f);
        assert(std::abs(screenRight-scale*input.x)<.03f);
        assert(std::abs(screenDown-scale*input.y)<.03f);
    }
}
stick={};tick(.016f);
// Switching zones starts fresh, so velocity cannot carry between position owners.
Vector2 town{600,600};keys[KEY_D]=true;
UpdatePlayerMovement(town,face,.016f);assert(town.x-600<1.0f);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'movement.cpp'; exe=Path(tmp)/'movement'
    cpp.write_text(code)
    subprocess.run(['g++','-std=c++17','-I.',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS production movement: keyboard, diagonal, analog stick, tap arrival, frame stalls and zone changes')
