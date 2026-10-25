#pragma once
#include "atlas_geometry.h"
namespace tfatlas {
static constexpr Color kInk={225,225,206,255},kGold={232,194,116,255},kMuted={159,178,172,255};
struct Marker {Vector2 pos;std::string name,detail;int group,kind,id;Color color;};
static View view;
static bool initialized=false,dragging=false,moved=false;
static Vector2 press{},last{};
static int filter=0;
static std::optional<Marker> selected,waypoint;
static std::vector<Marker> Markers(const GameState& s) {
    std::vector<Marker> out;
    for(int i=0;i<4;i++)out.push_back({kTownGates[i].wildernessPos,kTownGates[i].townName,"Town / shops and services",0,0,i,kGold});
    for(int i=0;i<(int)kWildernessDungeonEntrances.size();i++) {
        const auto& e=kWildernessDungeonEntrances[i];out.push_back({e.pos,kDungeons[e.dungeonIdx].name,"Dungeon entrance",0,1,100+i,Color{201,126,151,255}});
    }
    out.push_back({kOrcFortPos,"Grimtusk Hold","Orc fortress / dangerous territory",0,2,200,Color{220,112,87,255}});
    out.push_back({kWyrmLair,"Cinder Caldera",s.wyrmRespawnT<=0?"Vyrathax is awake":"The resting place of Vyrathax",0,2,201,Color{231,142,78,255}});
    out.push_back({kRivalCampSpots[s.rivalCampIdx],"Murder Inc. camp","Roaming camp / dangerous territory",0,2,202,Color{220,112,87,255}});
    out.push_back({kFieldsOfSorrow,"Fields of Sorrow","Haunted battlefield",0,2,203,Color{184,149,197,255}});
    if(s.notoriety>1 || s.refugeKnown)out.push_back({kOutlawRefuge,"Outlaw Refuge","Hidden refuge / outlaw services",0,2,204,Color{184,149,197,255}});
    for(int i=0;i<(int)kShrines.size();i++)out.push_back({kShrines[i].pos,std::string("Shrine of ")+kShrines[i].name,"Shrine / resurrection",1,3,300+i,Color{141,211,217,255}});
    for(int i=0;i<(int)kWildernessGatherNodes.size();i++) {
        const auto& n=kWildernessGatherNodes[i];
        std::string name=n.resource=="wood"?"Timber grove":n.resource=="fish"?"Fishing water":n.resource=="ice"?"Ice crystals":n.resource=="richore"?"Rich ore vein":"Ore deposits";
        out.push_back({n.pos,name,"Gathering / "+n.resource,1,4,400+i,Color{150,202,143,255}});
    }
    for(int i=0;i<(int)kHousePlots.size();i++) {
        const auto& h=kHousePlots[i];bool own=i==s.housePlotIdx;
        out.push_back({h.pos,own?"Your home":h.name,own?"Your house and settlement":TextFormat("Housing plot / %d gold",h.price),2,own?5:6,500+i,own?Color{133,219,155,255}:Color{191,174,135,255}});
    }
    if(s.life.farm)out.push_back({{s.life.x,s.life.y},"Your homestead","Farm / crops, animals and family",2,5,600,Color{133,219,155,255}});
    return out;
}
static void Text(const std::string& text,int x,int y,int size,Color color,int width=220) {
    std::string fit=text;while(MeasureUIText(fit.c_str(),size)>width && fit.size()>3)fit.pop_back();
    if(fit.size()<text.size()){while(MeasureUIText((fit+"...").c_str(),size)>width && fit.size()>3)fit.pop_back();fit+="...";}
    DrawUIText(fit.c_str(),x,y,size,color);
}
static bool Visible(Vector2 p,float pad=0){return p.x>=kCanvas.x+pad && p.y>=kCanvas.y+pad && p.x<=kCanvas.x+kCanvas.width-pad && p.y<=kCanvas.y+kCanvas.height-pad;}
static void Icon(const Marker& m,Vector2 p,float size=7) {
    DrawCircleV(p,size+3,Color{24,34,33,230});
    if(m.kind==0)MapIconTown(p,size);
    else if(m.kind==1)MapIconDungeon(p,size,m.color);
    else if(m.kind==3){DrawPoly(p,4,size,45,m.color);DrawPolyLines(p,4,size,45,Color{36,66,70,255});}
    else if(m.kind==5 || m.kind==6){DrawRectangleRec({p.x-size,p.y-size,2*size,2*size},m.color);DrawRectangleLinesEx({p.x-size,p.y-size,2*size,2*size},1,Color{38,68,45,255});}
    else {DrawCircleV(p,size,m.color);DrawCircleV(p,size*.38f,Color{40,52,40,255});}
}
static void Route(Vector2 a,Vector2 b,Color color) {
    float length=Dist(a,b);if(length<1)return;
    for(float d=0;d<length;d+=14){float e=std::min(d+7,length);DrawLineEx({a.x+(b.x-a.x)*d/length,a.y+(b.y-a.y)*d/length},{a.x+(b.x-a.x)*e/length,a.y+(b.y-a.y)*e/length},2,color);}
}
static void Draw(GameState& s) {
    bool bypass=g_uiShieldBypass;g_uiShieldBypass=true;
    bool opened=g_worldMapJustOpened;g_worldMapJustOpened=false;
    if(!initialized){view.center={kWildernessWorldSize/2,kWildernessWorldSize/2};initialized=true;}
    if(opened){dragging=false;moved=false;}
    auto markers=Markers(s);
    if(selected && selected->id!=9999)for(const auto& m:markers)if(m.id==selected->id)*selected=m;
    view.Clamp(kWildernessWorldSize);
    DrawRectangle(0,0,960,540,Color{17,27,31,255});
    DrawRectangleGradientV(0,0,960,72,Color{39,55,57,255},Color{23,35,39,255});
    DrawUIText("WILDERNESS ATLAS",22,17,22,kGold);
    Text(std::string(RegionName(RegionAt(s.wildernessPlayerPos)))+" / choose your next destination",22,46,12,kMuted,440);
    auto button=[&](Rectangle r,const std::string& label,bool enabled=true){
        bool hit=Button(r,"",enabled && !opened);
        bool hover=enabled && CheckCollisionPointRec(GetMousePosition(),r);
        DrawRectangleRounded(r,.12f,4,hover?Color{61,79,79,255}:Color{38,55,59,255});
        DrawRectangleLinesEx(r,1,enabled?Color{112,130,119,255}:Color{54,72,74,255});
        int w=MeasureUIText(label.c_str(),14);DrawUIText(label.c_str(),(int)(r.x+(r.width-w)/2),(int)(r.y+(r.height-14)/2),14,enabled?kInk:kMuted);
        return hit;
    };
    if(button({476,16,76,44},"World")){view.zoom=1;view.center={kWildernessWorldSize/2,kWildernessWorldSize/2};}
    if(button({560,16,76,44},"You")){view.zoom=2.2f;view.center=s.wildernessPlayerPos;}
    if(button({644,16,44,44},"-"))view.zoom/=1.35f;
    if(button({696,16,44,44},"+"))view.zoom*=1.35f;
    Text(TextFormat("%.0f%%",view.zoom*100),754,31,15,kInk,80);
    if(button({842,16,98,44},"Close") || IsKeyPressed(KEY_ESCAPE)){s.worldMapOpen=false;dragging=false;g_uiShieldBypass=bypass;return;}
    const char* filters[]={"Places","Supplies","Homes"};
    for(int i=0;i<3;i++) {
        Rectangle r={714.f+i*78,76,70,40};
        if(button(r,filters[i])){filter=i;selected.reset();}
        if(filter==i)DrawRectangleRec({r.x+8,r.y+r.height-3,r.width-16,3},kGold);
    }
    view.Clamp(kWildernessWorldSize);
    Vector2 mouse=GetMousePosition();
    if(!opened && Visible(mouse) && IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {dragging=true;moved=false;press=last=mouse;g_uiClickTaken=g_uiGestureOwned=true;}
    if(dragging && IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        if(Dist(mouse,press)>7)moved=true;
        if(moved){float scale=view.Scale(kWildernessWorldSize);view.center.x-=(mouse.x-last.x)/scale;view.center.y-=(mouse.y-last.y)/scale;view.Clamp(kWildernessWorldSize);}
        last=mouse;
    }
    if(dragging && !IsMouseButtonDown(MOUSE_BUTTON_LEFT)) {
        if(!moved && Visible(mouse)) {
            float best=24;std::optional<Marker> hit;
            for(const auto& m:markers)if(m.group==filter){float d=Dist(view.Project(m.pos,kWildernessWorldSize),mouse);if(d<best){best=d;hit=m;}}
            if(hit)selected=hit;
            else {Vector2 w=view.Unproject(mouse,kWildernessWorldSize);if(w.x>=0 && w.y>=0 && w.x<=kWildernessWorldSize && w.y<=kWildernessWorldSize)selected=Marker{w,"Map waypoint",RegionName(RegionAt(w)),filter,7,9999,kGold};}
        }
        dragging=false;g_uiClickTaken=g_uiGestureOwned=true;
    }
    if(Visible(mouse)){float wheel=GetMouseWheelMove();if(wheel!=0)view.ZoomAt(wheel>0?1.25f:.8f,mouse,kWildernessWorldSize);}
    DrawRectangleRec(kCanvas,Color{25,43,49,255});
    UIBeginScissorMode((int)kCanvas.x,(int)kCanvas.y,(int)kCanvas.width,(int)kCanvas.height);
    Vector2 origin=view.Project({0,0},kWildernessWorldSize);float scale=view.Scale(kWildernessWorldSize),size=kWildernessWorldSize*scale;
    DrawTexturePro(g_wildMapTex,{0,0,(float)g_wildMapTex.width,(float)g_wildMapTex.height},{origin.x,origin.y,size,size},{0,0},0,WHITE);
    // Quiet coordinate grid, anchored to the world rather than the viewport.
    for(float v=0;v<=kWildernessWorldSize;v+=720) {
        Vector2 a=view.Project({v,0},kWildernessWorldSize),b=view.Project({v,kWildernessWorldSize},kWildernessWorldSize);DrawLineEx(a,b,.7f,Color{33,49,45,30});
        a=view.Project({0,v},kWildernessWorldSize);b=view.Project({kWildernessWorldSize,v},kWildernessWorldSize);DrawLineEx(a,b,.7f,Color{33,49,45,30});
    }
    const struct {const char* name;Vector2 pos;} regions[]={{"FROSTWASTES",WP(1600,330)},{"WHISPERWOOD",WP(1200,1300)},{"STONEPEAKS",WP(250,2000)},{"SALT COAST",WP(2600,2100)}};
    for(const auto& region:regions){Vector2 p=view.Project(region.pos,kWildernessWorldSize);if(Visible(p,20)){int w=MeasureUIText(region.name,14);DrawUIText(region.name,(int)p.x-w/2,(int)p.y,14,Color{28,48,45,150});}}
    if(waypoint) {
        Vector2 p=view.Project(waypoint->pos,kWildernessWorldSize);Route(view.Project(s.wildernessPlayerPos,kWildernessWorldSize),p,Color{255,222,128,225});
        DrawCircleLinesV(p,14,kGold);DrawLineEx({p.x-7,p.y},{p.x+7,p.y},2,kGold);DrawLineEx({p.x,p.y-7},{p.x,p.y+7},2,kGold);
    }
    std::vector<Rectangle> labels;
    for(const auto& m:markers)if(m.group==filter) {
        Vector2 p=view.Project(m.pos,kWildernessWorldSize);if(!Visible(p,9))continue;
        bool active=selected && selected->id==m.id;
        if(active){DrawCircleV(p,20,Fade(kGold,.20f));DrawCircleLinesV(p,18,kGold);}
        Icon(m,p,active?9:7);
        if(m.kind==0 || active || (view.zoom>=2 && m.kind==1)) {
            int w=MeasureUIText(m.name.c_str(),14);Rectangle r={std::clamp(p.x-w/2-5,kCanvas.x+3,kCanvas.x+kCanvas.width-w-13),p.y+13,(float)w+10,22};
            bool overlap=false;for(auto other:labels)if(CheckCollisionRecs(r,other)){overlap=true;break;}
            if((!overlap || active) && r.y+r.height<kCanvas.y+kCanvas.height){DrawRectangleRounded(r,.2f,3,Color{26,37,38,235});DrawUIText(m.name.c_str(),(int)r.x+5,(int)r.y+3,14,active?kGold:kInk);labels.push_back(r);}
        }
    }
    MapIconPlayer(view.Project(s.wildernessPlayerPos,kWildernessWorldSize),s.playerFacing,8);
    UIEndScissorMode();
    DrawRectangleLinesEx(kCanvas,1,Color{106,128,119,255});
    DrawUIText("N",(int)kCanvas.x+12,(int)kCanvas.y+12,18,kInk);
    DrawTriangle({kCanvas.x+19,kCanvas.y+35},{kCanvas.x+13,kCanvas.y+45},{kCanvas.x+25,kCanvas.y+45},kInk);
    DrawUIText("Drag to explore / tap a place or ground to select",22,500,12,kMuted);
    float bar=std::min(130.f,500*scale);DrawLineEx({550,500},{550+bar,500},2,kInk);DrawLineEx({550,496},{550,504},2,kInk);DrawLineEx({550+bar,496},{550+bar,504},2,kInk);Text(TextFormat("%d paces",(int)(bar/scale/10)),550,511,10,kMuted,145);
    DrawRectangleRounded({714,124,226,152},.05f,4,Color{29,43,46,255});
    Text(selected?selected->name:"Choose a destination",726,136,16,kInk,202);
    Text(selected?selected->detail:"Tap a marker or choose below",726,165,12,kMuted,202);
    if(selected)Text(TextFormat("%s / %d paces",CompassWord(s.wildernessPlayerPos,selected->pos).c_str(),(int)(Dist(s.wildernessPlayerPos,selected->pos)/10)),726,193,16,kGold,202);
    if(button({726,228,202,40},"Set waypoint",selected.has_value()))waypoint=selected;
    DrawUIText("NEAR YOU",718,288,12,kMuted);
    std::vector<Marker> nearby;for(auto m:markers)if(m.group==filter)nearby.push_back(m);
    std::stable_sort(nearby.begin(),nearby.end(),[&](const Marker& a,const Marker& b){return Dist(a.pos,s.wildernessPlayerPos)<Dist(b.pos,s.wildernessPlayerPos);});
    for(int i=0;i<std::min(3,(int)nearby.size());i++) {
        const auto& m=nearby[i];Rectangle r={714,310.f+i*54,226,48};
        if(button(r,"")){selected=m;view.center=m.pos;view.Clamp(kWildernessWorldSize);}
        Text(m.name,724,(int)r.y+6,15,kInk,204);Text(TextFormat("%s / %d paces",CompassWord(s.wildernessPlayerPos,m.pos).c_str(),(int)(Dist(s.wildernessPlayerPos,m.pos)/10)),724,(int)r.y+27,11,kMuted,204);
    }
    if(button({714,476,226,44},waypoint?"Clear waypoint":"No waypoint set",waypoint.has_value()))waypoint.reset();
    g_uiShieldBypass=bypass;
}
static void WaypointHud(const GameState& s) {
    if(!waypoint || s.worldMapOpen || s.exploreMenuOpen || !g_landscapePage)return;
    float d=Dist(s.wildernessPlayerPos,waypoint->pos);
    Rectangle r={198,18,350,42};UIRegister(r);DrawRectangleRounded(r,.15f,4,Color{23,36,38,235});
    Text(waypoint->name,210,24,12,kGold,326);
    Text(d<65?"Arrived":TextFormat("%s / %d paces",CompassWord(s.wildernessPlayerPos,waypoint->pos).c_str(),(int)(d/10)),210,42,11,kInk,326);
}
}
