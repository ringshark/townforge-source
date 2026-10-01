"""Exercise production pointer capture across HUD boundaries without a GPU."""
from pathlib import Path
import subprocess,tempfile
s=Path('landscape_layout.h').read_text()
def section(a,b):
    start=s.index(a);return s[start:s.index(b,start)]
rules=Path('landscape_layout_rules.h').resolve()
code='''#include <vector>
#include <cassert>
#include <cmath>
struct Vector2 {float x,y;};struct Rectangle {float x,y,width,height;};
enum {MOUSE_BUTTON_LEFT};
Vector2 raw{};bool pressed=false,down=false,released=false;double clockTime=0;
Vector2 GetMousePosition(){return raw;}double GetTime(){return clockTime;}
bool IsMouseButtonPressed(int){return pressed;}bool IsMouseButtonDown(int){return down;}bool IsMouseButtonReleased(int){return released;}
bool CheckCollisionPointRec(Vector2 p,Rectangle r){return p.x>=r.x && p.x<=r.x+r.width && p.y>=r.y && p.y<=r.y+r.height;}
bool g_presentedWorld=true,g_presentedDialog=false;float g_landscapeScroll=0;
'''+f'#include "{rules}"\n'
code+=section('static std::vector<Rectangle> g_landscapeHitRects;', 'static RenderTexture2D LandscapeLoadScene(')
code+=section('static Vector2 LandscapeMouse()', 'static Vector2 LandscapeTouch(')
code+='''
void frame(Vector2 p,bool press,bool hold,bool release){raw=p;pressed=press;down=hold;released=release;clockTime+=.016;}
int main(){
auto near=[](float a,float b){return std::fabs(a-b)<.01;};
g_landscapeHitRects={{190,768,154,44},{12,58,110,44}};
frame({85,455},true,true,false);auto p=LandscapeMouse();assert(near(p.x,85)&&near(p.y,815));
frame({195,455},false,true,false);p=LandscapeMouse();assert(near(p.x,195)&&near(p.y,815)); // no jump when exiting stick region
frame({700,420},false,false,true);p=LandscapeMouse();assert(near(p.x,700)&&near(p.y,780));
frame({700,420},true,true,false);p=LandscapeMouse();assert(near(p.x,280)&&near(p.y,780)); // new press immediately after release gets fresh routing
frame({400,420},false,true,false);p=LandscapeMouse();assert(near(p.x,-20)&&near(p.y,780)); // spell gesture stays in its own coordinates
frame({400,420},false,false,true);LandscapeMouse();
frame({400,420},true,true,false);p=LandscapeMouse();assert(near(p.x,225)&&near(p.y,700));assert(!LandscapeUIAllowed());
frame({900,260},false,false,true);LandscapeMouse();
frame({900,260},true,true,false);p=LandscapeMouse();assert(near(p.x,506.25)&&near(p.y,433.3333));assert(!LandscapeUIAllowed()); // empty HUD space remains walkable
frame({900,260},false,false,true);LandscapeMouse();
g_landscapeHitRects.push_back({200,300,100,50});
frame({400,195},true,true,false);p=LandscapeMouse();assert(near(p.x,225)&&near(p.y,325));assert(LandscapeUIAllowed()); // visible middle-strip control
frame({400,195},false,false,true);LandscapeMouse();
g_presentedDialog=true;
frame({480,195},true,true,false);p=LandscapeMouse();assert(near(p.x,270)&&near(p.y,195));assert(LandscapeUIAllowed()); // modal uses one centered mapping
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'pointer.cpp';exe=Path(tmp)/'pointer';cpp.write_text(code)
    subprocess.run(['g++','-std=c++17',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS production landscape pointer capture, fresh gestures, and ground routing')

# Exercise the production modal selector rather than inferring dialogs from
# control dimensions (which missed menus and mistook broad HUD bars for panels).
main=Path('main.cpp').read_text()
start=main.index('static bool LandscapeDialogOpen(')
selector=main[start:main.index('static void UpdateDrawFrame()',start)]
modal='''#include <optional>
#include <cassert>
enum class Screen {Town,Blackwake,Character};
bool IsMenuScreen(Screen s){return s==Screen::Character;}
bool g_trackOpen=false;int g_denPanel=0;
struct GameState {
 Screen screen=Screen::Town;
 bool exploreMenuOpen=false,dungeonMenuOpen=false,worldMapOpen=false,guideOpen=false;
 bool interiorGreeted=false,journalOpen=false,recallPickerOpen=false,houseDesignerOpen=false,houseChestOpen=false;
 std::optional<int> selectedTile,greetedNPC,hotbarPickerSlot;
 int houseCraftModule=-1,openCorpseId=-1;
};
'''+selector+'''int main(){
GameState s;assert(!LandscapeDialogOpen(s));
'''
for flag in ['exploreMenuOpen','dungeonMenuOpen','worldMapOpen','guideOpen','interiorGreeted','journalOpen','recallPickerOpen','houseDesignerOpen','houseChestOpen']:
    modal+=f's=GameState{{}};s.{flag}=true;assert(LandscapeDialogOpen(s));\n'
for flag in ['selectedTile','greetedNPC','hotbarPickerSlot','houseCraftModule','openCorpseId']:
    modal+=f's=GameState{{}};s.{flag}=0;assert(LandscapeDialogOpen(s));\n'
modal+='''s=GameState{};s.screen=Screen::Character;assert(LandscapeDialogOpen(s));
s=GameState{};g_trackOpen=true;assert(LandscapeDialogOpen(s));g_trackOpen=false;
s.screen=Screen::Blackwake;g_denPanel=1;assert(LandscapeDialogOpen(s));
g_denPanel=0;assert(!LandscapeDialogOpen(s));
}'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'dialogs.cpp';exe=Path(tmp)/'dialogs';cpp.write_text(modal)
    subprocess.run(['g++','-std=c++17',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS production landscape menu and popup selection')
