#pragma once
// Shared geometry for the connected 2D world. No save IDs or owned plots move.
namespace tfdesign {
struct Lane {Vector2 a,b;float width;};
static std::vector<std::vector<Vector2>> trails;
static bool trailsBuilt=false;
static bool ClearTrail(Vector2 p) {
    if(p.x<24 || p.y<24 || p.x>kWildernessWorldSize-24 || p.y>kWildernessWorldSize-24 || OpenTownAt(p)>=0)return false;
    for(Vector2 d:std::array<Vector2,5>{{{0,0},{16,0},{-16,0},{0,16},{0,-16}}})
        if(WildTerrainAt(p.x+d.x,p.y+d.y)&(kWTWater|kWTRidge))return false;
    if(Dist(p,kOrcFortPos)<kOrcFortRadius+40 || Dist(p,kWyrmLair)<kWyrmLairR+40)return false;
    for(const auto& camp:kRivalCampSpots)if(Dist(p,camp)<kCampRadius+40)return false;
    for(const auto& h:kHousePlots)if(Dist(p,h.pos)<h.cells*kHouseCellSize*.5f+28)return false;
    return true;
}
static bool ClearLine(Vector2 a,Vector2 b) {
    int n=std::max(1,(int)ceilf(Dist(a,b)/8));
    for(int i=0;i<=n;i++)if(!ClearTrail({a.x+(b.x-a.x)*i/n,a.y+(b.y-a.y)*i/n}))return false;
    return true;
}
static void BuildTrails() {
    if(trailsBuilt)return;trailsBuilt=true;WildTerrainEnsure();
    constexpr int step=16,N=360;std::vector<int> parent(N*N,-2),queue;queue.reserve(N*N);
    auto index=[](Vector2 p){return std::clamp((int)(p.y/step),0,N-1)*N+std::clamp((int)(p.x/step),0,N-1);};
    auto point=[](int i){return Vector2{(i%N+.5f)*step,(i/N+.5f)*step};};
    for(const auto& road:g_wtRoads)for(auto p:road){int i=index(p);if(parent[i]==-2 && ClearTrail(point(i))){parent[i]=-1;queue.push_back(i);}}
    for(size_t head=0;head<queue.size();++head){int i=queue[head],x=i%N,y=i/N;
        for(auto d:std::array<Vector2,4>{{{1,0},{-1,0},{0,1},{0,-1}}}){
            int nx=x+(int)d.x,ny=y+(int)d.y;if(nx<0 || ny<0 || nx>=N || ny>=N)continue;
            int j=ny*N+nx;if(parent[j]!=-2 || !ClearTrail(point(j)) || !ClearTrail({(point(i).x+point(j).x)*.5f,(point(i).y+point(j).y)*.5f}))continue;parent[j]=i;queue.push_back(j);
        }
    }
    std::vector<Vector2> destinations;
    for(int i=0;i<(int)kWildernessDungeonEntrances.size();++i)destinations.push_back(EntranceStepOut(i));
    for(const auto& n:kWildernessGatherNodes)destinations.push_back(n.pos);
    for(const auto& h:kHousePlots)destinations.push_back({h.pos.x,h.pos.y+h.cells*kHouseCellSize*.5f+64});
    for(const auto& s:kShrines)destinations.push_back(s.pos);
    for(auto destination:destinations){int best=-1;float distance=120;
        int at=index(destination),x=at%N,y=at/N;
        for(int dy=-7;dy<=7;dy++)for(int dx=-7;dx<=7;dx++){
            int nx=x+dx,ny=y+dy;if(nx<0 || ny<0 || nx>=N || ny>=N)continue;
            int j=ny*N+nx;float d=Dist(point(j),destination);if(parent[j]!=-2 && d<distance){distance=d;best=j;}
        }
        if(best<0)continue;std::vector<Vector2> raw;
        for(int i=best;i>=0 && raw.size()<N*N;i=parent[i])raw.push_back(point(i));
        if(raw.size()<3)continue;std::vector<Vector2> path{raw.front()};
        for(size_t i=0;i+1<raw.size();){size_t j=std::min(raw.size()-1,i+24);while(j>i+1 && !ClearLine(raw[i],raw[j]))--j;path.push_back(raw[j]);i=j;}
        trails.push_back(std::move(path));
    }
}
static void DrawRoads(Vector2 camera) {
    BuildTrails();
    auto stroke=[&](const std::vector<Vector2>& path,float width,Color color){
        for(size_t i=1;i<path.size();i++){
            auto a=WorldToScreen(path[i-1],camera),b=WorldToScreen(path[i],camera);
            if(std::max(a.x,b.x)<-60 || std::min(a.x,b.x)>kViewport.width+60 || std::max(a.y,b.y)<-60 || std::min(a.y,b.y)>kViewport.height+60)continue;
            DrawLineEx(a,b,width,color);DrawCircleV(a,width*.5f,color);DrawCircleV(b,width*.5f,color);
        }
    };
    for(int pass=0;pass<3;pass++){
        for(const auto& path:trails)stroke(path,pass==0?22:pass==1?16:10,pass==0?Color{100,111,77,255}:pass==1?Color{139,132,95,255}:Color{151,140,103,255});
        for(size_t i=0;i<g_wtRoads.size();i++)stroke(g_wtRoads[i],(i==0?38:28)-(pass==0?0:pass==1?6:14),pass==0?Color{116,112,83,255}:pass==1?Color{161,148,111,255}:Color{174,158,119,255});
    }
}
static std::vector<Lane> TownLanes(int ti) {
    std::vector<Lane> out;auto add=[&](float x,float y,float X,float Y,float w){out.push_back({OpenTownWorld(ti,TS(x,y)),OpenTownWorld(ti,TS(X,Y)),w});};
    if(ti==1 || ti==3){auto world=[&](Vector2 p){return GateWorld(p,kTownGates[ti].wildernessPos,TownGateFacing(ti));};
        out.push_back({world({-280,-180}),world({280,-180}),36});
        out.push_back({world({-280,-35}),world({280,-35}),30});
        out.push_back({world({-280,-180}),world({-280,-35}),30});
        out.push_back({world({280,-180}),world({280,-35}),30});
        out.push_back({world({0,0}),world({0,-180}),36});return out;
    }
    add(350,200,350,950,30);add(650,200,650,950,30);
    for(float y:{200.f,450.f,650.f,950.f})add(350,y,650,y,y==650?38:30);
    out.push_back({kTownGates[ti].wildernessPos,OpenTownWorld(ti,TS(500,950)),38});
    return out;
}
static Vector2 NearestOn(Vector2 p,Vector2 a,Vector2 b) {
    Vector2 d={b.x-a.x,b.y-a.y};float n=d.x*d.x+d.y*d.y;
    float t=n?std::clamp(((p.x-a.x)*d.x+(p.y-a.y)*d.y)/n,0.f,1.f):0;
    return {a.x+d.x*t,a.y+d.y*t};
}
static void DrawTownStreets(int ti,Vector2 camera) {
    auto lanes=TownLanes(ti);Color stone=ti==2?Color{175,190,193,255}:ti==1?Color{172,159,132,255}:Color{173,165,141,255};
    // Doorsteps face down-screen, just like the existing building art.
    for(const auto& node:ActiveTownNodes(ti)){
        Vector2 door=OpenTownWorld(ti,node.pos);door.y+=24;Vector2 best{};float distance=1e9f;
        for(const auto& lane:lanes){auto q=NearestOn(door,lane.a,lane.b);float d=Dist(door,q);if(d<distance){best=q;distance=d;}}
        lanes.push_back({door,best,22});
    }
    for(int pass=0;pass<2;pass++)for(const auto& lane:lanes){auto a=WorldToScreen(lane.a,camera),b=WorldToScreen(lane.b,camera);float width=lane.width+(pass?0:5);Color c=pass?stone:Color{109,116,98,255};DrawLineEx(a,b,width,c);DrawCircleV(a,width*.5f,c);DrawCircleV(b,width*.5f,c);}
    // Small staggered stone joints add texture after the entire street union
    // has been painted, so overlapping lane caps never produce dark scallops.
    for(const auto& lane:lanes){float length=Dist(lane.a,lane.b);if(length<1)continue;
        Vector2 t={(lane.b.x-lane.a.x)/length,(lane.b.y-lane.a.y)/length},n={-t.y,t.x};
        for(float d=12;d<length-8;d+=18){Vector2 p=WorldToScreen({lane.a.x+t.x*d,lane.a.y+t.y*d},camera);float half=lane.width*.5f-3;
            DrawLineEx({p.x-n.x*half,p.y-n.y*half},{p.x+n.x*half,p.y+n.y*half},.6f,Color{135,138,119,150});}
    }
    Vector2 square=WorldToScreen((ti==1 || ti==3)?GateWorld({0,-110},kTownGates[ti].wildernessPos,TownGateFacing(ti)):OpenTownWorld(ti,TS(500,600)),camera);
    DrawRectangleRounded({square.x-52,square.y-40,104,80},.12f,4,stone);
    // A low fountain and planted corners give the square a recognizable center.
    tf2dart::Shadow({square.x,square.y+9},19,7,.2f);
    DrawEllipse((int)square.x,(int)square.y,22,13,Color{102,111,107,255});
    DrawEllipse((int)square.x,(int)square.y-3,19,10,Color{190,190,170,255});
    DrawEllipse((int)square.x,(int)square.y-4,14,7,Color{84,133,143,255});
    DrawRectangle((int)square.x-3,(int)square.y-20,6,15,Color{185,187,168,255});
    for(int dx:{-1,1})for(int dy:{-1,1}){Vector2 p={square.x+dx*44,square.y+dy*33};DrawEllipse((int)p.x,(int)p.y,9,6,Color{77,102,64,255});DrawCircleV({p.x-3,p.y-2},3,ti==2?Color{216,232,231,255}:Color{137,160,104,255});}
}
// Each mouth has a footprint, approach and silhouette; the interaction point is
// the production EntranceDoorPoint, shared with collision and dungeon travel.
static void DrawDungeon(int index,const GameState& s,Vector2 camera,bool near) {
    const auto& entrance=kWildernessDungeonEntrances[index];int kind=entrance.dungeonIdx;
    Vector2 door=WorldToScreen(EntranceDoorPoint(index),camera);
    if(!OpenVisible(door,230))return;
    const Texture2D& atlas=tf2dart::entranceAtlas;
    const float size=kind==2?206.f:190.f;
    if(atlas.id) {
        float cw=atlas.width/3.f,ch=atlas.height/2.f;
        Rectangle source={(kind%3)*cw,(kind/3)*ch,cw,ch};
        DrawTexturePro(atlas,source,{door.x-size*.5f,door.y-size*.625f,size,size},{0,0},0,WHITE);
    }else {
        tf2dart::Shadow({door.x,door.y-15},70,30,.22f);
        DrawEllipse((int)door.x,(int)door.y-35,74,60,kind==4?Color{169,199,204,255}:Color{112,119,100,255});
        DrawRectangleRounded({door.x-24,door.y-53,48,56},.25f,4,Color{25,31,31,255});
        for(int j=0;j<4;j++)DrawRectangle((int)door.x-28,(int)door.y+j*6,56,4,Color{151,151,131,255});
    }
    if(near){DrawEllipseLines((int)door.x,(int)door.y+15,31,12,Color{235,204,134,255});DrawWorldLabel(kDungeons[kind].name,{door.x,door.y+72},14,Color{244,224,180,255});}
}
}
