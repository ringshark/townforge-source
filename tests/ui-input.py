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
bool menuBlocked=false;bool LandscapeMenuBlocksWorld(){return menuBlocked;}
bool landscapeAllowed=true;bool g_landscapePage=false;
bool LandscapeUIAllowed(){return landscapeAllowed;}
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
landscapeAllowed=false;frame({200,800},true,true,false);
assert(!UIClick({180,760,160,80}));assert(!UIHit(mouse));
landscapeAllowed=true;frame({0,0},false,false,false);
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
menuBlocked=true;landscapeAllowed=false;frame({900,500},true,true,false);
assert(UIHit(mouse));assert(!UIClick({880,480,60,40}));
menuBlocked=false;landscapeAllowed=true;
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

release=code[:code.index('int main()')]+r'''int main(){
g_landscapePage=true;
Rectangle item{20,240,120,44};
frame({60,260},true,true,false);UIBeginScissorMode(0,200,960,320);assert(!UIClick(item));UIEndScissorMode();
frame({60,260},false,false,true);UIBeginScissorMode(0,200,960,320);assert(UIClick(item));assert(!UIClick(item));UIEndScissorMode();
frame({60,260},false,false,false);
frame({60,260},true,true,false);UIBeginScissorMode(0,200,960,320);assert(!UIClick(item));UIEndScissorMode();
frame({60,240},false,true,false);UIBeginScissorMode(0,200,960,320);assert(!UIClick(item));UIEndScissorMode();
frame({60,260},false,false,true);UIBeginScissorMode(0,200,960,320);assert(!UIClick(item));UIEndScissorMode();
frame({60,260},false,false,false);
frame({60,260},true,true,false);assert(UIClick(item)); // fixed navigation stays immediate
frame({60,260},false,false,false);
frame({60,260},true,true,false);UIBeginScissorMode(0,200,960,320);assert(UIClick(item,true,true));UIEndScissorMode(); // continuous sliders still begin on press
}'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'release.cpp';exe=Path(tmp)/'release';cpp.write_text(release)
    subprocess.run(['g++','-std=c++17',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS menu list actions on completed taps; swipes and repeated release callbacks do not activate')


# Exercise actual native menu callbacks using the production click router.
native=code[:code.index('void frame(')]
native+='''#include <optional>
struct Color{int r,g,b,a;};struct RenderTexture2D{int id=0,texture=0;};
constexpr int TEXTURE_FILTER_BILINEAR=1;
RenderTexture2D g_landscapeNativeMenuTarget;
bool g_landscapeNativeMenu=false,g_landscapeNativeMenuDrawing=false,g_landscapeDialog=false;
float g_landscapeScroll=0;
RenderTexture2D LoadRenderTexture(int,int){return {1,1};}
void SetTextureFilter(int,int){}void BeginTextureMode(RenderTexture2D){}void EndTextureMode(){}
void ClearBackground(Color){}void DrawRectangleGradientV(int,int,int,int,Color,Color){}
void DrawUIText(const char*,int,int,int,Color){}void LandscapePlate(Rectangle,Color){}
const char* TextFormat(const char*,...){return "resources";}
enum class SfxId{Click};void PlaySfx(SfxId){}
enum class Screen{Town,Blackwake,Character,Skills,Magic,Craft,Pets,Bank,House,Guide};
struct GameState{
 Screen screen=Screen::Town;std::optional<int> combat;bool playerIsGhost=false;
 float playerDeathAnimT=0,leaveDungT=-1;int gold=100,wood=0,ore=0,leather=0,backpackScroll=0,selectedTown=0,guidePage=0;Vector2 townPlayerPos{};
};
bool g_characterPack=false,g_questOpen=false,g_optOpen=false,g_settleOpen=false;int g_pdSel=-1,g_questTab=0,g_guildTab=0,g_denPanel=0;
Screen g_playScreen=Screen::Town;
bool DenFighting(){return false;}
void MenuGoScreen(GameState& s,Screen sc){s.screen=sc;g_questOpen=g_optOpen=false;}
void OpenWarWeek(GameState&){}std::string GuildAttentionLabel(){return "Guild";}
std::string JournalAttentionLabel(GameState&){return "Journal";}
Vector2 TS(float x,float y){return {x,y};}void WalkTargetClear(){}void TryStartLeaveDungeon(GameState& s){s.leaveDungT=1;}
'''+section('static bool LandscapeMenuCard(', 'static const char* LandscapePageTitle(')
native+='''int main(){
Screen pages[]={Screen::Character,Screen::Character,Screen::Skills,Screen::Magic,Screen::Craft,Screen::Pets,Screen::Bank,Screen::House,Screen::House,Screen::House,Screen::House,Screen::Guide};
for(int slot=0;slot<12;slot++){
 GameState s;bool open=true;mouse={28.f+(slot%4)*230.f+100,132.f+(slot/4)*86.f+30};pressed=down=true;UIFrameReset();g_uiShieldBypass=true;
 DrawLandscapeMainMenu(s,open,false);assert(!open && s.screen==pages[slot]);
 if(slot==1)assert(g_characterPack);if(slot==9)assert(g_questOpen);if(slot==10)assert(g_optOpen);
 for(auto r:g_uiRects)assert(r.x>=0 && r.y>=0 && r.x+r.width<=960 && r.y+r.height<=540 && r.height>=48);
 pressed=down=false;UIFrameReset();
}
GameState s;bool open=true;mouse={72,80};pressed=down=true;UIFrameReset();DrawLandscapeMainMenu(s,open,false);assert(!open && s.screen==Screen::Town);
pressed=down=false;UIFrameReset();open=true;s.screen=Screen::Character;mouse={100,462};pressed=down=true;UIFrameReset();DrawLandscapeMainMenu(s,open,false);assert(!open && s.screen==Screen::Town);
pressed=down=false;UIFrameReset();open=true;s.playerIsGhost=true;mouse={100,170};pressed=down=true;UIFrameReset();DrawLandscapeMainMenu(s,open,false);assert(open && s.screen==Screen::Town);
}'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'native-menu.cpp';exe=Path(tmp)/'native-menu';cpp.write_text(native)
    subprocess.run(['g++','-std=c++17',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
print('PASS native landscape menu: all twelve destinations, Close, Resume, disabled actions and visible touch targets')
