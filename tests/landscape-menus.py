from pathlib import Path
import re, os, tempfile, subprocess, shutil
run_dir=Path(tempfile.mkdtemp(prefix='tf-menu-test-'))
raylib=Path(os.environ.get('RAYLIB_DIR','/tmp/tf-raylib'))
if (raylib/'src'/'raylib.h').exists():raylib=raylib/'src'
h=(raylib/'raylib.h').read_text()
special={
 'GetMousePosition':'return testMouse;', 'IsMouseButtonPressed':'return testPressed;', 'IsMouseButtonDown':'return testPressed;',
 'GetFrameTime':'return 1.f/60;', 'GetTime':'return 10;', 'GetRandomValue':'return min;',
 'GetFontDefault':'Font f{};f.baseSize=16;return f;',
 'MeasureTextEx':'return {(float)strlen(text)*fontSize*.55f,fontSize};',
 'MeasureText':'return (int)(strlen(text)*fontSize*.55f);',
 'TextFormat':'static char buffers[16][4096];static int n=0;char* b=buffers[(n++)%16];va_list args;va_start(args,text);vsnprintf(b,4096,text,args);va_end(args);return b;',
 'Fade':'color.a=(unsigned char)(color.a*alpha);return color;',
 'CheckCollisionPointRec':'return point.x>=rec.x && point.x<=rec.x+rec.width && point.y>=rec.y && point.y<=rec.y+rec.height;',
 'DrawTextEx':'testTexts.push_back({text,position,fontSize});',
 'ColorBrightness':'return color;',
 'LoadRenderTexture':'RenderTexture2D t{};t.id=1;t.texture.id=1;t.texture.width=width;t.texture.height=height;return t;'
}
stubs='#include "raylib.h"\n#include <cstring>\n#include <cstdio>\n#include <cstdarg>\n#include <string>\n#include <vector>\nstruct Label{std::string text;Vector2 pos;float size;};std::vector<Label> testTexts;Vector2 testMouse{};bool testPressed=false;\n'
for ret,name,args in re.findall(r'RLAPI[ \t]+([^\n;]+?)\b(\w+)\(([^;]*?)\);',h):
 ret=ret.strip();stubs+=f'{ret} {name}({args}) {{'+special.get(name, '' if ret=='void' else 'return {};')+'}\n'
stubs+='extern "C" {void rlViewport(int,int,int,int){}void rlLoadIdentity(){}void rlDrawRenderBatchActive(){}void rlMatrixMode(int){}void rlOrtho(double,double,double,double,double,double){}void rlFrustum(double,double,double,double,double,double){}void rlMultMatrixf(const float*){}void rlPushMatrix(){}void rlPopMatrix(){}void rlTranslatef(float,float,float){}void rlRotatef(float,float,float,float){}void rlScalef(float,float,float){}}\n'
stubs+='\n#define CGLTF_IMPLEMENTATION\n#include \"external/cgltf.h\"\nextern \"C\" {void rlEnableFramebuffer(unsigned int){}unsigned int rlGetTextureIdDefault(){return 0;}void rlDisableBackfaceCulling(){}void rlEnableBackfaceCulling(){}void rlBegin(int){}void rlEnd(){}void rlSetTexture(unsigned int){}void rlNormal3f(float,float,float){}void rlTexCoord2f(float,float){}void rlVertex3f(float,float,float){}void rlColor4ub(unsigned char,unsigned char,unsigned char,unsigned char){}}\n'
(run_dir/'stubs.cpp').write_text(stubs)
root=Path(__file__).resolve().parents[1]
test='''#include <cassert>
#include <iostream>
void JS_CloudOpen(){}void CleanupAndClose(){}
#define main game_main
#include "'''+str(root/'main.cpp')+'''"
#undef main
extern bool testPressed;extern Vector2 testMouse;
static void reset(){g_uoTexReady=true;g_landscapePage=true;g_presentedPage=true;g_presentedWorld=false;g_presentedNativeMenu=false;g_landscapeNativeMenu=false;g_landscapeNativeMenuDrawing=false;g_landscapeActive=true;g_uiShieldOn=false;g_uiClips.clear();g_uiRects.clear();g_uiClickTaken=false;testPressed=false;g_landscapePointerHeld=false;}
static void check(const char* name){assert(g_uiClips.empty());for(auto r:g_uiRects){if(r.x<0 || r.y<0 || r.x+r.width>961 || r.y+r.height>541){std::cerr<<name<<" out of bounds "<<r.x<<","<<r.y<<","<<r.width<<","<<r.height<<"\\n";abort();}}std::cout<<name<<" controls="<<g_uiRects.size()<<"\\n";}
int main(){g_assets.itemIconOk.resize(kItemIconManifest.size());g_assets.itemIconTex.resize(kItemIconManifest.size());g_assets.paperdollOk.resize(kPaperdollManifest.size());g_assets.paperdollTex.resize(kPaperdollManifest.size());GameState s;s.gold=2000;s.magery=s.necromancy=s.chivalry=99;for(auto& v:s.buildingSkill)v=99;
for(int i=0;i<30;i++){Item it{};it.id=900+i;it.name="Test equipment "+std::to_string(i);it.type=ItemType::Armor;it.slot="helmet";it.power=3;s.backpack.push_back(it);if(i<10)s.bankItems.push_back(it);}
Pet pet{};pet.name="Test companion";s.pets.push_back(pet);
s.housePlotIdx=0;s.settle[kSbHall].level=3;
reset();bool open=false;
s.screen=Screen::Character;DrawLandscapePageHeader(s,open);check("header");
reset();testPressed=true;testMouse={360,88};DrawLandscapePageHeader(s,open);assert(g_characterPack);
reset();s.screen=Screen::Bank;DrawLandscapePageHeader(s,open);check("bank header");
reset();testPressed=true;testMouse={860,42};DrawLandscapePageHeader(s,open);assert(s.screen==g_playScreen);
s.screen=Screen::Character;
reset();DrawCharacterScreen(s,960,540);check("gear");
reset();g_characterPack=true;DrawCharacterScreen(s,960,540);check("backpack");
reset();DrawSkillsScreen(s,960,540);check("skills");
reset();DrawMagicScreen(s,960,540);check("magic");
reset();s.hotbarPickerSlot=0;DrawHotbarPicker(s,960,540,false);check("spell picker");s.hotbarPickerSlot.reset();
reset();DrawPetsScreen(s,960,540);check("pets");
reset();DrawBankScreen(s,960,540);check("bank");
for(int tab=0;tab<4;tab++){reset();g_questTab=tab;DrawQuestBoard(s,960,540);check("journal");}
reset();DrawOptions(s,960,540);check("settings");
for(int i=0;i<20;i++){GameState::TreasureMap m{};m.tier=i%5;m.decoded=i%2;s.tmaps.push_back(m);}
reset();DrawTreasureMaps(s,960,540);check("treasure maps");
reset();g_questOpen=g_optOpen=g_warOpen=g_tmapOpen=g_settleOpen=false;DrawHouseScreen(s,960,540);check("home");
reset();g_settleOpen=true;MenuGoScreen(s,Screen::House);assert(!g_settleOpen);DrawSettlement(s,960,540);check("settlement");
for(int tab=0;tab<8;tab++){reset();g_guildTab=tab;DrawWarWeek(s,960,540);check("guild");}
for(int book=0;book<4;book++)for(int mode=0;mode<6;mode++){reset();s.craftBuildingTab=book;s.craftModeTab=mode;DrawCraftScreen(s,960,540);check("craft");}
for(int tab=0;tab<3;tab++){reset();s.provisionerTab=tab;DrawProvisionerScreen(s,960,540);check("provisioner");}
reset();DrawFurTraderScreen(s,960,540);check("fur trader");
reset();DrawMinersGuildScreen(s,960,540);check("miners guild");
reset();DrawRefugeScreen(s,960,540);check("refuge");
for(int page=0;page<kGuidePageCount;page++){reset();s.guidePage=page;DrawGuideScreen(s,960);check("help");}
}
'''
(run_dir/'menu.cpp').write_text(test)

try:
    exe=run_dir/'menus'
    subprocess.run(['g++','-std=c++17','-O0','-ffunction-sections','-fdata-sections','-I'+str(raylib),str(run_dir/'menu.cpp'),str(run_dir/'stubs.cpp'),'-Wl,--gc-sections','-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True)
    print('PASS production landscape pages: full inventories, pets, all craft modes, journal tabs, guild tabs, vendors, help, native navigation and visible control bounds')
finally:shutil.rmtree(run_dir)
