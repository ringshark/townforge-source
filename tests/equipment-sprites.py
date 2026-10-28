"""Exercise production equipment poses and render their real draw commands to SVG."""
from pathlib import Path
import re,os,subprocess,shutil,json,base64,html,math
from PIL import Image
root=Path(__file__).resolve().parents[1]
ns={'__file__':str(root/'tests/landscape-menus.py')}
source=(root/'tests/landscape-menus.py').read_text();exec(source[:source.index("test='''")],ns)
run=ns['run_dir'];header=(root/'equipment_sprites_2d.h').read_text();initial=[];assets={}
for ident,(key,path,cols,rows,xedges) in enumerate(re.findall(r'tfgfx::LoadAtlas\((\w+),"([^"]+)",(\d+),\{([^}]+)\}(?:,\{([^}]+)\})?\)',header),1):
    image=Image.open(root/path).convert('RGBA');rows=[eval(x.replace('f','')) for x in rows.split(',')];cols=int(cols);edges=[eval(x.replace('f','')) for x in xedges.split(',')] if xedges else [x/cols for x in range(cols+1)];frames=[]
    for row in range(len(rows)-1):
        for col in range(cols):
            x0=int(edges[col]*image.width);y0=int(rows[row]*image.height);x1=int(edges[col+1]*image.width);y1=int(rows[row+1]*image.height)
            b=image.getchannel('A').crop((x0,y0,x1,y1)).point(lambda a:255 if a>48 else 0).getbbox();assert b,(key,row,col)
            assert b[0]>0 and b[1]>0 and b[2]<x1-x0 and b[3]<y1-y0,(key,row,col,'cell clipped')
            frames.append([x0+b[0],y0+b[1],b[2]-b[0],b[3]-b[1]])
    assert len(frames)==(12 if key=='weapons' else 36)
    assets[ident]={'path':path,'size':image.size,'data':base64.b64encode((root/path).read_bytes()).decode()}
    initial.append(f'tfequip2d::{key}.texture={{{ident},{image.width},{image.height},1,7}};tfequip2d::{key}.frames={{'+','.join('{'+','.join(map(str,b))+'}' for b in frames)+'};')
# Log the production renderer's texture transforms, including the actual grip pivot.
stubs=ns['stubs'].replace('struct Label{','FILE* testRender=nullptr;\nstruct Label{')
pat=r'void DrawTexturePro\([^\n]+?\) \{[^\n]*\}'
body='void DrawTexturePro(Texture2D texture,Rectangle source,Rectangle dest,Vector2 origin,float rotation,Color tint) {if(testRender)fprintf(testRender,"T %u %f %f %f %f %f %f %f %f %f %f %f\\n",texture.id,source.x,source.y,source.width,source.height,dest.x,dest.y,dest.width,dest.height,origin.x,origin.y,rotation);}'
stubs,n=re.subn(pat,lambda _:body,stubs);assert n==1
(run/'equipment-stubs.cpp').write_text(stubs)
cpp=r'''
#include <cassert>
#include <iostream>
void JS_CloudOpen(){}
#define main game_main
#include "MAIN_PATH"
#undef main
extern FILE* testRender;
static Item weapon(int id,const char* name,const char* category,const char* handed="1h"){Item x{};x.id=id;x.name=name;x.type=ItemType::Weapon;x.category=category;x.handed=handed;return x;}
static Item armor(int id,const char* slot,const char* material){Item x{};x.id=id;x.name=std::string(material)+" "+slot;x.type=ItemType::Armor;x.category=material;x.slot=slot;return x;}
int main(){
INITIAL
using namespace tfequip2d;GameState s;s.screen=Screen::Wilderness;s.playerHurtT=-1;s.wildEngaged.emplace();s.optClassic2D=true;s.optOnline=false;s.youngT=1200;s.starterStep=-1;s.starterAccepted=s.starterRewarded=true;
auto fixture=[&](const char* name){if(const char* prefix=getenv("TF_EQUIPMENT_FIXTURE")){SaveGame(s);std::ifstream from(kSaveFilePath);std::ofstream to(std::string(prefix)+"-"+name+".txt");to<<from.rdbuf();}};
testRender=fopen("TRACE_PATH","w");assert(testRender);
auto sample=[&](const std::string& label,int direction,ActorAnim anim,bool moving=false){fprintf(testRender,"B %s\n",label.c_str());assert(Draw(s,{0,0},direction,anim,moving,100,WHITE));for(auto hand:lastPose.hand)assert(std::isfinite(hand.x)&&std::isfinite(hand.y));};
const char* materials[]={"","Leather","Chain Mail","Plate Mail"};
for(int m=0;m<4;m++){
 if(m){s.equipped.helmet=armor(1,"helmet",materials[m]);s.equipped.chest=armor(2,"chest",materials[m]);s.equipped.arms=armor(3,"arms",materials[m]);s.equipped.gloves=armor(4,"gloves",materials[m]);s.equipped.legs=armor(5,"legs",materials[m]);}
 s.equipped.rightHand=weapon(90,"Cutlass","Swordsmanship");fixture(m==0?"base":m==1?"leather":m==2?"chain":"plate");
 for(int d=0;d<4;d++){sample(std::string(materials[m])+" idle dir "+std::to_string(d),d,ActorAnim::Idle);assert(lastMaterials[0]==m&&lastMaterials[1]==m&&lastMaterials[4]==m);}
}
s.equipped={};s.equipped.helmet=armor(1,"helmet","Plate Mail");s.equipped.chest=armor(2,"chest","Leather");s.equipped.legs=armor(3,"legs","Chain Mail");s.equipped.rightHand=weapon(91,"Spear","Fencing","2h");fixture("mixed");sample("Mixed individual armor slots",0,ActorAnim::Idle);assert(lastMaterials[0]==3&&lastMaterials[1]==1&&lastMaterials[4]==2&&lastMaterials[2]==0);
s.equipped={};
std::vector<Item> items={weapon(1,"Cutlass","Swordsmanship"),weapon(2,"Longsword","Swordsmanship"),weapon(3,"Kryss","Fencing"),weapon(4,"Spear","Fencing","2h"),weapon(5,"War Axe","Macing"),weapon(6,"Mace","Macing"),weapon(7,"Quarterstaff","Macing","2h"),weapon(8,"Bow","Archery","2h"),weapon(9,"Crossbow","Archery","2h")};
for(const auto& item:items){s.equipped.rightHand=item;for(int d=0;d<4;d++){
 s.wildEngaged->swingEffectTimer=.32f;auto first=MakePose(s,d,ActorAnim::Attack,false);s.wildEngaged->swingEffectTimer=.15f;sample(item.name+" attack dir "+std::to_string(d),d,ActorAnim::Attack);auto mid=lastPose;
 assert(first.angle!=mid.angle || std::hypot(first.hand[0].x-mid.hand[0].x,first.hand[0].y-mid.hand[0].y)>1 || mid.action==Action::Shoot);
 if(item.handed=="2h")assert(mid.twoHands);
 if(mid.action==Action::Thrust || mid.action==Action::Shoot){auto meta=HeldMetadata(mid.weapon);float a=(mid.angle+(mid.action==Action::Shoot?meta.rotation:0))*DEG2RAD;Vector2 forward=Rotate(mid.weapon==Weapon::Bow?Vector2{-1,0}:Vector2{0,-1},a);Vector2 expect=d==0?Vector2{0,1}:d==1?Vector2{-1,0}:d==2?Vector2{1,0}:Vector2{0,-1};assert(forward.x*expect.x+forward.y*expect.y>.99f);}
}}
s.equipped.rightHand=items[0];s.equipped.leftHand=armor(13,"shield","Shield");sample("Sword and independent shield",0,ActorAnim::Idle);assert(lastPose.offhand==Weapon::Shield&&!lastPose.twoHands);
s.equipped={};sample("Unarmed attack has no sword",0,ActorAnim::Attack);assert(lastPose.weapon==Weapon::None);
for(const char* resource:{"wood","ore","richore","fish"}){
 s.gatheringResource=resource;s.gatherDuration=8;
 for(int step=0;step<4;step++){s.gatherSecondsRemaining=8-step*1.9f;sample(std::string(resource)+" work phase "+std::to_string(step),step,ActorAnim::Idle);assert(lastPose.weapon==(*s.gatheringResource=="fish"?Weapon::Fishing:*s.gatheringResource=="wood"?Weapon::Axe:Weapon::Pickaxe));}
 auto first=MakePose(s,0,ActorAnim::Idle,false);s.gatherSecondsRemaining-=.25f;auto next=MakePose(s,0,ActorAnim::Idle,false);assert(first.phase!=next.phase);
}
s.gatheringResource="fish";s.gatherSecondsRemaining=.3f;sample("Fishing final reel",0,ActorAnim::Idle);assert(lastPose.phase>.75f&&lastPose.line==1);
s.gatheringResource.reset();for(int f=0;f<8;f++){s.worldTime=f*.09f;sample("Walk phase "+std::to_string(f),f%4,ActorAnim::Idle,true);assert(lastPose.action==Action::Walk);}
for(int d=0;d<4;d++){s.playerHurtT=.15f;sample("Hit reaction dir "+std::to_string(d),d,ActorAnim::Idle);assert(lastPose.action==Action::Hurt);}s.playerHurtT=-1;
for(int d=0;d<4;d++){s.wildEngaged->castEffectTimer=.18f;sample("Empty hand spell dir "+std::to_string(d),d,ActorAnim::Cast);}
for(int f=0;f<4;f++){s.playerDeathAnimT=1.2f-f*.29f;sample("Fall phase "+std::to_string(f),0,ActorAnim::Idle);assert(lastPose.action==Action::Fall);}s.playerDeathAnimT=0;
// Actual serializer preserves mixed armor, weapon identity and stats.
s.equipped.rightHand=items[3];s.equipped.helmet=armor(20,"helmet","Plate Mail");s.equipped.chest=armor(21,"chest","Leather");SaveGame(s);GameState restored;assert(LoadGame(restored));assert(WeaponFor(&*restored.equipped.rightHand)==Weapon::Spear&&ArmorMaterial(restored.equipped.helmet)==3&&ArmorMaterial(restored.equipped.chest)==1);
fprintf(testRender,"B Actual world renderer entry\n");g_compactHeroAtlas=true;int before=drawCount;DrawPlayer(s,{0,0},{1,0},"",1,ActorAnim::Idle);assert(drawCount==before+1);
fclose(testRender);std::cout<<"PASS layered sprites: 84 isolated art parts, all four directions, independent armor slots, grip-anchored weapon families, bow/spear aim, two-hand poses, unarmed/cast/hit/fall, fishing/mining/chopping and unchanged equipment save round trip"<<std::endl;
}
'''.replace('MAIN_PATH',str(root/'main.cpp')).replace('INITIAL','\n'.join(initial)).replace('TRACE_PATH',str(run/'trace.txt'))
(run/'equipment.cpp').write_text(cpp)
try:
 subprocess.run(['g++','-std=c++17','-O0','-ffunction-sections','-fdata-sections','-I'+str(ns['raylib']),str(run/'equipment.cpp'),str(run/'equipment-stubs.cpp'),'-Wl,--gc-sections','-o',str(run/'equipment')],check=True)
 subprocess.run([str(run/'equipment')],check=True,cwd=run)
 samples=[]
 for line in (run/'trace.txt').read_text().splitlines():
  if line.startswith('B '):samples.append([line[2:],[]])
  elif line.startswith('T '):samples[-1][1].append(list(map(float,line[2:].split())))
 width=1440;height=math.ceil(len(samples)/8)*210
 svg=[f'<svg xmlns="http://www.w3.org/2000/svg" xmlns:xlink="http://www.w3.org/1999/xlink" width="{width}" height="{height}"><rect width="100%" height="100%" fill="#283b3b"/><defs>']
 for ident,a in assets.items():svg.append(f'<image id="art{ident}" width="{a["size"][0]}" height="{a["size"][1]}" xlink:href="data:image/png;base64,{a["data"]}"/>')
 svg.append('</defs>')
 for index,(label,draws) in enumerate(samples):
  gx=index%8*180;gy=index//8*210;svg.append(f'<g transform="translate({gx+90} {gy+110})">')
  for tex,sx,sy,sw,sh,x,y,w,h,ox,oy,angle in draws:
   svg.append(f'<g transform="translate({x} {y}) rotate({angle}) translate({-ox} {-oy})">')
   if sw<0:svg.append(f'<g transform="translate({w} 0) scale(-1 1)">')
   svg.append(f'<svg width="{w}" height="{h}" viewBox="{sx} {sy} {abs(sw)} {sh}" preserveAspectRatio="none"><use xlink:href="#art{int(tex)}"/></svg>')
   if sw<0:svg.append('</g>')
   svg.append('</g>')
  svg.append('</g>');svg.append(f'<text x="{gx+6}" y="{gy+193}" font-size="10" fill="#ecdec2">{html.escape(label)}</text>')
 svg.append('</svg>');out=root/'browser-checks';out.mkdir(exist_ok=True);(out/'layered-equipment-native.svg').write_text(''.join(svg))
 print('PASS native draw-command contact sheet:',len(samples),'poses')
finally:shutil.rmtree(run)
