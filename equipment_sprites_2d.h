#pragma once
// Art is attached to a shared pose, never an inventory icon or baked-in sword.
namespace tfequip2d {
enum class Weapon { None, Sword, Longsword, Rapier, Spear, Axe, Mace, Staff, Bow, Crossbow, Shield, Pickaxe, Fishing };
enum class Action { Idle, Walk, Slash, Thrust, Smash, Shoot, Cast, Chop, Mine, Fish, Hurt, Fall };
static tfgfx::Atlas soft,metal,weapons;
static void Load();
static void Unload(){for(auto* a:{&soft,&metal,&weapons}){if(a->texture.id)UnloadTexture(a->texture);*a={};}}
static int ArmorMaterial(const std::optional<Item>& item){
    if(!item || item->type!=ItemType::Armor)return 0;
    std::string key=item->category+" "+item->name;
    std::transform(key.begin(),key.end(),key.begin(),[](unsigned char c){return (char)std::tolower(c);});
    if(key.find("plate")!=std::string::npos)return 3;
    if(key.find("chain")!=std::string::npos || key.find("ring")!=std::string::npos)return 2;
    return 1;
}
static Weapon WeaponFor(const Item* item){
    if(!item)return Weapon::None;
    if(item->type==ItemType::Armor && item->slot=="shield")return Weapon::Shield;
    if(item->type!=ItemType::Weapon)return Weapon::None;
    std::string n=item->name;std::transform(n.begin(),n.end(),n.begin(),[](unsigned char c){return (char)std::tolower(c);});
    if(item->category=="Archery")return n.find("crossbow")!=std::string::npos?Weapon::Crossbow:Weapon::Bow;
    if(n.find("staff")!=std::string::npos)return Weapon::Staff;
    if(n.find("spear")!=std::string::npos || n.find("pike")!=std::string::npos || n.find("fork")!=std::string::npos || n.find("halberd")!=std::string::npos)return Weapon::Spear;
    if(n.find("axe")!=std::string::npos)return Weapon::Axe;
    if(item->category=="Macing")return Weapon::Mace;
    if(item->category=="Fencing")return Weapon::Rapier;
    return item->handed=="2h" || n.find("longsword")!=std::string::npos?Weapon::Longsword:Weapon::Sword;
}
static bool TwoHands(Weapon w,const Item* item){return w==Weapon::Bow || w==Weapon::Crossbow || w==Weapon::Spear || w==Weapon::Staff || w==Weapon::Pickaxe || w==Weapon::Fishing || (item && item->handed=="2h");}
static Vector2 Add(Vector2 a,Vector2 b){return {a.x+b.x,a.y+b.y};}
static Vector2 Mul(Vector2 a,float f){return {a.x*f,a.y*f};}
static Vector2 Rotate(Vector2 p,float radians){return {p.x*cosf(radians)-p.y*sinf(radians),p.x*sinf(radians)+p.y*cosf(radians)};}
static float Lerp(float a,float b,float t){return a+(b-a)*t;}
struct Pose {
    Vector2 shoulder[2],elbow[2],hand[2],hip[2],knee[2],ankle[2];
    Vector2 head={0,0},torso={0,0};
    Weapon weapon=Weapon::None,offhand=Weapon::None;Action action=Action::Idle;
    int direction=0,view=0;float phase=0,angle=0,bodyAngle=0,bob=0,scale=1;
    bool mirror=false,twoHands=false;float line=0;
};
static Vector2 SolveElbow(Vector2 shoulder,Vector2 hand,int side){
    Vector2 delta={hand.x-shoulder.x,hand.y-shoulder.y};float d=std::max(.001f,std::hypot(delta.x,delta.y));
    float bend=sqrtf(std::max(0.f,17.f*17.f-std::min(33.5f,d)*std::min(33.5f,d)*.25f));
    return {(shoulder.x+hand.x)*.5f-delta.y/d*bend*side,(shoulder.y+hand.y)*.5f+delta.x/d*bend*side};
}
static Pose MakePose(const GameState& s,int direction,ActorAnim anim,bool moving){
    Pose p;p.direction=direction;p.view=direction==3?2:direction==0?0:1;p.mirror=direction==2;
    const Item* main=s.equipped.rightHand?&*s.equipped.rightHand:(s.equipped.leftHand?&*s.equipped.leftHand:nullptr);
    p.weapon=WeaponFor(main);
    if(s.equipped.leftHand && (!main || s.equipped.leftHand->id!=main->id))p.offhand=WeaponFor(&*s.equipped.leftHand);
    Vector2 face=direction==0?Vector2{0,1}:direction==1?Vector2{-1,0}:direction==2?Vector2{1,0}:Vector2{0,-1};
    float aim=atan2f(face.y,face.x)*RAD2DEG+90;
    float remaining=s.screen==Screen::Wilderness && s.wildEngaged?s.wildEngaged->swingEffectTimer:s.screen==Screen::Hunt && s.dungeonEngaged?s.dungeonEngaged->swingEffectTimer:0;
    if(moving){p.action=Action::Walk;p.phase=fmodf(s.worldTime*(IsKeyDown(KEY_LEFT_SHIFT)?3.f:2.f),1.f);}
    if(anim==ActorAnim::Attack){
        p.phase=std::clamp(1-remaining/.32f,0.f,1.f);
        p.action=p.weapon==Weapon::Spear || p.weapon==Weapon::Rapier?Action::Thrust:p.weapon==Weapon::Axe || p.weapon==Weapon::Mace || p.weapon==Weapon::Staff?Action::Smash:p.weapon==Weapon::Bow || p.weapon==Weapon::Crossbow?Action::Shoot:Action::Slash;
    }
    if(anim==ActorAnim::Cast){p.action=Action::Cast;float t=s.screen==Screen::Wilderness && s.wildEngaged?s.wildEngaged->castEffectTimer:s.screen==Screen::Hunt && s.dungeonEngaged?s.dungeonEngaged->castEffectTimer:0;p.phase=std::clamp(1-t/.35f,0.f,1.f);}
    if(s.gatheringResource){
        const auto& r=*s.gatheringResource;p.action=r=="fish"?Action::Fish:r=="wood"?Action::Chop:Action::Mine;
        p.weapon=p.action==Action::Fish?Weapon::Fishing:p.action==Action::Chop?Weapon::Axe:Weapon::Pickaxe;p.offhand=Weapon::None;
        float elapsed=std::max(0.f,s.gatherDuration-s.gatherSecondsRemaining);
        p.phase=p.action==Action::Fish?std::clamp(elapsed/std::max(.1f,s.gatherDuration),0.f,1.f):fmodf(elapsed*1.2f,1.f);
    }
    if(s.playerHurtT>=0){p.action=Action::Hurt;p.phase=std::clamp(s.playerHurtT/.3f,0.f,1.f);}
    if(s.playerDeathAnimT>0){p.action=Action::Fall;p.phase=std::clamp((1.2f-s.playerDeathAnimT)/1.2f,0.f,1.f);p.bodyAngle=p.phase*78;p.scale=1-p.phase*.12f;}
    float gait=sinf(p.phase*2*PI),walk=p.action==Action::Walk?gait:0;
    p.bob=p.action==Action::Walk?-fabsf(gait)*1.4f:0;
    float side=p.view==1?5.f:17.f,flip=direction==3?-1.f:1.f;
    for(int i=0;i<2;i++){
        float sign=(i==0?-1.f:1.f)*flip;
        p.shoulder[i]={sign*side,36};p.hand[i]={sign*(side+5),66};
        p.elbow[i]={sign*(side+7),50};p.hip[i]={sign*7,62};
        float step=walk*(i==0?1.f:-1.f);
        p.knee[i]={sign*7+(p.view==1?step*9:0),80-std::max(0.f,step)*2};
        p.ankle[i]={sign*8+(p.view==1?step*15:0),99-std::max(0.f,step)*6};
        p.hand[i].x-=p.view==1?step*10:0;p.hand[i].y+=p.view==1?0:step*4;
    }
    p.angle=p.view==1?(direction==1?-20.f:20.f):direction==3?-12.f:12.f;
    p.twoHands=TwoHands(p.weapon,main);
    float strike=sinf(p.phase*PI),reach=0;
    if(p.action==Action::Slash || p.action==Action::Smash || p.action==Action::Chop || p.action==Action::Mine){
        float swing=p.phase<.28f?Lerp(-95,-120,p.phase/.28f):p.phase<.66f?Lerp(-120,55,(p.phase-.28f)/.38f):Lerp(55,15,(p.phase-.66f)/.34f);
        p.angle=aim+swing;p.bodyAngle=face.x*strike*6;p.bob=strike*2;reach=strike*9;p.hand[0]={-5+face.x*(10+reach),52+face.y*(8+reach)};
        if(p.action==Action::Smash || p.action==Action::Mine){p.hand[0].y-=sinf(p.phase*PI)*10;p.angle=aim+Lerp(-125,35,p.phase);}
    }else if(p.action==Action::Thrust){p.angle=aim;p.hand[0]={-4+face.x*(8+strike*22),52+face.y*(7+strike*17)};}
    else if(p.action==Action::Shoot){p.angle=aim;p.hand[0]={face.x*23,51+face.y*14};p.hand[1]={p.hand[0].x-face.x*(8+strike*13),p.hand[0].y-face.y*(8+strike*13)+3};}
    else if(p.action==Action::Cast){p.hand[0]={-19-face.x*3,43-strike*14};p.hand[1]={19+face.x*3,43-strike*14};p.angle=direction==1?-25:25;}
    else if(p.action==Action::Fish){
        float cast=p.phase<.22f?p.phase/.22f:1.f;
        p.angle=aim+Lerp(-110,12,cast);p.hand[0]={face.x*16-3,54+face.y*9+sinf(p.phase*PI*8)*1.5f};p.line=cast;
        if(p.phase>.75f){p.hand[0].y-=sinf(p.phase*PI*24)*2;p.angle-=sinf(p.phase*PI*24)*5;}
    }else if(p.action==Action::Hurt){p.bodyAngle=-sinf(p.phase*PI)*12;p.hand[0].y-=4;p.hand[1].y-=4;}
    if(p.twoHands && p.action!=Action::Shoot && p.action!=Action::Cast){
        Vector2 axis=Rotate({0,-1},p.angle*DEG2RAD);p.hand[1]=Add(p.hand[0],Mul(axis,p.weapon==Weapon::Spear?15.f:p.weapon==Weapon::Longsword?-4.f:8.f));
    }
    for(int i=0;i<2;i++)p.elbow[i]=SolveElbow(p.shoulder[i],p.hand[i],p.shoulder[i].x<0?1:-1);
    p.head={0,0};p.torso={0,31};return p;
}
static Vector2 ScreenPoint(const Pose& pose,Vector2 point,Vector2 pos,float size){
    point.y+=pose.bob;point=Rotate({point.x,point.y-100},pose.bodyAngle*DEG2RAD);
    return {pos.x+point.x*size*.01f*pose.scale,pos.y+size*.48f+point.y*size*.01f*pose.scale};
}
static void Part(int material,int view,int part,Vector2 a,Vector2 b,float width,bool mirror,Color tint){
    const auto& atlas=material>=2?metal:soft;int row=(material%2)*3+view,cell=row*6+part;
    if(!atlas.texture.id || cell>=(int)atlas.frames.size())return;auto src=atlas.frames[cell];
    if(mirror)src.width=-src.width;
    float length=std::hypot(b.x-a.x,b.y-a.y);if(length<.01f)return;
    DrawTexturePro(atlas.texture,src,{a.x,a.y,width,length},{width*.5f,0},atan2f(b.y-a.y,b.x-a.x)*RAD2DEG-90,tint);
}
static int WeaponCell(Weapon w){switch(w){case Weapon::Sword:return 0;case Weapon::Longsword:return 1;case Weapon::Rapier:return 2;case Weapon::Spear:return 3;case Weapon::Axe:return 4;case Weapon::Mace:return 5;case Weapon::Staff:return 6;case Weapon::Bow:return 7;case Weapon::Crossbow:return 8;case Weapon::Shield:return 9;case Weapon::Pickaxe:return 10;case Weapon::Fishing:return 11;default:return -1;}}
struct HeldArt {float height,grip,rotation;float gripX=.5f;};
static HeldArt HeldMetadata(Weapon w){
    switch(w){case Weapon::Spear:return {92,.78f,0};case Weapon::Staff:return {87,.74f,0};case Weapon::Bow:return {66,.50f,90,.44f};case Weapon::Crossbow:return {49,.77f,0};case Weapon::Shield:return {37,.50f,0};case Weapon::Longsword:return {68,.88f,0};case Weapon::Fishing:return {104,.88f,0};case Weapon::Pickaxe:return {68,.83f,0};case Weapon::Rapier:return {61,.89f,0,.54f};case Weapon::Axe:return {53,.80f,0,.73f};default:return {53,.84f,0};}
}
static void Held(Weapon w,const Pose& pose,Vector2 hand,Vector2 pos,float size,Color tint){
    int cell=WeaponCell(w);if(cell<0 || !weapons.texture.id || cell>=(int)weapons.frames.size())return;
    auto src=weapons.frames[cell];auto meta=HeldMetadata(w);float height=meta.height*size*.01f*pose.scale,width=height*src.width/src.height;
    auto at=ScreenPoint(pose,hand,pos,size);float angle=(w==Weapon::Shield?0:pose.angle+(pose.action==Action::Shoot?meta.rotation:0))+pose.bodyAngle;
    DrawTexturePro(weapons.texture,src,{at.x,at.y,width,height},{width*meta.gripX,height*meta.grip},angle,tint);
}
static Pose lastPose;static int lastMaterials[6]={};static int drawCount=0;
static int previewMode=0;
static const char* ActionName(Action a){const char* names[]={"Idle","Walk","Slash","Thrust","Heavy swing","Shoot","Cast","Chop","Mine","Fish","Hurt","Fall"};return names[(int)a];}
static bool Draw(const GameState& s,Vector2 pos,int direction,ActorAnim anim,bool moving,float size,Color tint){
    if(!soft.texture.id || !metal.texture.id || !weapons.texture.id)return false;
    Pose p=MakePose(s,direction,anim,moving);lastPose=p;drawCount++;
    const auto& e=s.equipped;int head=ArmorMaterial(e.helmet),chest=ArmorMaterial(e.chest),arms=ArmorMaterial(e.arms),gloves=ArmorMaterial(e.gloves),legs=ArmorMaterial(e.legs);
    lastMaterials[0]=head;lastMaterials[1]=chest;lastMaterials[2]=arms;lastMaterials[3]=gloves;lastMaterials[4]=legs;lastMaterials[5]=ArmorMaterial(e.gorget);
    auto point=[&](Vector2 v){return ScreenPoint(p,v,pos,size);};
    auto part=[&](int material,int cell,Vector2 a,Vector2 b,float width,Color color){Part(material,p.view,cell,point(a),point(b),width*size*.01f*p.scale,p.mirror,color);};
    auto cloth=[&](const std::optional<Item>& item){if(item){Color c=ClothColor(*item);return Color{(unsigned char)(c.r*tint.r/255),(unsigned char)(c.g*tint.g/255),(unsigned char)(c.b*tint.b/255),tint.a};}return tint;};
    if(p.action!=Action::Fall)DrawEllipse((int)pos.x,(int)(pos.y+size*.48f),size*.22f,size*.06f,Fade(BLACK,.20f));
    auto arm=[&](int i){part(arms,2,p.shoulder[i],p.elbow[i],12,tint);part(gloves?gloves:arms,3,p.elbow[i],p.hand[i],10,tint);};
    // Far hand/equipment are occluded by the torso, near hand covers its grip.
    int far=p.view==2?0:1,near=1-far;
    if(p.direction==3){Held(p.weapon,p,p.hand[0],pos,size,tint);if(!p.twoHands)Held(p.offhand,p,p.hand[1],pos,size,tint);}
    arm(far);
    for(int i:{far,near}){part(legs,4,p.hip[i],p.knee[i],13,legs?tint:cloth(e.pants));part(legs,5,p.knee[i],p.ankle[i],14,legs?tint:cloth(e.shoes));}
    part(chest,1,{0,30},{0,65},33,chest?tint:cloth(e.shirt));
    if(e.gorget)part(ArmorMaterial(e.gorget),1,{0,28},{0,34},18,tint);
    part(head,0,{0,0},{0,31},34,tint);
    if(p.direction!=3){Held(p.weapon,p,p.hand[0],pos,size,tint);if(!p.twoHands)Held(p.offhand,p,p.hand[1],pos,size,tint);}
    arm(near);if(p.twoHands)arm(far);
    if(p.action==Action::Cast){auto a=point(p.hand[0]),b=point(p.hand[1]);DrawCircleLinesV(a,size*.11f,Fade(SKYBLUE,.8f));DrawCircleLinesV(b,size*.11f,Fade(SKYBLUE,.8f));}
    if(p.action==Action::Fish){
        auto meta=HeldMetadata(Weapon::Fishing);auto tip=Add(p.hand[0],Rotate({0,-meta.height*meta.grip},p.angle*DEG2RAD));
        Vector2 face=direction==0?Vector2{0,1}:direction==1?Vector2{-1,0}:direction==2?Vector2{1,0}:Vector2{0,-1};
        Vector2 bobber=Add(p.hand[0],Mul(face,(48+25*p.line)*(p.phase>.75f?Lerp(1,.45f,(p.phase-.75f)/.25f):1)));bobber.y+=22;
        if(p.phase>.22f){auto a=point(tip),b=point(bobber);DrawLineEx(a,b,std::max(.6f,size*.018f),Fade(Color{221,220,195,255},.75f));DrawCircleV(b,std::max(1.f,size*.035f),Color{205,76,57,255});}
    }
    return true;
}
static void Load(){
    tfgfx::LoadAtlas(soft,"assets/equipment_2d/base-leather.png",6,{0,229/1254.f,431/1254.f,618/1254.f,827/1254.f,1023/1254.f,1});
    tfgfx::LoadAtlas(metal,"assets/equipment_2d/chain-plate.png",6,{0,217/1254.f,411/1254.f,609/1254.f,813/1254.f,1008/1254.f,1},{0,1/6.f,440/1254.f,.5f,4/6.f,5/6.f,1});
    tfgfx::LoadAtlas(weapons,"assets/equipment_2d/weapons.png",4,{0,347/1024.f,679/1024.f,1});
    for(auto* a:{&soft,&metal,&weapons})if(a->texture.id)SetTextureFilter(a->texture,TEXTURE_FILTER_POINT);
}
}
