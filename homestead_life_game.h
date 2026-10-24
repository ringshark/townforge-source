#pragma once
// Included after the existing world, economy, combat, housing and UI helpers.
static bool g_farmOpen=false;
static int g_farmCrop=0;
static std::vector<int> g_rovingSpots;
static bool HomeLifeAlive(const GameState& s){return !s.playerIsGhost && s.playerDeathAnimT<=0;}
static bool HomeLifeQuiet(const GameState& s){return HomeLifeAlive(s) && !s.wildEngaged && s.wildExtraAttackers.empty() && !s.combat && !DenFighting();}
static bool FarmPresent(const GameState& s) {
    return s.life.farm && (s.screen==Screen::Wilderness || (s.screen==Screen::House && g_farmOpen && g_playScreen==Screen::Wilderness)) &&
        Dist(s.wildernessPlayerPos,{s.life.x,s.life.y})<140;
}
static bool FarmSite(const GameState& s,Vector2& out) {
    if(s.housePlotIdx<0)return false;
    if(s.life.farm){out={s.life.x,s.life.y};return true;}
    const auto& hp=kHousePlots[s.housePlotIdx];float offset=hp.cells*kHouseCellSize*.5f+110;
    for(Vector2 delta:std::array<Vector2,4>{{{offset,0},{-offset,0},{0,offset},{0,-offset}}}) {
        Vector2 p={hp.pos.x+delta.x,hp.pos.y+delta.y};bool clear=true;
        for(float y:{-58.f,0.f,58.f})for(float x:{-82.f,0.f,82.f}) {
            Vector2 q={p.x+x,p.y+y};
            if(q.x<80 || q.y<80 || q.x>kWildernessWorldSize-80 || q.y>kWildernessWorldSize-80 || WildBlocked(q) || OpenTownAt(q)>=0)clear=false;
        }
        if(clear){out=p;return true;}
    }
    return false;
}
static bool TryBuyFarm(GameState& s) {
    Vector2 p;if(s.life.farm || !HomeLifeQuiet(s) || !FarmSite(s,p) || !CanAfford(s,150) || s.wood<20)return false;
    PayGold(s,150);s.wood-=20;s.life.farm=true;s.life.x=p.x;s.life.y=p.y;s.life.seeds={6,3,3};
    s.logLine="Six farm beds are ready beside your home. Starter seeds included. Visit the beds to plant and water.";return true;
}
static bool TryFarmBed(GameState& s,int bed,int crop) {
    if(!HomeLifeQuiet(s) || !FarmPresent(s) || bed<0 || bed>=6)return false;
    auto& b=s.life.beds[bed];bool ok=false;
    if(b.crop<0)ok=tflife::Plant(s.life,bed,crop);
    else if(tflife::Ready(b)){int n=tflife::Harvest(s.life,bed);ok=n>0;s.logLine=TextFormat("Harvested %d crops. Ship them, eat vegetables, or turn herbs into reagents.",n);}
    else ok=tflife::Water(s.life,bed);
    if(ok){SkillUseGain(s.life.farming,.5f,.7f);RevealFromHiding(s,"");if(b.crop>=0)s.logLine=b.watered?"Watered. This bed will keep growing while you explore.":"Planted. Water this bed to start growth.";PlaySfx(SfxId::Coin);}
    return ok;
}
static bool TryBuySeeds(GameState& s,int crop) {
    if(!HomeLifeQuiet(s) || !s.life.farm || crop<0 || crop>=3 || s.life.seeds[crop]>99990 || !CanAfford(s,5*tflife::crops[crop].seedPrice))return false;
    PayGold(s,5*tflife::crops[crop].seedPrice);s.life.seeds[crop]+=5;return true;
}
static bool TryUseProduce(GameState& s,int crop,bool ship) {
    if(!HomeLifeQuiet(s) || crop<0 || crop>=3 || s.life.produce[crop]<=0)return false;
    if(ship){int n=s.life.produce[crop];s.life.produce[crop]=0;s.gold+=n*tflife::crops[crop].salePrice;s.logLine=TextFormat("Shipped %d %s for %d gold.",n,tflife::crops[crop].name,n*tflife::crops[crop].salePrice);}
    else if(crop==2){--s.life.produce[crop];s.reagents+=3;s.logLine="Prepared herbs: +3 reagents for your existing spells and alchemy.";}
    else {if(s.hp>=s.maxHp)return false;--s.life.produce[crop];s.hp=std::min(s.maxHp,s.hp+15);s.logLine="A homegrown meal restores 15 health.";}
    return true;
}
static bool TryMaintainHome(GameState& s) {
    if(s.housePlotIdx<0 || !HomeLifeQuiet(s) || s.wood<5 || s.life.condition>=100)return false;
    bool inside=s.screen==Screen::Interior && s.interiorKey=="wildhouse";
    bool outside=(s.screen==Screen::Wilderness || (s.screen==Screen::House && g_playScreen==Screen::Wilderness)) && Dist(s.wildernessPlayerPos,kHousePlots[s.housePlotIdx].pos)<420;
    if(!inside && !outside)return false;
    s.wood-=5;s.life.condition=std::min(100.f,s.life.condition+25);s.logLine="You mend and tidy your home. A well-kept home gives an extra crop at harvest.";return true;
}
// Expand one edge into unused plot cells, preserving custom layouts and moving
// any exterior door to the new boundary. Full/custom plots retain their layout.
static bool ExpandHomeWing(GameState& s,int side) {
    int n=kHousePlots[s.housePlotIdx].cells;if(!HouseLayoutValid(s.houseLayout,n))return false;
    int x0=n,y0=n,x1=-1,y1=-1;
    for(int y=0;y<n;y++)for(int x=0;x<n;x++)if(HouseCellAt(s.houseLayout,n,x,y)!='.'){x0=std::min(x0,x);y0=std::min(y0,y);x1=std::max(x1,x);y1=std::max(y1,y);}
    if(x1<x0 || y1<y0)return false;
    bool horizontal=side==0 || side==2;int delta=(side==0 || side==3)?-1:1;
    int old=horizontal?(side==0?y0:y1):(side==3?x0:x1),edge=old+delta;
    if(edge<0 || edge>=n)return false;
    int lo=horizontal?x0:y0,hi=horizontal?x1:y1;
    for(int a=lo;a<=hi;a++)if(HouseCellAt(s.houseLayout,n,horizontal?a:edge,horizontal?edge:a)!='.' || HouseCellAt(s.houseLayout,n,horizontal?a:old,horizontal?old:a)=='.')return false;
    for(int a=lo;a<=hi;a++) {
        char prev=HouseCellAt(s.houseLayout,n,horizontal?a:old,horizontal?old:a);
        char next=prev=='D'?'D':a==(lo+hi)/2?'N':'W';
        HouseSetCell(s.houseLayout,n,horizontal?a:edge,horizontal?edge:a,next);
        if(prev=='D' || (a>lo && a<hi))HouseSetCell(s.houseLayout,n,horizontal?a:old,horizontal?old:a,'F');
    }
    g_houseInt.ok=false;return true;
}
static const int kHomeWingWood[]={30,50,20,25},kHomeWingOre[]={15,0,0,10},kHomeWingLeather[]={0,0,10,0};
static bool CanBuildHomeAddition(const GameState& s,int m) {
    return m>=0 && m<4 && s.housePlotIdx>=0 && HomeLifeQuiet(s) && s.buildingSkill[m]>=50 && !(s.life.additions&(1<<m)) && CanAfford(s,150) && s.wood>=kHomeWingWood[m] && s.ore>=kHomeWingOre[m] && s.leather>=kHomeWingLeather[m];
}
static bool TryBuildHomeAddition(GameState& s,int m) {
    if(!CanBuildHomeAddition(s,m))return false;
    PayGold(s,150);s.wood-=kHomeWingWood[m];s.ore-=kHomeWingOre[m];s.leather-=kHomeWingLeather[m];
    s.houseModuleLevel[m]=std::max(3,HomeWingLevel(s,m));s.life.additions|=1<<m;
    bool expanded=ExpandHomeWing(s,m);
    if(s.houseFurnished) {
        const char* names[]={"Anvil","Carpenter's Bench","Dress Form","Brewing Cauldron"};int kind=HouseDecorFind(names[m]);
        bool exists=false;for(const auto& d:s.houseDecor)if(d.kind==kind)exists=true;
        if(!exists){HouseInteriorBuild(s);HouseAutoPlace(s,kind);}
    }
    s.logLine=std::string(kHomeModuleDefs[m].label)+(expanded?" added! Your house is larger; place its crafting bench inside.":" added within your existing house. Your custom layout is preserved.");
    PlaySfx(SfxId::Coin);return true;
}
static bool SpawnRovingPack(GameState& s) {
    if(s.screen!=Screen::Wilderness || !HomeLifeQuiet(s) || PlayerYoung(s) || OpenTownAt(s.wildernessPlayerPos)>=0)return false;
    std::vector<int> band;std::vector<Vector2> positions;
    int count=CombatPower(s)>=80?8:CombatPower(s)>=40?6:4;
    float cap=std::max(9.f,(float)CombatPower(s)*.28f),angle=RandUnit()*6.283185f;
    for(int i=0;i<12 && (int)band.size()<count;i++) {
        const auto& sp=kWildernessMonsterSpots[i];
        if(sp.iconIdx==4 || sp.level>cap || s.wildSpotRespawn[i]>0 || FindWildExtra(s,i) || FindDyingWildSpot(s,i) || Dist(sp.pos,s.wildernessPlayerPos)<450)continue;
        float a=angle+band.size()*1.05f;Vector2 p={s.wildernessPlayerPos.x+cosf(a)*230,s.wildernessPlayerPos.y+sinf(a)*230};
        p=ClampToWorld(p,80,kWildernessWorldSize);
        if(WildBlocked(p) || OpenTownAt(p)>=0)continue;
        band.push_back(i);positions.push_back(p);
    }
    if(band.size()<3)return false;
    g_rovingSpots=band;
    for(size_t k=0;k<band.size();k++) {
        GameState::ActiveMonster am;am.spotIdx=band[k];am.pos=am.spawnPos=positions[k];am.hp=am.maxHp=WildSpotMaxHp(s,band[k]);
        if(k==0)s.wildEngaged=am;else s.wildExtraAttackers.push_back(am);
    }
    s.autoGather=false;s.gatheringResource.reset();
    s.logLine=TextFormat("A roaming pack of %d closes in! Fight, use your spells, or retreat to town.",(int)band.size());
    s.rivalBanner="Roaming pack!";s.rivalBannerTimer=8;PlaySfx(SfxId::Hunt);return true;
}
static void TickHomesteadLife(GameState& s,float dt) {
    tflife::Grow(s.life,dt);
    if(s.housePlotIdx>=0 && HomeLifeAlive(s)){s.life.careTime+=dt;while(s.life.careTime>=1200){s.life.careTime-=1200;s.life.condition=std::max(0.f,s.life.condition-1);}}
    bool field=s.screen==Screen::Wilderness || (s.screen==Screen::House && g_farmOpen && g_playScreen==Screen::Wilderness);
    bool working=s.gatheringResource.has_value() || s.life.workTime>0;
    if(FarmPresent(s))for(const auto& b:s.life.beds)if(b.watered && !tflife::Ready(b))working=true;
    bool eligible=field && HomeLifeQuiet(s) && !PlayerYoung(s) && OpenTownAt(s.wildernessPlayerPos)<0;
    auto event=tflife::Watch(s.life,dt,working,eligible,RandUnit());
    if(event==tflife::Encounter::Warning) {
        s.screen=g_playScreen=Screen::Wilderness;g_farmOpen=false;s.exploreMenuOpen=false;WalkTargetClear();
        s.rivalBanner="Movement in the brush! A pack approaches...";s.rivalBannerTimer=8;
        s.logLine="A roaming pack is approaching. You have a few seconds to prepare or retreat.";PlaySfx(SfxId::Hunt);
    } else if(event==tflife::Encounter::Arrive){if(!SpawnRovingPack(s))s.logLine="The roaming pack passes in the distance.";}
    if(!g_rovingSpots.empty()) {
        bool alive=false,cleared=true;
        for(int i:g_rovingSpots){if((s.wildEngaged && !s.wildEngaged->isRival && s.wildEngaged->bladeIdx<0 && s.wildEngaged->spotIdx==i) || FindWildExtra(s,i))alive=true;if(s.wildSpotRespawn[i]<=0 && !FindDyingWildSpot(s,i))cleared=false;}
        if(!alive){if(cleared)s.logLine="Roaming pack cleared. Back to the homestead, or onward into the wilds.";g_rovingSpots.clear();}
    }
}
static void DrawFarmWorld(const GameState& s,Vector2 camera) {
    if(!s.life.farm)return;Vector2 p=WorldToScreen({s.life.x,s.life.y},camera);
    if(p.x<kViewport.x-180 || p.x>kViewport.x+kViewport.width+180 || p.y<kViewport.y-140 || p.y>kViewport.y+kViewport.height+140)return;
    DrawRectangleRounded({p.x-80,p.y-56,160,112},.08f,3,Color{92,100,54,255});
    for(int i=0;i<6;i++) {
        float x=p.x-70+(i%3)*48,y=p.y-45+(i/3)*48;const auto& b=s.life.beds[i];
        DrawRectangle((int)x,(int)y,43,40,b.watered?Color{75,53,35,255}:Color{112,78,45,255});
        for(int row=0;row<4;row++)DrawLine((int)x+3,(int)y+7+row*8,(int)x+39,(int)y+7+row*8,Color{59,43,29,160});
        if(b.crop>=0)for(int j=0;j<4;j++) {
            Vector2 at={x+11+(j%2)*20,y+12+(j/2)*17};float growth=b.growth/tflife::crops[b.crop].seconds;
            DrawEllipse((int)at.x-3,(int)at.y,3+growth*3,2+growth*2,Color{65,120,52,255});
            DrawEllipse((int)at.x+3,(int)at.y-2,3+growth*3,2+growth*2,Color{100,149,63,255});
            if(tflife::Ready(b))DrawCircleV(at,b.crop==0?4:3,b.crop==0?Color{226,198,138,255}:b.crop==1?Color{182,199,83,255}:Color{165,125,184,255});
        }
    }
    for(int i=0;i<6;i++){int x=(int)p.x-80+i*32;DrawRectangle(x,(int)p.y+53,4,15,Color{143,105,63,255});}
    DrawLineEx({p.x-80,p.y+58},{p.x+80,p.y+58},3,Color{151,114,69,255});
    DrawWorldLabel("Home farm",{p.x,p.y-62},11,Color{235,221,174,255});
}
static void DrawFarmScreen(GameState& s,int screenW,int screenH) {
    DrawUIText("Home farm",20,120,18,kColorHeading);
    DrawUIText(TextFormat("Farming %.1f",s.life.farming),210,124,13,kColorText);
    if(Button({(float)screenW-168,114,148,44},"Back to Home",true)){g_farmOpen=false;return;}
    if(s.housePlotIdx<0){DrawUIText("Buy a house plot first. Your farm will grow beside it.",20,180,14,kColorText);return;}
    if(!s.life.farm) {
        DrawUIText("Build six beds beside your home: 150 gold + 20 wood.",20,180,14,kColorText);
        DrawUIText("Includes turnip, bean and herb seeds. Water crops, then explore while they grow.",20,210,12,kColorText);
        Vector2 site;bool available=FarmSite(s,site);
        if(Button({20,250,300,48},"Start my farm",available && CanAfford(s,150) && s.wood>=20 && HomeLifeQuiet(s)))TryBuyFarm(s);
        if(!available)DrawUIText("No clear ground beside this plot.",20,310,12,kColorText);return;
    }
    bool present=FarmPresent(s),can=present && HomeLifeQuiet(s);int crop=std::clamp(g_farmCrop,0,2);
    for(int k=0;k<3;k++)if(Button({20+k*154.f,164,146,44},TextFormat("%s%s (%d)",g_farmCrop==k?"* ":"",tflife::crops[k].name,s.life.seeds[k]),true))g_farmCrop=k;
    if(Button({500,164,138,44},TextFormat("5 seeds / %dg",5*tflife::crops[crop].seedPrice),HomeLifeQuiet(s) && CanAfford(s,5*tflife::crops[crop].seedPrice)))TryBuySeeds(s,crop);
    if(Button({648,164,140,44},TextFormat("Ship (%d)",s.life.produce[crop]),s.life.produce[crop]>0 && HomeLifeQuiet(s)))TryUseProduce(s,crop,true);
    if(Button({798,164,142,44},crop==2?"Prepare herbs":"Eat vegetables",s.life.produce[crop]>0 && HomeLifeQuiet(s)))TryUseProduce(s,crop,false);
    float width=(screenW-60)/2.f;
    for(int i=0;i<6;i++) {
        Rectangle r={20+(i%2)*(width+20),220+(i/2)*100.f,width,92};auto& b=s.life.beds[i];
        DrawRectangleRounded(r,.08f,4,Color{51,62,43,255});
        std::string title=b.crop<0?"Empty bed":std::string(tflife::crops[b.crop].name);
        DrawUIText(TextFormat("Bed %d - %s",i+1,title.c_str()),(int)r.x+12,(int)r.y+10,14,Color{239,226,191,255});
        std::string status=b.crop<0?"Choose seeds, then plant":tflife::Ready(b)?"Ready to harvest":!b.watered?"Needs watering":TextFormat("Growing - %ds",(int)ceilf(tflife::crops[b.crop].seconds-b.growth));
        DrawUIText(status.c_str(),(int)r.x+12,(int)r.y+37,12,Color{193,201,164,255});
        const char* action=b.crop<0?"Plant":tflife::Ready(b)?"Harvest":b.watered?"Growing":"Water";
        bool enabled=can && (b.crop<0?s.life.seeds[crop]>0:tflife::Ready(b)||!b.watered);
        if(Button({r.x+r.width-135,r.y+44,123,44},action,enabled))TryFarmBed(s,i,crop);
    }
    if(!present)DrawUIText("Visit your farm in the wilderness to plant, water and harvest.",20,210,10,kColorText);
    (void)screenH;
}
