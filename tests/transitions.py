"""Verify retained transition frames and one-time idle cache preparation without a GPU."""
from pathlib import Path
import subprocess,tempfile
main=Path('main.cpp').read_text();layout=Path('landscape_layout.h').read_text()
def extract(text,start,end):
    i=text.index(start);return text[i:text.index(end,i)]
code='''#include <vector>
#include <cassert>
struct Vector2{float x,y;};struct Rectangle{float x,y,width,height;};
struct Color{int r,g,b,a;};const Color WHITE{255,255,255,255};
struct Texture{int id;};struct RenderTexture2D{Texture texture;};
bool g_landscapeActive=true,g_landscapeFrameReady=true;
RenderTexture2D g_landscapeFrame{{7}};std::vector<RenderTexture2D> g_landscapeTargets(1);
int begun=0,drawn=0,ended=0,cleared=0;
void BeginDrawing(){begun++;}void EndTextureMode(){ended++;}
void EndScissorMode(){}
void DrawTexturePro(Texture t,Rectangle s,Rectangle d,Vector2,float,Color){assert(t.id==7 && s.height==-540 && d.width==960);drawn++;}
void ClearBackground(Color){cleared++;}void DrawText(const char*,int,int,int,Color){}
'''+extract(layout,'static void LandscapeDrawLastFrame()', 'static std::set<unsigned>')
code+='''enum class Screen{Town,Wilderness};struct GameState{Screen screen=Screen::Town;};
bool dialog=false,down=false,g_walkOn=false;double now=0;
enum{MOUSE_BUTTON_LEFT,KEY_W,KEY_A,KEY_S,KEY_D,KEY_UP,KEY_DOWN,KEY_LEFT,KEY_RIGHT};
bool LandscapeDialogOpen(const GameState&){return dialog;}bool IsMouseButtonDown(int){return down;}
bool IsKeyDown(int){return false;}double GetTime(){return now;}
int loads[4]={};void Wild3DLoadModels(){loads[0]++;}void Wild3DEnsureGround(){loads[1]++;}
void Wild3DBuildScatter(){loads[2]++;}void WildMapEnsureTexture(){loads[3]++;}
'''+extract(main,'static void WarmWildernessCache(', 'static void UpdateDrawFrame()')
code+='''int main(){
LandscapeHoldFrame();assert(!g_landscapeActive && ended==1 && begun==1 && drawn==1 && cleared==0 && g_landscapeTargets.empty());
g_landscapeFrameReady=false;LandscapeDrawLastFrame();assert(cleared==1); // first-launch fallback is explicit
GameState s;now=1;WarmWildernessCache(s);assert(loads[0]==0);
now=2;WarmWildernessCache(s);assert(loads[0]==1 && loads[1]==0);
down=true;now=3;WarmWildernessCache(s);assert(loads[1]==0);
down=false;now=4;WarmWildernessCache(s);assert(loads[1]==0);
now=5;WarmWildernessCache(s);assert(loads[1]==1);
g_walkOn=true;now=6;WarmWildernessCache(s);assert(loads[2]==0);
g_walkOn=false;now=8;WarmWildernessCache(s);assert(loads[2]==1);
s.screen=Screen::Wilderness;now=9;WarmWildernessCache(s);assert(loads[3]==0);
s.screen=Screen::Town;now=11;WarmWildernessCache(s);assert(loads[3]==1);
for(int i=0;i<10;i++){now++;WarmWildernessCache(s);}for(int n:loads)assert(n==1);
}
'''
with tempfile.TemporaryDirectory() as tmp:
    cpp=Path(tmp)/'transition.cpp';exe=Path(tmp)/'transition';cpp.write_text(code)
    subprocess.run(['g++','-std=c++17',str(cpp),'-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
# A navigation frame must advance input without presenting its partial render.
hold=main.index('if(state.screen!=frameScreen)')
assert main.index('LandscapeHoldFrame();EndDrawing();return;',hold)<main.index('LandscapePresent(UiFont()',hold)
assert 'Fade(BLACK, std::min(1.0f, g_screenFadeT' not in main
print('PASS retained transition frames, input advancement, and one-time idle caches')
