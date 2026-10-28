#pragma once
static bool DungeonMapAllowed(const GameState& s){return s.selectedDungeon && !s.dungeonEngaged && s.dungeonExtraAttackers.empty() && !s.combat;}
static void DungeonMapPaint(const GameState& s,Rectangle area,bool full){
    int di=*s.selectedDungeon;float scale=std::min(area.width,area.height)/kDungeonWorldSize;
    Vector2 origin={area.x+(area.width-kDungeonWorldSize*scale)/2,area.y+(area.height-kDungeonWorldSize*scale)/2};
    auto project=[&](Vector2 p){return Vector2{origin.x+p.x*scale,origin.y+p.y*scale};};
    DrawRectangleRounded(area,.04f,4,Color{23,29,33,245});
    for(auto r:kDungeonRoomLayouts[di]){
        Vector2 p=project({r.x,r.y});Rectangle room={p.x,p.y,r.width*scale,r.height*scale};
        DrawRectangleRec(room,Color{100,111,111,255});DrawRectangleLinesEx(room,full?1.f:.5f,Color{151,155,139,255});
    }
    for(int slot=0;slot<kDungeonSlotCount;slot++)if(s.dungeonSpawnRespawn[di][slot]<=0){
        Vector2 p=project(DungeonMonsterLivePos(di,slot,s.worldTime));
        DrawCircleV(p,slot==kDungeonBossSlot?(full?5:3):(full?2.5f:1.3f),slot==kDungeonBossSlot?Color{194,142,207,255}:Color{216,132,111,255});
    }
    Vector2 exit=project({1800,2600}),hero=project(s.dungeonPlayerPos);
    DrawRectangleRec({exit.x-3,exit.y-3,6,6},Color{145,210,158,255});
    DrawCircleV(hero,full?5:3,Color{252,224,151,255});DrawCircleLinesV(hero,full?8:5,Color{252,224,151,255});
}
static void DrawDungeonOverview(GameState& s){
    DrawRectangle(0,0,960,540,Color{21,27,31,255});DrawUIText(kDungeons[*s.selectedDungeon].name.c_str(),24,22,24,Color{241,224,184,255});
    DrawUIText("Entire layout revealed",24,56,13,Color{171,194,183,255});
    if(Button({804,20,132,48},"Back [M]",true))g_dungeonMapOpen=false;
    DungeonMapPaint(s,{20,90,680,426},true);
    DrawUIText("DUNGEON GUIDE",724,106,17,Color{231,203,145,255});
    const char* lines[]={"Gold: your position","Green: entrance / exit","Red: roaming monsters","Purple: boss chamber","","Explore the whole layout.","Fight at your own pace.","Clear rooms to unlock","the dungeon's boss."};
    for(int i=0;i<9;i++)DrawUIText(lines[i],724,148+i*27,13,Color{204,210,197,255});
    DrawUIText(TextFormat("Progress: %d / %d XP",s.dungeonXP[*s.selectedDungeon],kDungeons[*s.selectedDungeon].bossUnlockXp),724,424,13,Color{231,203,145,255});
}
static void DrawDungeonMinimap(GameState& s,int width){
    Rectangle map={(float)width-156,20,136,136};UIRegister(map);DungeonMapPaint(s,map,false);
    if(Button({(float)width-194,164,174,40},"Dungeon map [M]",DungeonMapAllowed(s)))g_dungeonMapOpen=true;
}
