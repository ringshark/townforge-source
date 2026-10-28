// Original compact Town Forge adventurer. Local coordinates are in world units;
// the animated hierarchy carries equipment with the hands rather than the camera.
#include "compact_motion.h"
static constexpr float kCompactHeroHeight = 46.0f; // old sculpted hero: 66
static bool DrawCompactAdventurer(int track, float x, float z, float yaw, Color tint,
                                 const Equipment& equipment, const HumanPose& p,
                                 bool shadowPass, bool sneaking, bool blocking) {
    if(shadowPass)return true;
    T3DLiftScope terrain(x,z);
    HumanOutfit o=HumanOutfitFor(equipment);
    HumanEnsure();
    const Color olive{93,106,57,255}, scarf{66,79,43,255}, leather{100,65,39,255};
    const Color skin{222,174,127,255}, hair{67,43,29,255}, steel{128,139,145,255};
    Color top=equipment.robe ? ClothColor(*equipment.robe):equipment.shirt ? ClothColor(*equipment.shirt):olive;
    Color pants=equipment.pants ? ClothColor(*equipment.pants):Color{54,51,47,255};
    Color boots=equipment.shoes ? ClothColor(*equipment.shoes):leather;
    auto color=[&](Color c){return HumanMul(c,tint);};
    auto box=[&](Vector3 pos,Vector3 size,Color c){DrawCubeV(pos,size,color(c));};
    auto oval=[&](Vector3 pos,Vector3 size,Color c){
        rlPushMatrix();rlTranslatef(pos.x,pos.y,pos.z);rlScalef(size.x,size.y,size.z);
        DrawSphereEx({0,0,0},1,6,8,color(c));rlPopMatrix();
    };
    const float move=std::clamp(p.move,0.f,1.f);
    float cycle=(float)g_gameClock*9.0f, stride=sinf(cycle)*27*move;
    float atk=p.attackT>=0 ? sinf(std::clamp(p.attackT,0.f,1.f)*PI):0;
    float cast=p.castT>=0 ? sinf(std::clamp(p.castT,0.f,1.f)*PI):0;
    const bool gathering=p.gather>0 && move<.1f && p.attackT<0 && p.castT<0 && p.deathT<0;
    const bool mining=gathering && p.gather==2;
    const auto mine=tfcompact::Mining((float)g_gameClock);
    float work=gathering ? .5f+.5f*sinf((float)g_gameClock*5.f):0;
    float death=p.deathT>=0 ? std::clamp(p.deathT,0.f,1.f):0;
    float bob=fabsf(sinf(cycle))*move*.9f;
    // Feet remain on the same terrain; only the visible avatar changes scale.
    DrawCircle3D({x,.7f,z},8,{1,0,0},90,Fade(BLACK,.23f*tint.a/255.f));
    rlPushMatrix();rlTranslatef(x,bob,z);rlRotatef(90-yaw*RAD2DEG,0,1,0);
    rlRotatef(-85*death,1,0,0);
    // Lean from the hips; keep the planted feet out of the work transform.
    if(sneaking)rlRotatef(12,1,0,0);
    if(p.hurtT>=0)rlRotatef(-9*sinf(p.hurtT*PI),0,0,1);
    // Walking legs: hips -> knees -> boots, opposite phases for each side.
    for(int side:{-1,1}) {
        rlPushMatrix();rlTranslatef(side*4.1f,20,0);rlRotatef(side*stride,1,0,0);
        box({0,-4,0},{6.3f,9,6.6f},pants);
        rlTranslatef(0,-8,0);rlRotatef(std::max(0.f,-side*stride)*.7f,1,0,0);
        box({0,-4,0},{5.6f,8,5.7f},pants);
        box({0,-7,1.1f},{7.1f,6,9.3f},boots);
        box({0,-9.6f,1.1f},{7.3f,1.1f,9.5f},Color{48,33,24,255});
        if(o.armLegs)box({0,-2,3.1f},{5.8f,6,1.6f},o.armLegsCol);
        rlPopMatrix();
    }
    rlPushMatrix();rlTranslatef(0,20,0);rlRotatef(mining ? mine.lean:0,1,0,0);rlTranslatef(0,-20,0);
    // Broad tunic, split hem, belt and brass buckle.
    oval({0,26,0},{8.3f,8.2f,5.5f},top);
    box({0,19.5f,0},{15.5f,5,10.1f},ColorBrightness(top,-.08f));
    box({0,21.3f,0},{16.4f,2.5f,10.9f},leather);
    box({0,21.4f,5.7f},{3.7f,2.8f,1},Color{190,151,75,255});
    box({0,21.4f,6.3f},{2,1.4f,.4f},leather);
    if(o.armChest && !o.robe)oval({0,27.4f,1.3f},{8.4f,6.1f,5.1f},o.armChestCol);
    if(o.robe)box({0,17.5f,0},{16.5f,10,11.4f},top);
    // Backpack with a flap, straps and buckle; no baked-in weapon or shield.
    box({0,26.5f,-7.1f},{11.4f,12,5.3f},Color{140,98,54,255});
    box({0,30.5f,-10},{12,4.2f,1.6f},Color{165,120,67,255});
    box({0,26,-10},{1.9f,9,.8f},leather);
    box({0,26.7f,-10.7f},{3,2.8f,.7f},Color{190,151,75,255});
    for(int side:{-1,1})box({side*5.6f,27,5.1f},{1.7f,11,1},leather);
    if(equipment.cloak)box({0,25,-10.5f},{15,15,1},ClothColor(*equipment.cloak));
    oval({0,33,0},{6.7f,2.7f,5.7f},scarf);
    oval({0,33,-4},{5.7f,3.6f,3.1f},scarf);
    // Compact face and swept hair. Head is deliberately larger relative to torso.
    oval({0,39,0},{6.7f,7,5.7f},skin);
    oval({0,43,-.9f},{7,3.7f,5.6f},hair);
    for(int i=0;i<4;i++)oval({-4.5f+i*2.8f,43.4f+(i%2)*.9f,3.6f},{2.9f,2.8f,2.5f},i%2 ? hair:Color{83,54,35,255});
    for(int side:{-1,1}) {
        box({side*2.4f,39.3f,5.1f},{1.5f,1.9f,.7f},Color{35,26,20,255});
        box({side*2.4f,41,4.9f},{2.2f,.8f,.8f},hair);
    }
    oval({0,37.7f,5.5f},{1.3f,1.4f,1},skin);
    if(o.helm!=kHhNone){oval({0,43,-.8f},{7.4f,4.3f,6.1f},o.helmCol);box({0,42,5.2f},{12,1.4f,1.2f},o.helmCol);}
    else if(o.hat!=kClStyleNone){oval({0,44,-.5f},{7.8f,4.2f,6.5f},o.hatCol);}
    // Arms and held items share transforms, including swings, casts and blocks.
    for(int side:{-1,1}) {
        bool right=side==-1;
        rlPushMatrix();rlTranslatef(side*9,30,0);
        float swing=-side*stride*.7f;
        if(right)swing-=atk*115+work*85+cast*95;
        else swing-=blocking ? 70:cast*85;
        if(mining)swing=right ? mine.shoulder:-52.f;
        if(!gathering && o.style==kHsBow && p.engaged)swing=-75;
        rlRotatef(swing,1,0,0);
        oval({0,0,0},{3.7f,3.1f,4.3f},o.armArms ? o.armArmsCol:steel);
        box({0,-4,0},{4.5f,7,4.8f},top);
        rlTranslatef(0,-7,0);rlRotatef(mining ? (right ? mine.elbow:-65.f):(right ? -atk*40:0),1,0,0);
        box({0,-3,0},{4.2f,6.5f,4.4f},skin);
        box({0,-5.7f,0},{4.8f,5,5},o.armArms ? o.armArmsCol:leather);
        oval({0,-8,0},{2.6f,2.7f,2.6f},leather);
        rlTranslatef(0,-8,0);
        int weapon=gathering ? (p.gather==1 ? kHwAxe:p.gather==2 ? kHwPickaxe:kHwRod):o.weapon;
        const bool bow=!gathering && o.style==kHsBow;
        if((right && !bow) || (!right && bow)) {
            const Model* model=gathering ? nullptr:MeshWeaponModel(o.meshWeapon);
            float sc=17.f;
            if(!model && weapon>=0 && weapon<kHwGearCount && g_human.gearOk[weapon]) {
                model=&g_human.gear[weapon];
                static const float scales[]={.56f,.56f,.42f,.8f,.5f,.56f,1,1,1,1,1,1,1,1};
                sc*=scales[weapon]*(gathering ? 1.f:o.weaponScale);
            }
            // Grip-space +Y is the blade/shaft. A forward carry keeps swords
            // clear of the boots and visible beside the body in the paper doll.
            // The pick uses its own steeper grip for an overhead work arc.
            if(model){
                rlPushMatrix();
                rlRotatef(bow ? 0:mining ? 130:gathering ? 100:25,1,0,0);
                if(!bow && !gathering)rlRotatef(15,0,0,1); // outside the right hip, clear of the tunic
                DrawModel(*model,{0,0,0},sc,tint);rlPopMatrix();
            }
        }
        if(!right && o.shield && !gathering) {
            oval({-1,2,2.2f},{1.9f,7.3f,7.3f},steel);
            oval({-2.5f,2,2.2f},{.7f,6.2f,6.2f},Color{126,83,45,255});
            oval({-3.2f,2,2.2f},{.8f,2.1f,2.1f},steel);
        }
        rlPopMatrix();
    }
    rlPopMatrix();rlPopMatrix();return true;
}

