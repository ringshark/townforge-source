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
struct RenderTexture2D {int id;};
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
frame({480,195},true,true,false);p=LandscapeMouse();assert(near(p.x,270)&&near(p.y,146.25));assert(LandscapeUIAllowed()); // modal uses one centered mapping
frame({480,195},false,false,true);LandscapeMouse();
g_presentedDialog=false;g_presentedMap=true;
g_presentedMapSource={388,152,148,148};g_presentedMapDest={796,92,148,148};
frame({870,166},true,true,false);p=LandscapeMouse();assert(near(p.x,462)&&near(p.y,226));assert(LandscapeUIAllowed());
frame({750,166},false,true,false);p=LandscapeMouse();assert(near(p.x,342)&&near(p.y,226)); // map gesture stays captured outside widget
frame({750,166},false,false,true);LandscapeMouse();
g_presentedMap=false;
frame({870,166},true,true,false);p=LandscapeMouse();assert(near(p.x,489.375)&&near(p.y,276.6667));assert(!LandscapeUIAllowed()); // transition removes the map hit area
frame({870,166},false,false,true);LandscapeMouse();
g_presentedGear=true;g_presentedWorld=false;
frame({244,285},true,true,false);p=LandscapeMouse();assert(near(p.x,270)&&near(p.y,310.045));
frame({600,285},false,true,false);p=LandscapeMouse();assert(near(p.x,706.909)); // paperdoll drag retains its column
frame({600,285},false,false,true);LandscapeMouse();
frame({716,285},true,true,false);p=LandscapeMouse();assert(near(p.x,270)&&near(p.y,762.045));assert(LandscapeUIAllowed());
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'pointer.cpp';exe=Path(tmp)/'pointer';cpp.write_text(code)
    subprocess.run(['g++','-std=c++17',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS production landscape pointer capture, fresh gestures, and ground routing')

# Native page controls must change the offset before content is rendered.
panel=r"""#include <cassert>
#include <cmath>
struct Vector2{float x,y;};struct Rectangle{float x,y,width,height;};
enum{MOUSE_BUTTON_LEFT};Vector2 raw{};bool pressed=false,down=false;float wheel=0,g_landscapeScroll=0;
Vector2 GetMousePosition(){return raw;}
bool IsMouseButtonPressed(int){return pressed;}bool IsMouseButtonDown(int){return down;}
float GetMouseWheelMove(){return wheel;}
bool CheckCollisionPointRec(Vector2 p,Rectangle r){return p.x>=r.x && p.x<=r.x+r.width && p.y>=r.y && p.y<=r.y+r.height;}
"""+f'#include "{rules}"\n'+section('static Rectangle LandscapePanelTop()', 'static void LandscapeBeginFrame(')
panel+=r"""int main(){
auto near=[](float a,float b){return std::fabs(a-b)<.01;};
raw={900,488};pressed=down=true;LandscapePanelInput();assert(near(g_landscapeScroll,tflayout::maxScroll));
pressed=down=false;LandscapePanelInput();
raw={900,52};down=true; // touch-down fallback when the one-frame press was missed
LandscapePanelInput();assert(g_landscapeScroll==0);
down=false;LandscapePanelInput();
raw={900,268};pressed=down=true;LandscapePanelInput();assert(near(g_landscapeScroll,tflayout::maxScroll*.5f));
pressed=false;raw.y=430;LandscapePanelInput();assert(g_landscapeScroll>tflayout::maxScroll*.9f);
down=false;LandscapePanelInput();
raw={900,300};wheel=1;float before=g_landscapeScroll;LandscapePanelInput();assert(near(g_landscapeScroll,before-48));
assert(tflayout::Map({900,488},tflayout::Region::PanelChrome).x<0);
} """
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'panel.cpp';exe=Path(tmp)/'panel';cpp.write_text(panel)
    subprocess.run(['g++','-std=c++17',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS production Top/Bottom, missed-press touch fallback, rail drag, wheel and sidebar isolation')

# Exercise the production modal selector rather than inferring dialogs from
# control dimensions (which missed menus and mistook broad HUD bars for panels).
main=Path('main.cpp').read_text()
start=main.index('static bool LandscapeDialogOpen(')
selector=main[start:main.index('static void WarmWildernessCache(',start)]
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

# Production nested render-target restoration: drawing a complete minimap must
# clear only its own target and return subsequent HUD drawing to the HUD target.
widget = """#include <vector>
#include <map>
#include <cassert>
struct Vector2{float x,y;};struct Rectangle{float x,y,width,height;};
struct Texture{int id,width,height;};struct RenderTexture2D{int id;Texture texture;};
int bound=0,cleared=0;
void BeginTextureMode(RenderTexture2D t){bound=t.id;}void EndTextureMode(){bound=0;}
void ClearBackground(int){cleared=bound;}void SetTextureFilter(Texture,int){}
RenderTexture2D LoadRenderTexture(int w,int h){return {2,{2,w,h}};}
void rlMatrixMode(int){}void rlLoadIdentity(){}void rlOrtho(float,float,float,float,int,int){}
enum{BLANK, TEXTURE_FILTER_BILINEAR, RL_PROJECTION, RL_MODELVIEW};
bool g_landscapeActive=true,g_landscapeWorld=true,g_landscapeDialog=false,g_landscapeMap=false;
Rectangle g_landscapeMapSource{},g_landscapeMapDest{};RenderTexture2D g_landscapeMapTarget{};
std::vector<RenderTexture2D> g_landscapeTargets;std::map<unsigned,Vector2> g_landscapeSceneSizes;
"""+section('static void LandscapeTextureBegin(', 'static void Landscape3DBegin(')+"""
int main(){
RenderTexture2D hud{1,{1,540,900}};g_landscapeTargets.push_back(hud);bound=1;
{LandscapeMapWidget map({388,152,148,148});assert(bound==2 && cleared==2);assert(g_landscapeTargets.size()==2);}
assert(bound==1 && g_landscapeTargets.size()==1 && g_landscapeMap);
assert(g_landscapeMapDest.x==796 && g_landscapeMapDest.width==148 && g_landscapeMapDest.height==148);
g_landscapeDialog=true;{LandscapeMapWidget map({388,152,148,148});assert(!map.active && bound==1);}
assert(bound==1 && cleared==2);
}
"""
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'widget.cpp';exe=Path(tmp)/'widget';cpp.write_text(widget)
    subprocess.run(['g++','-std=c++17',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS complete minimap target isolation and HUD restoration')
