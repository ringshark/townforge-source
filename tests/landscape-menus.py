from pathlib import Path
import re, os, tempfile, subprocess, shutil
run_dir=Path(tempfile.mkdtemp(prefix='tf-menu-test-'))
raylib=Path(os.environ.get('RAYLIB_DIR','/tmp/tf-raylib'))
if (raylib/'src'/'raylib.h').exists():raylib=raylib/'src'
h=(raylib/'raylib.h').read_text()
special={
 'GetMousePosition':'return testMouse;', 'IsMouseButtonPressed':'return testPressed;', 'IsMouseButtonDown':'return testPressed;',
 'IsKeyDown':'return key==testMoveKey;',
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
stubs='#include "raylib.h"\n#include <cstring>\n#include <cstdio>\n#include <cstdarg>\n#include <string>\n#include <vector>\nstruct Label{std::string text;Vector2 pos;float size;};std::vector<Label> testTexts;Vector2 testMouse{};bool testPressed=false;int testMoveKey=-1;\n'
for ret,name,args in re.findall(r'RLAPI[ \t]+([^\n;]+?)\b(\w+)\(([^;]*?)\);',h):
 ret=ret.strip();stubs+=f'{ret} {name}({args}) {{'+special.get(name, '' if ret=='void' else 'return {};')+'}\n'
stubs+='extern "C" {void rlViewport(int,int,int,int){}void rlLoadIdentity(){}void rlDrawRenderBatchActive(){}void rlMatrixMode(int){}void rlOrtho(double,double,double,double,double,double){}void rlFrustum(double,double,double,double,double,double){}void rlMultMatrixf(const float*){}void rlPushMatrix(){}void rlPopMatrix(){}void rlTranslatef(float,float,float){}void rlRotatef(float,float,float,float){}void rlScalef(float,float,float){}}\n'
stubs+='\n#define CGLTF_IMPLEMENTATION\n#include \"external/cgltf.h\"\nextern \"C\" {void rlEnableShader(unsigned int){}unsigned int rlGetShaderIdDefault(){return 0;}void rlEnableFramebuffer(unsigned int){}unsigned int rlGetTextureIdDefault(){return 0;}void rlDisableBackfaceCulling(){}void rlEnableBackfaceCulling(){}void rlBegin(int){}void rlEnd(){}void rlSetTexture(unsigned int){}void rlNormal3f(float,float,float){}void rlTexCoord2f(float,float){}void rlVertex3f(float,float,float){}void rlColor4ub(unsigned char,unsigned char,unsigned char,unsigned char){}}\n'
stubs+='extern "C" {void rlDisableDepthMask(){}void rlEnableDepthMask(){}bool rlEnableVertexArray(unsigned int){return false;}unsigned int rlLoadVertexBuffer(const void*,int,bool){return 0;}void rlSetVertexAttribute(unsigned int,int,int,bool,int,int){}void rlEnableVertexAttribute(unsigned int){}void rlDisableVertexArray(){}Matrix rlGetMatrixProjection(){return {};}Matrix rlGetMatrixModelview(){return {};}}\n'
(run_dir/'stubs.cpp').write_text(stubs)
root=Path(__file__).resolve().parents[1]
test='''#include <cassert>
#include <iostream>
void JS_CloudOpen(){}
#define main game_main
#include "'''+str(root/'main.cpp')+'''"
#undef main
extern bool testPressed;extern int testMoveKey;extern Vector2 testMouse;
static void reset(){g_uoTexReady=true;g_landscapePage=true;g_presentedPage=true;g_presentedWorld=false;g_presentedNativeMenu=false;g_landscapeNativeMenu=false;g_landscapeNativeMenuDrawing=false;g_landscapeActive=true;g_uiShieldOn=false;g_uiClips.clear();g_uiRects.clear();g_uiClickTaken=false;testPressed=false;g_landscapePointerHeld=false;}
static void check(const char* name){assert(g_uiClips.empty());for(auto r:g_uiRects){if(r.x<0 || r.y<0 || r.x+r.width>961 || r.y+r.height>541){std::cerr<<name<<" out of bounds "<<r.x<<","<<r.y<<","<<r.width<<","<<r.height<<"\\n";abort();}}std::cout<<name<<" controls="<<g_uiRects.size()<<"\\n";}
int main(){g_assets.itemIconOk.resize(kItemIconManifest.size());g_assets.itemIconTex.resize(kItemIconManifest.size());g_assets.paperdollOk.resize(kPaperdollManifest.size());g_assets.paperdollTex.resize(kPaperdollManifest.size());GameState s;s.gold=2000;s.magery=s.necromancy=s.chivalry=99;for(auto& v:s.buildingSkill)v=99;
for(int i=0;i<30;i++){Item it{};it.id=900+i;it.name="Test equipment "+std::to_string(i);it.type=ItemType::Armor;it.slot="helmet";it.power=3;s.backpack.push_back(it);if(i<10)s.bankItems.push_back(it);}
Pet pet{};pet.name="Test companion";s.pets.push_back(pet);
s.housePlotIdx=0;s.settle[kSbHall].level=3;
// Dungeons must render as a native world, with the hero inside the visible view.
for(int di=0;di<(int)kDungeons.size();++di) {
 GameState dungeon;dungeon.screen=Screen::Hunt;dungeon.selectedDungeon=di;dungeon.hunt3DView=false;
 assert(NativeLandscapePage(dungeon));
 for(Vector2 pos:std::vector<Vector2>{{900,1300},{900,1000},{900,700},{700,700},{1100,700}}) {
  pos={pos.x*2,pos.y*2};if(!DungeonIsFloor(di,pos))continue;
  reset();g_landscapeWorld=true;g_presentedWorld=true;g_landscapeScroll=180;
  dungeon.dungeonPlayerPos=pos;DrawHuntScreen(dungeon,960,540);
  assert(kViewport.x==0 && kViewport.y==0 && kViewport.width==960 && kViewport.height==540);
  auto hero=WorldToScreen(dungeon.dungeonPlayerPos,CameraTopLeft(dungeon.dungeonPlayerPos,kDungeonWorldSize));
  assert(hero.x>=32 && hero.x<=928 && hero.y>=32 && hero.y<=508);
  if(pos.x>=480 && pos.x<=kDungeonWorldSize-480 && pos.y>=270 && pos.y<=kDungeonWorldSize-270)
   assert(std::abs(hero.x-480)<.01 && std::abs(hero.y-270)<.01);
  assert(LandscapeRegion({480,270})==tflayout::Region::NativePage);
  auto input=LandscapePointerMap({480,270},tflayout::Region::NativePage);
  assert(input.x==480 && input.y==270); // stale panel scroll cannot move the world or its picking
  check("dungeon world");
 }
 reset();dungeon.dungeonPlayerPos={1800,2600};dungeon.playerIsGhost=true;testMoveKey=KEY_S;
 const auto start=dungeon.dungeonPlayerPos;
 for(int frame=0;frame<20;++frame) {
  DrawHuntScreen(dungeon,960,540);
  auto hero=WorldToScreen(dungeon.dungeonPlayerPos,CameraTopLeft(dungeon.dungeonPlayerPos,kDungeonWorldSize));
  assert(std::abs(hero.x-480)<.01 && std::abs(hero.y-270)<.01);
 }
 testMoveKey=-1;assert(Dist(start,dungeon.dungeonPlayerPos)>10);dungeon.playerIsGhost=false;
 reset();dungeon.dungeonMenuOpen=true;DrawHuntScreen(dungeon,960,540);check("dungeon open menu");
 assert(g_landscapeNativeMenu);dungeon.dungeonMenuOpen=false;
 dungeon.hunt3DView=true;assert(!NativeLandscapePage(dungeon));
}
std::cout<<"PASS all six dungeon layouts: full landscape viewport, hero camera follow, identity input without panel scrolling, visible HUD and menu controls"<<std::endl;
std::fill(std::begin(g_isReady),std::end(g_isReady),true); // GPU surfaces are stubbed in this layout test
// Every 2D building uses the native world, with camera tracking rather than page scrolling.
for(const auto& room:kInteriorRooms) {
 GameState inside;inside.screen=Screen::Interior;inside.interiorKey=room.key;inside.interior3DView=false;
 assert(NativeLandscapePage(inside) && ExploreHeaderCollapsed(inside));
 for(Vector2 position:std::vector<Vector2>{{260,520},{260,100},{120,310},{400,310}}) {
  reset();g_landscapeWorld=g_presentedWorld=true;g_landscapeScroll=180;
  inside.interiorPlayerPos=position;DrawInteriorScreen(inside,960,540);
  assert(kViewport.x==0 && kViewport.y==0 && kViewport.width==960 && kViewport.height==540);
  auto pp=WorldToScreen(inside.interiorPlayerPos,InteriorCameraTopLeft(inside.interiorPlayerPos));
  assert(pp.x>0 && pp.x<960 && pp.y>0 && pp.y<540);
  auto input=LandscapePointerMap({pp.x,pp.y},tflayout::Region::NativePage);
  assert(input.x==pp.x && input.y==pp.y);check(room.key);
 }
 reset();g_landscapeWorld=true;inside.selectedTile=room.key;DrawInteriorScreen(inside,960,540);check("interior building actions");
 reset();inside.selectedTile.reset();inside.interiorGreeted=true;DrawInteriorScreen(inside,960,540);check("interior greeting");
 inside.interior3DView=true;assert(!NativeLandscapePage(inside));
}
std::cout<<"PASS all 13 interiors: native landscape, visible hero, identity input and building panels"<<std::endl;
// Physical planning uses existing building keys, doors and walkable terrain.
g_openWorld2D=true;
for(int town=0;town<4;++town){
 std::cout<<"town "<<town<<" depth="<<OpenTownDepth(town)<<" facing="<<TownGateFacing(town).x<<","<<TownGateFacing(town).y<<std::endl;
 // Visible front curtains must match the existing solid wall and spare the gate.
 auto walls=OpenTownWallSpans(town);assert(walls.size()==5);
 Vector2 gate=kTownGates[town].wildernessPos,facing=TownGateFacing(town);
 for(int side:{-1,1}){
  const auto& span=walls[side<0?3:4];
  auto a=GateLocal(span.a,gate,facing),b=GateLocal(span.b,gate,facing);
  assert(std::abs(a.y+6)<.01f && std::abs(b.y+6)<.01f);
  assert(std::abs(std::abs(side<0?a.x:b.x)-kOpenTownHalfW)<.01f);
  assert(std::abs(std::abs(side<0?b.x:a.x)-56)<.01f);
  Vector2 probe=GateWorld({side*(kOpenTownHalfW+56)*.5f,-6},gate,facing),moved=probe;
  assert(Dist(probe,tfdesign::NearestOn(probe,span.a,span.b))<.01f);
  GateCollide(moved,gate,facing);assert(Dist(probe,moved)>17);
 }
 Vector2 passage=GateWorld({0,-6},gate,facing),entered=passage;
 GateCollide(entered,gate,facing);assert(Dist(passage,entered)<.01f);
 // Flood the actual movement footprint from the gate, not just a drawn road.
 constexpr int N=140;const float step=8;auto center=GateWorld({0,-OpenTownDepth(town)*.5f},kTownGates[town].wildernessPos,TownGateFacing(town));Vector2 origin={center.x-560,center.y-560};
 auto point=[&](int i){return Vector2{origin.x+(i%N+.5f)*step,origin.y+(i/N+.5f)*step};};
 auto index=[&](Vector2 p){return std::clamp((int)((p.y-origin.y)/step),0,N-1)*N+std::clamp((int)((p.x-origin.x)/step),0,N-1);};
 auto walk=[&](Vector2 p){auto l=GateLocal(p,kTownGates[town].wildernessPos,TownGateFacing(town));if(OpenTownAt(p)!=town || fabsf(l.x)>kOpenTownHalfW-28 || l.y<-OpenTownDepth(town)+28)return false;for(const auto& n:ActiveTownNodes(town))if(Dist(p,OpenTownWorld(town,n.pos))<53)return false;return true;};
 std::vector<char> seen(N*N);std::vector<int> queue;int start=index(GateWorld({0,-35},kTownGates[town].wildernessPos,TownGateFacing(town)));queue.push_back(start);seen[start]=1;
 for(size_t head=0;head<queue.size();head++){int i=queue[head];for(int d:std::array<int,4>{{1,-1,N,-N}}){int j=i+d;if(j<0 || j>=N*N || std::abs(j%N-i%N)>1 || seen[j] || !walk(point(j)))continue;seen[j]=1;queue.push_back(j);}}
 for(const auto& node:ActiveTownNodes(town)){
  auto w=OpenTownWorld(town,node.pos);assert(OpenTownAt(w)==town);
  Vector2 approach={w.x,w.y+58};float access=1e9;for(int i:queue)access=std::min(access,Dist(point(i),approach));assert(access<24);
  auto local=OpenTownLocal(town,w);assert(Dist(local,node.pos)<.01f);
  for(const auto& other:ActiveTownNodes(town))if(node.key!=other.key)assert(Dist(w,OpenTownWorld(town,other.pos))>100);
 }
}
for(const auto& plot:kHousePlots)for(int dx:{-1,0,1})for(int dy:{-1,0,1})assert(OpenTownAt({plot.pos.x+dx*plot.cells*kHouseCellSize*.5f,plot.pos.y+dy*plot.cells*kHouseCellSize*.5f})<0);
for(int i=0;i<6;i++){auto door=EntranceDoorPoint(i),body=EntranceBodyCenter(i);assert(Dist({door.x,door.y+20},body)>=79);assert(Dist(EntranceStepOut(i),door)>60);}
tfdesign::BuildTrails();assert(!tfdesign::trails.empty());
for(const auto& path:tfdesign::trails)for(size_t i=1;i<path.size();++i)assert(tfdesign::ClearLine(path[i-1],path[i]));
std::cout<<"PASS all four towns: four visible wall sides, solid front curtains and open gate passage; all original shops inside their compound, spaced apart, coordinate round trips; "<<tfdesign::trails.size()<<" wilderness trails avoid water, ridges and owned plot footprints"<<std::endl;
// Expanded dungeons are connected, have distinct well-spaced real encounters,
// and keep a quiet entry/exit plus a fully revealed, nonblocking overview.
for(int di=0;di<6;di++){
 constexpr int N=90;std::vector<char> visited(N*N);std::vector<int> queue;
 auto p=[](int i){return Vector2{(i%N+.5f)*40,(i/N+.5f)*40};};int start=65*N+45;visited[start]=1;queue.push_back(start);
 for(size_t head=0;head<queue.size();head++){int i=queue[head];for(int d:std::array<int,4>{{1,-1,N,-N}}){int j=i+d;if(j<0 || j>=N*N || std::abs(j%N-i%N)>1 || visited[j] || !DungeonIsFloor(di,p(j)))continue;visited[j]=1;queue.push_back(j);}}
 for(int slot=0;slot<kDungeonSlotCount;slot++){
  auto at=DungeonMonsterNodePos(di,slot);assert(DungeonIsFloor(di,at));float access=1e9;
  for(int i:queue)access=std::min(access,Dist(p(i),at));assert(access<60);
  if(slot<kDungeonRegularSlots)assert(Dist(at,{1800,2600})>300);
  for(int other=0;other<slot;other++)assert(Dist(at,DungeonMonsterNodePos(di,other))>170);
 }
 GameState dungeon;dungeon.screen=Screen::Hunt;dungeon.selectedDungeon=di;dungeon.dungeonPlayerPos={1800,2600};reset();g_dungeonMapOpen=true;
 assert(LandscapeDialogOpen(dungeon)==17);DrawHuntScreen(dungeon,960,540);check("fully revealed dungeon map");assert(dungeon.dungeonPlayerPos.x==1800 && dungeon.dungeonPlayerPos.y==2600);g_dungeonMapOpen=false;
}
std::cout<<"PASS six expanded dungeons: 3600-unit worlds, 20 spaced encounters and boss, connected floors, quiet entry and fully revealed overview"<<std::endl;
for(int species=0;species<11;species++){
 Pet pet{};pet.id=900+species;pet.name=kWildCreatures[species].name;pet.role=kWildCreatures[species].role;pet.species=species;pet.hp=pet.maxHp=50;
 auto saved=PetFromLine(PetToLine(pet));assert(saved && saved->species==species);saved->name="Renamed companion";assert(PetSpecies(*saved)==species);
 auto legacy=PetToLine(pet);legacy=legacy.substr(0,legacy.rfind('|'));auto old=PetFromLine(legacy);assert(old && PetSpecies(*old)==species);
}
{std::vector<Color> image(25,WHITE);for(int y=1;y<4;y++)for(int x=1;x<4;x++)image[y*5+x]=BLACK;image[12]=WHITE;Image matte{image.data(),5,5,1,7};tfworldart::CleanMatte(matte,5,5);assert(image[0].a==0 && image[12].a==255 && image[6].a==255);}
{auto before=tfgfx::hero,up=tfgfx::north;tfgfx::hero.texture.id=991;tfgfx::hero.frames.assign(56,{0,0,100,139});tfgfx::north.texture.id=992;tfgfx::north.frames.assign(8,{0,0,200,290});
 GameState actor;actor.screen=Screen::Wilderness;actor.wildEngaged=GameState::ActiveMonster{};Item sword{};sword.category="Swordsmanship";actor.equipped.rightHand=sword;
 for(int face=0;face<4;face++){
  actor.wildEngaged->swingEffectTimer=.32f;assert(tfgfx::HeroAction(actor,{100,100},face,ActorAnim::Attack,34,WHITE));int first=tfgfx::lastHeroFrame;
  actor.wildEngaged->swingEffectTimer=.04f;assert(tfgfx::HeroAction(actor,{100,100},face,ActorAnim::Attack,34,WHITE));assert(tfgfx::lastHeroFrame!=first);assert(tfgfx::lastHeroNorth==(face==3));
  actor.wildEngaged->castEffectTimer=.04f;assert(tfgfx::HeroAction(actor,{100,100},face,ActorAnim::Cast,34,WHITE));assert(tfgfx::lastHeroFrame%8>=4 || (face==3 && tfgfx::lastHeroFrame>=4));
 }
 actor.playerDeathAnimT=1.2f;tfgfx::HeroAction(actor,{100,100},0,ActorAnim::Idle,34,WHITE);int falling=tfgfx::lastHeroFrame;actor.playerDeathAnimT=.01f;tfgfx::HeroAction(actor,{100,100},0,ActorAnim::Idle,34,WHITE);assert(tfgfx::lastHeroFrame>falling);
 tfgfx::hero=before;tfgfx::north=up;
}
std::cout<<"PASS graphics states: four-direction attack/cast/death phases, all 11 saved and renamed pet species, legacy pet rows and white matte cleanup"<<std::endl;
reset();bool open=false;
// Every native menu destination must preserve its action and fit the canvas.
auto menuReset=[&](){reset();g_landscapePage=g_presentedPage=false;g_presentedNativeMenu=true;g_uiShieldBypass=true;open=true;s.screen=Screen::Town;};
const Vector2 targets[]={{60,178},{60,250},{60,322},{60,394},{370,178},{370,250},{680,178},{680,250},{370,322},{680,322},{370,394}};
const Screen screens[]={Screen::Character,Screen::Character,Screen::Skills,Screen::Magic,Screen::Craft,Screen::Pets,Screen::House,Screen::Bank,Screen::House,Screen::House,Screen::Guide};
for(int i=0;i<11;i++){
 menuReset();testPressed=true;testMouse=targets[i];DrawLandscapeMainMenu(s,open,false);
 assert(!open && s.screen==screens[i]);check("main menu destination");
 if(i==0)assert(!g_characterPack);if(i==1)assert(g_characterPack);
 if(i==8)assert(g_questOpen);if(i==9)assert(g_optOpen);
}
menuReset();testPressed=true;testMouse={800,52};DrawLandscapeMainMenu(s,open,false);assert(!open && s.screen==Screen::Town);
menuReset();s.playerIsGhost=true;testPressed=true;testMouse=targets[0];DrawLandscapeMainMenu(s,open,false);assert(open && s.screen==Screen::Town);s.playerIsGhost=false;
menuReset();testPressed=true;testMouse={750,480};DrawLandscapeMainMenu(s,open,false);assert(!open && s.screen==Screen::Blackwake);
menuReset();s.screen=Screen::Blackwake;testPressed=true;testMouse={750,480};DrawLandscapeMainMenu(s,open,false);assert(!open && s.screen==Screen::Town && s.selectedTown==1);
menuReset();DrawLandscapeMainMenu(s,open,true);check("dungeon menu");
reset();open=false;
s.screen=Screen::Character;DrawLandscapePageHeader(s,open);check("header");
reset();testPressed=true;testMouse={360,88};DrawLandscapePageHeader(s,open);assert(g_characterPack);
reset();s.screen=Screen::Bank;DrawLandscapePageHeader(s,open);check("bank header");
reset();testPressed=true;testMouse={860,42};DrawLandscapePageHeader(s,open);assert(s.screen==g_playScreen);
s.screen=Screen::Character;
// New avatar renders through the same production path in every action state.
for(int state=0;state<10;state++) {
 HumanPose pose;pose.move=state==1 ? 1:0;pose.attackT=state==2 ? .5f:-1;
 pose.castT=state==3 ? .5f:-1;pose.hurtT=state==4 ? .5f:-1;
 pose.deathT=state==5 ? 1:-1;pose.gather=state>=7 ? state-6:0;
 assert(DrawEquippedHero(1,10,20,.7f,WHITE,s.equipped,pose,false,state==6,state==4));
 assert(DrawEquippedHero(1,10,20,.7f,WHITE,s.equipped,pose,true));
}
assert(kCompactHeroHeight==46.f && kCompactHeroHeight<66.f*.71f);
std::cout<<"PASS compact 3D avatar: idle, walk, attack, cast, hit, death, sneak, wood, ore, fish and shadow paths"<<std::endl;
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
o.screen=Screen::Wilderness;o.wildernessPlayerPos=TownWildernessSpawn(0);UpdateStarterProgress(o);assert(o.starterStep==kStGather);
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


// Retirement preserves saved progress and never charges upkeep for hidden systems.
{
GameState retired;retired.housePlotIdx=0;retired.gold=5000;retired.settleUpgrading=0;retired.settleUpgradeT=20;retired.settleRaidT=10;retired.settleEpoch=100;
retired.guildName="Existing guild";retired.guildRecruits.push_back({"Miran",0,10,true});retired.guildWarOn[0]=true;
SettleTick(retired,100);SettleCatchUp(retired);WarNetTick(retired,100);
assert(retired.settleUpgradeT==20 && retired.settleRaidT==10 && retired.settleEpoch==100 && retired.gold==5000);
assert(retired.guildRecruits[0].along && retired.guildWarOn[0] && !GuildWarActive(retired,0));
assert(FollowersUsed(retired)==0); // archived recruits never occupy pet slots
reset();g_warOpen=g_settleOpen=true;DrawHouseScreen(retired,960,540);assert(!g_warOpen && !g_settleOpen);check("homestead home");
for(int i=0;i<4;i++){int gold=retired.gold;BuildHouseModule(retired,i);assert(retired.houseModuleLevel[i]==1 && retired.gold==gold-150);}
int gold=retired.gold;BuildHouseModule(retired,0);assert(retired.gold==gold);
retired.settle[kSbForge].level=6;assert(HomeWingLevel(retired,0)==3);
UpgradeHouseModule(retired,0);assert(retired.houseModuleLevel[0]==4 && retired.gold==gold-600);
GameState noPlot;noPlot.gold=10000;BuildHouseModule(noPlot,0);assert(noPlot.houseModuleLevel[0]==0 && noPlot.gold==10000);
GameState poor;poor.housePlotIdx=0;poor.gold=1;BuildHouseModule(poor,0);assert(poor.houseModuleLevel[0]==0 && poor.gold==1);
GameState complete;complete.housePlotIdx=0;complete.gold=10000;complete.houseModuleLevel[0]=5;UpgradeHouseModule(complete,0);assert(complete.gold==10000 && complete.houseModuleLevel[0]==5);
SaveGame(retired);GameState restored;assert(LoadGame(restored));
assert(restored.guildName==retired.guildName && restored.guildWarOn[0] && restored.guildRecruits.size()==1 && restored.guildRecruits[0].along);
assert(restored.settle[kSbForge].level==6 && restored.houseModuleLevel[0]==4 && restored.gold==retired.gold);
assert(restored.settleUpgrading==retired.settleUpgrading && restored.settleUpgradeT==retired.settleUpgradeT);
std::cout<<"PASS homestead: archived management stays paused, no hidden wars or follower costs, direct workshops preserve inherited levels and enforce prices"<<std::endl;
}


{
GameState life;life.housePlotIdx=0;life.houseLayout=HouseStarterLayout(7);life.gold=3000;life.wood=300;life.ore=100;life.leather=100;
life.buildingSkill[0]=49.99f;assert(!TryBuildHomeAddition(life,0) && life.gold==3000);
life.buildingSkill[0]=50;int before=(int)std::count_if(life.houseLayout.begin(),life.houseLayout.end(),[](char c){return c!='.';});
assert(TryBuildHomeAddition(life,0) && life.gold==2850 && life.wood==270 && life.ore==85);
assert(life.life.additions==1 && HomeWingLevel(life,0)==3 && HouseHasDoor(life.houseLayout,7));
assert(std::count_if(life.houseLayout.begin(),life.houseLayout.end(),[](char c){return c!='.';})>before);
assert(!TryBuildHomeAddition(life,0) && life.gold==2850);
assert(TryBuyFarm(life) && life.life.farm && life.life.seeds[0]==6);
assert(!TryBuyFarm(life));life.screen=Screen::House;g_farmOpen=true;g_playScreen=Screen::Wilderness;life.wildernessPlayerPos={life.life.x,life.life.y};
assert(TryFarmBed(life,0,0) && life.life.seeds[0]==5);assert(TryFarmBed(life,0,0));
tflife::Grow(life.life,180);assert(TryFarmBed(life,0,0) && life.life.produce[0]==3);
int gold=life.gold;assert(TryUseProduce(life,0,true) && life.gold==gold+24 && life.life.produce[0]==0);
assert(!TryUseProduce(life,0,true));
reset();g_farmOpen=true;DrawHouseScreen(life,960,540);check("farm beds");
life.wildernessPlayerPos={0,0};assert(!TryFarmBed(life,1,0));
life.wildernessPlayerPos=kHousePlots[0].pos;life.life.condition=50;int wood=life.wood;assert(TryMaintainHome(life) && life.life.condition==75 && life.wood==wood-5);
SaveGame(life);GameState restored;assert(LoadGame(restored));assert(restored.life.farm && restored.life.additions==1 && restored.life.seeds[0]==5 && restored.life.condition==75);
if(const char* path=std::getenv("TF_HOMESTEAD_FIXTURE")) {
    auto fixture=life;fixture.optOnline=false;fixture.autoReagents=false;fixture.gold=1000;fixture.life.seeds={6,3,3};fixture.life.produce={8,4,3};fixture.life.cooldown=360;fixture.youngT=1200;fixture.starterStep=-1;fixture.starterAccepted=fixture.starterRewarded=true;
    fixture.life.beds[0]={0,180,true};SaveGame(fixture);std::ifstream from(kSaveFilePath);std::ofstream to(path);to<<from.rdbuf();
}
if(const char* path=std::getenv("TF_INTERIOR_FIXTURE")) {
    GameState fixture;fixture.optClassic2D=true;fixture.optOnline=false;fixture.youngT=1200;fixture.starterStep=-1;fixture.starterAccepted=fixture.starterRewarded=true;
    fixture.screen=Screen::Wilderness;fixture.selectedTown=0;
    for(const auto& n:ActiveTownNodes(0))if(std::string(n.key)=="smith")fixture.wildernessPlayerPos=OpenTownWorld(0,{n.pos.x,n.pos.y+150});
    SaveGame(fixture);std::ifstream from(kSaveFilePath);std::ofstream to(path);to<<from.rdbuf();
}
{
 GameState route;route.townPlayerPos=TS(450,830);auto gate=kTownGates[0].wildernessPos;auto start=OpenTownWorld(0,route.townPlayerPos);
 for(const auto& node:ActiveTownNodes(0))ResolveCircleCollision(start,kPlayerRadius,OpenTownWorld(0,node.pos),35.f);
 std::array<Vector2,5> points={{start,{gate.x+90,start.y},{gate.x+90,gate.y-45},{gate.x,gate.y-45},{gate.x,gate.y+100}}};
 for(int i=1;i<5;i++)for(int sample=1;sample<=100;sample++){
  Vector2 p={points[i-1].x+(points[i].x-points[i-1].x)*sample/100,points[i-1].y+(points[i].y-points[i-1].y)*sample/100};
  for(const auto& n:ActiveTownNodes(0))assert(Dist(p,OpenTownWorld(0,n.pos))>=35+kPlayerRadius-.1f);
  auto resolved=p;GateCollide(resolved,gate,TownGateFacing(0));assert(Dist(p,resolved)<.1f);
 }
 std::cout<<"PASS town route: real arrival, clear side lane and open gate geometry"<<std::endl;
}
if(const char* path=std::getenv("TF_GRAPHICS_FIXTURE")){
 GameState fixture;fixture.optClassic2D=true;fixture.optOnline=false;fixture.youngT=1200;fixture.starterStep=-1;fixture.starterAccepted=fixture.starterRewarded=true;fixture.gold=1000;fixture.autoReagents=false;
 Pet pet{};pet.id=1;pet.name="My renamed wolf";pet.species=1;pet.role=PetRole::Melee;pet.hp=pet.maxHp=100;pet.active=true;fixture.pets.push_back(pet);fixture.nextPetId=2;
 Item sword{};sword.id=100;sword.name="Iron Sword";sword.type=ItemType::Weapon;sword.category="Swordsmanship";sword.power=15;fixture.equipped.rightHand=sword;
 SaveGame(fixture);std::ifstream from(kSaveFilePath);std::ofstream to(path);to<<from.rdbuf();
}
g_farmOpen=false;GameState target;target.screen=Screen::Wilderness;target.youngT=0;target.starterStep=-1;target.wildernessPlayerPos=WP(450,1300);
assert(SpawnRovingPack(target));assert(target.wildEngaged && target.wildExtraAttackers.size()>=2 && target.wildExtraAttackers.size()<=7);
assert(!SpawnRovingPack(target)); // never stack a pack onto an existing duel/fight
for(const auto& am:target.wildExtraAttackers)assert(am.hp>0 && am.spotIdx>=0 && am.spotIdx<12 && !WildBlocked(am.pos));
GameState safe;safe.screen=Screen::Town;assert(!SpawnRovingPack(safe));safe.screen=Screen::Wilderness;safe.playerIsGhost=true;assert(!SpawnRovingPack(safe));
std::cout<<"PASS homestead loop: skill-50 physical addition, materials, farm ownership/work/progression, produce, upkeep, real save round trip, bounded real-monster pack and duel safety"<<std::endl;
}

}
'''
(run_dir/'menu.cpp').write_text(test)

try:
    exe=run_dir/'menus'
    subprocess.run(['g++','-std=c++17','-O0','-ffunction-sections','-fdata-sections','-I'+str(raylib),str(run_dir/'menu.cpp'),str(run_dir/'stubs.cpp'),'-Wl,--gc-sections','-o',str(exe)],check=True)
    subprocess.run([str(exe)],check=True,cwd=run_dir)
    print('PASS production landscape pages: full inventories, pets, all craft modes, journal tabs, guild tabs, vendors, help, native navigation and visible control bounds')
finally:shutil.rmtree(run_dir)

