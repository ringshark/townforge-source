"""Exercise production click ownership, clipping, and press/release routing."""
from pathlib import Path
import subprocess, tempfile
s=Path('main.cpp').read_text()
def section(a,b):
    start=s.index(a); return s[start:s.index(b,start)]
code='''#include <vector>
#include <algorithm>
#include <cmath>
#include <string>
#include <cassert>
struct Vector2 {float x,y;}; struct Rectangle {float x,y,width,height;};
enum {MOUSE_BUTTON_LEFT};
Vector2 mouse{}; bool pressed=false,down=false,released=false;
Vector2 GetMousePosition(){return mouse;}
bool IsMouseButtonPressed(int){return pressed;} bool IsMouseButtonDown(int){return down;}
bool IsMouseButtonReleased(int){return released;}
bool CheckCollisionPointRec(Vector2 p,Rectangle r){return p.x>=r.x && p.x<=r.x+r.width && p.y>=r.y && p.y<=r.y+r.height;}
void BeginScissorMode(int,int,int,int){} void EndScissorMode(){}
float Dist(Vector2 a,Vector2 b){return std::hypot(a.x-b.x,a.y-b.y);}
int prompts=0; void PromptTextInto(const char*,std::string&,size_t){prompts++;}
'''
code+=section('static Rectangle g_uiShield =','static bool Button(')
code+=section('static void TapToEditText(', 'static void UpdateTextInput(')
code+='constexpr Rectangle kViewport={0,110,540,790};\n'
code+=section('static Rectangle TargetButtonRect()', '// ---------------------------------------------------------------------')
code+='''void frame(Vector2 p,bool press,bool hold,bool release){mouse=p;pressed=press;down=hold;released=release;UIFrameReset();}
int main(){
Rectangle target=TargetButtonRect();assert(target.y+target.height<748);assert(target.x>=0 && target.x+target.width<=540);
Rectangle play{130,56,92,40},chat{134,58,132,36};
frame({170,75},true,true,false);assert(UIClick(play));assert(!UIClick(chat));
frame({400,500},false,false,true);assert(UIHit(mouse)); // release cannot pick the world after navigation
frame({400,500},false,false,false);assert(!UIHit(mouse));
frame({170,75},true,true,false);assert(UIClick(chat)); // deliberate subsequent chat tap still works
frame({0,0},false,false,false);
UIBeginScissorMode(0,100,100,100);
frame({20,80},true,true,false); // reset clears draw clips; apply the current list clip
UIBeginScissorMode(0,100,100,100);
assert(!UIClick({0,50,100,40}));assert(!UIClick({0,90,100,30}));
mouse={20,110};assert(UIClick({0,90,100,30}));
UIBeginScissorMode(0,120,100,40);assert(!UIContains({20,110},{0,100,100,100}));
UIEndScissorMode();assert(UIContains({20,110},{0,100,100,100}));UIEndScissorMode();
frame({10,10},true,true,false);g_uiShieldOn=true;g_uiShield={0,0,100,100};
assert(UIHit(mouse));assert(!UIClick({0,0,20,20}));g_uiShieldBypass=true;assert(UIClick({0,0,20,20}));
g_uiShieldOn=g_uiShieldBypass=false;
frame({10,10},true,true,false);assert(!UIClick({0,0,20,20},false));
frame({400,500},false,false,true);assert(UIHit(mouse)); // disabled controls own the gesture too
std::string name;Rectangle field{0,0,100,30};
frame({10,10},true,true,false);TapToEditText(field,"Name",name,24);
frame({11,11},false,false,true);TapToEditText(field,"Name",name,24);assert(prompts==1);
frame({10,10},true,true,false);TapToEditText(field,"Name",name,24);
frame({70,10},false,false,true);TapToEditText(field,"Name",name,24);assert(prompts==1); // drag is not a text tap
frame({10,10},true,true,false);assert(UIClick({0,0,100,30}));TapToEditText(field,"Name",name,24);
frame({10,10},false,false,true);TapToEditText(field,"Name",name,24);assert(prompts==1); // navigation never opens a new text field
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'ui.cpp';exe=Path(tmp)/'ui';cpp.write_text(code)
    subprocess.run(['g++','-std=c++17',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS production UI routing: one click per press, release ownership, clipping, nested clips, shields, disabled controls and text-field taps')
