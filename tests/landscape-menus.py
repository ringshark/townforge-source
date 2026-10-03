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
reset();GameState noHome;DrawSettlement(noHome,960,540);check("settlement first visit");
reset();g_settleOpen=true;MenuGoScreen(s,Screen::House);assert(!g_settleOpen);DrawSettlement(s,960,540);check("settlement");
for(int tab=0;tab<8;tab++){reset();g_guildTab=tab;DrawWarWeek(s,960,540);check("guild");}
for(int book=0;book<4;book++)for(int mode=0;mode<6;mode++){reset();s.craftBuildingTab=book;s.craftModeTab=mode;DrawCraftScreen(s,960,540);check("craft");}
for(int tab=0;tab<3;tab++){reset();s.provisionerTab=tab;DrawProvisionerScreen(s,960,540);check("provisioner");}
reset();DrawFurTraderScreen(s,960,540);check("fur trader");
reset();DrawMinersGuildScreen(s,960,540);check("miners guild");
reset();DrawRefugeScreen(s,960,540);check("refuge");
for(int page=0;page<kGuidePageCount;page++){reset();s.guidePage=page;DrawGuideScreen(s,960);check("help");}
// Decoration is a native overlay: every control fits and taps use wide coordinates.
auto decorReset=[&](){reset();g_landscapePage=g_presentedPage=false;g_presentedNativeMenu=true;g_landscapeWorld=g_presentedWorld=true;g_presentedDialog=false;g_hdOn=true;g_hdGhost=g_hdMove=g_hdSel=-1;g_hdMsg.clear();};
decorReset();DrawHouseDecorateUI(s,540,900);check("decorate catalog");assert(g_landscapeNativeMenu && !g_landscapeNativeMenuDrawing);
decorReset();g_hdCat=0;testPressed=true;testMouse={584+16+2*(336.f/kHdcCount)+10,190};DrawHouseDecorateUI(s,540,900);assert(g_hdCat==2);check("decorate categories");
decorReset();g_hdGhost=0;g_hdRot=0;testPressed=true;testMouse={270,488};DrawHouseDecorateUI(s,540,900);assert(g_hdRot==90);check("decorate placement");
decorReset();g_hdGhost=0;testPressed=true;testMouse={658,488};DrawHouseDecorateUI(s,540,900);assert(g_hdGhost==-1 && g_hdMove==-1);
decorReset();testPressed=true;testMouse={937,161};DrawHouseDecorateUI(s,540,900);assert(!g_hdOn);
for(int i=0;i<kIsCount;i++)g_isReady[i]=true;
decorReset();s.houseDesignerOpen=true;s.houseLayout=HouseStarterLayout(kHousePlots[s.housePlotIdx].cells);DrawHouseDesigner(s,540,900);check("house designer");
decorReset();int style=s.houseWallStyle;testPressed=true;testMouse={666,215};DrawHouseDesigner(s,540,900);assert(s.houseWallStyle!=style);check("house styles");
decorReset();s.houseDesignerOpen=true;testPressed=true;testMouse={140,490};DrawHouseDesigner(s,540,900);assert(!s.houseDesignerOpen);
g_hdOn=false;g_presentedNativeMenu=g_landscapeNativeMenu=false;
// Invisible panels from other screens must not shrink a playable interior.
GameState room;room.screen=Screen::Town;room.greetedNPC=0;
EnterInterior(room,"stable");assert(!room.greetedNPC && !LandscapeDialogOpen(room));
room.journalOpen=room.recallPickerOpen=room.worldMapOpen=room.guideOpen=true;
room.dungeonMenuOpen=room.houseDesignerOpen=room.houseChestOpen=true;
room.hotbarPickerSlot=0;room.houseCraftModule=0;room.openCorpseId=42;g_trackOpen=true;
assert(!LandscapeDialogOpen(room));
room.selectedTile="stable";assert(LandscapeDialogOpen(room));
reset();DrawBuildingDetailPanel(room,540);assert(!g_uiRects.empty());
for(auto r:g_uiRects)assert(r.y>=170 && r.y+r.height<110+tflayout::panelBody); // actions and Close visible without scrolling
room.selectedTile.reset();
room.interiorGreeted=true;assert(LandscapeDialogOpen(room));room.interiorGreeted=false;
room.exploreMenuOpen=true;assert(LandscapeDialogOpen(room));room.exploreMenuOpen=false;
EnterInterior(room,"wildhouse");assert(!room.houseChestOpen && room.houseCraftModule<0);
room.houseChestOpen=true;assert(LandscapeDialogOpen(room));room.houseChestOpen=false;
room.houseCraftModule=0;assert(LandscapeDialogOpen(room));
room.housePlotIdx=0;room.selectedTile="wildhouse";room.interiorGreeted=true;
ExitInterior(room);assert(room.screen==Screen::Wilderness && !room.selectedTile && !room.interiorGreeted && !room.houseChestOpen && room.houseCraftModule<0);
room=GameState{};EnterInterior(room,"stable");room.interiorGreeted=true;room.selectedTile="stable";ExitInterior(room);
assert(room.screen==Screen::Town && !LandscapeDialogOpen(room));
g_trackOpen=false;
std::cout<<"PASS interior layout: stale panels ignored, real panels retained, town and homestead exits clear interior locks"<<std::endl;
// Opening progression uses actual gameplay events, not timeouts or arbitrary gold changes.
GameState o;int gold=o.gold,bandages=o.bandages;
UpdateStarterProgress(o);o.townPlayerPos.x+=300;UpdateStarterProgress(o);assert(o.starterStep==kStWalk);
o.starterAccepted=true;UpdateStarterProgress(o);
for(int i=0;i<6;i++){o.townPlayerPos.x+=40;UpdateStarterProgress(o);}assert(o.starterStep==kStGate);
o.screen=Screen::Wilderness;UpdateStarterProgress(o);assert(o.starterStep==kStGather);
UpdateStarterProgress(o);assert(o.starterStep==kStGather);o.wood++;UpdateStarterProgress(o);assert(o.starterStep==kStFight);
GameState::WorldCorpse corpse{};corpse.zone=1;corpse.spotIdx=0;corpse.name="Test bat";
AddWorldCorpse(o,corpse);UpdateStarterProgress(o);assert(o.starterStep==kStFight);o.worldCorpses.clear();
corpse.zone=0;corpse.loot.push_back({GameState::kClGold,2,std::nullopt});
AddWorldCorpse(o,corpse);UpdateStarterProgress(o);assert(o.starterStep==kStLoot);
o.gold+=100;UpdateStarterProgress(o);assert(o.starterStep==kStLoot);
o.worldCorpses.clear();UpdateStarterProgress(o);assert(o.starterStep==kStLoot);
Vector2 target;assert(StarterTarget(o,&target)); // missing/reloaded corpse gives a recovery target
AddWorldCorpse(o,corpse);assert(TakeCorpseLoot(o,o.worldCorpses.back(),0));UpdateStarterProgress(o);assert(o.starterStep==kStPack);
o.screen=Screen::Character;g_characterPack=false;UpdateStarterProgress(o);assert(o.starterStep==kStPack);
g_characterPack=true;UpdateStarterProgress(o);assert(o.starterStep==kStReturn);
SaveGame(o);GameState restored;assert(LoadGame(restored));assert(restored.starterStep==kStReturn && restored.starterAccepted && restored.starterLooted==1);
o.screen=Screen::Town;UpdateStarterProgress(o);assert(o.starterStep==kStDone && o.starterRewarded && o.gold==gold+152 && o.bandages==bandages+5);
int rewardGold=o.gold;StarterComplete(o);assert(o.gold==rewardGold);SaveGame(o);assert(LoadGame(restored));assert(restored.starterRewarded);
StarterBegin(o,kStWalk);o.screen=Screen::Town;UpdateStarterProgress(o);o.screen=Screen::Wilderness;UpdateStarterProgress(o);assert(o.starterWalked==0); // travel is not walking
// Fast loot, before the next render, still completes both combat and collecting.
StarterBegin(o,kStFight);o.starterLooted=0;AddWorldCorpse(o,corpse);assert(TakeCorpseLoot(o,o.worldCorpses.back(),0));UpdateStarterProgress(o);assert(o.starterStep==kStPack);
// A new card stays in native landscape space and owns its gesture.
reset();GameState fresh;fresh.screen=Screen::Town;g_landscapeWorld=true;g_landscapeDialog=false;g_landscapePage=false;g_presentedPage=false;
UpdateDrawStarter(fresh,540,900);assert(g_landscapeOpening.visible && g_landscapeOpening.actions[0]=="Begin expedition");
LandscapePresent(UiFont(),g_uiRects);assert(g_presentedOpening && g_presentedOpeningRect.x==176);
testPressed=true;testMouse={300,336};g_landscapePointerHeld=false;UIFrameReset();
assert(g_landscapePointerRegion==tflayout::Region::NativePage && UIHit(GetMousePosition()));
assert(!UIClick({200,300,200,60}));UpdateDrawStarter(fresh,540,900);assert(fresh.starterAccepted && fresh.starterStep==kStWalk);
reset();GameState field;field.starterAccepted=true;field.starterStep=kStGather;field.screen=Screen::Wilderness;
g_landscapeWorld=true;g_landscapeDialog=false;g_landscapePage=false;g_presentedPage=false;g_landscapeMap=true;
g_landscapeMapDest={796,92,148,148};g_landscapeMapSource={388,152,148,148};
UpdateDrawStarter(field,540,900);LandscapePresent(UiFont(),g_uiRects);assert(g_presentedMap && g_presentedOpening); // minimap must not hide the objective
std::cout<<"PASS opening: explicit start, movement, gathering, real kills/loot, missing-corpse recovery, backpack, return, saved progress, one-time reward and native input ownership"<<std::endl;

}
'''
(run_dir/'menu.cpp').write_text(test)

try:
    exe=run_dir/'menus'
    subprocess.run(['g++','-std=c++17','-O0','-ffunction-sections','-fdata-sections','-I'+str(raylib),str(run_dir/'menu.cpp'),str(run_dir/'stubs.cpp'),'-Wl,--gc-sections','-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True,cwd=run_dir)
    print('PASS production landscape pages: full inventories, pets, all craft modes, journal tabs, guild tabs, vendors, help, native navigation and visible control bounds')
finally:shutil.rmtree(run_dir)
