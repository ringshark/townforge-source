// Exercise the actual world transforms, terrain clearance, gates, and shop return.
#define main TownForgeApplicationMain
#define JS_CloudOpen() ((void)0)
#include "../main.cpp"
#undef main
#include <cassert>
int main() {
    g_openWorld2D=true;
    GameState s;
    for(int ti=0;ti<4;ti++) {
        Vector2 f=TownGateFacing(ti),gate=kTownGates[ti].wildernessPos;
        printf("town %d facing %.3f %.3f\n",ti,f.x,f.y);
        for(const auto& node:ActiveTownNodes(ti)) {
            Vector2 w=OpenTownWorld(ti,node.pos),local=OpenTownLocal(ti,w);
            assert(Dist(local,node.pos)<.01f);
            assert(OpenTownAt(w)==ti);
            assert(w.x>kPlayerEdgeMargin && w.y>kPlayerEdgeMargin);
            assert(w.x<kWildernessWorldSize-kPlayerEdgeMargin && w.y<kWildernessWorldSize-kPlayerEdgeMargin);
            assert(!WildBlocked(w));
            GameState townState;townState.screen=Screen::Wilderness;
            townState.wildernessPlayerPos=w;townState.wild3DView=false;
            assert(InOutdoorTown(townState));
            townState.wild3DView=true;assert(!InOutdoorTown(townState));
        }
        // Walk both directions through the actual arch and terrain, at frame-sized steps.
        for(int dir:{-1,1}) {
            Vector2 p=GateWorld({0,dir==1?-70.f:70.f},gate,f);
            for(int k=0;k<140;k++) {
                Vector2 prev=p;p.x+=f.x*dir;p.y+=f.y*dir;
                GateCollide(p,gate,f);TownCompoundCollide(p,ti,TownCompoundDepth(s,ti));
                if(!WildBlocked(prev))WildTerrainResolve(p,prev);
                assert(Dist(p,prev)>.5f && Dist(p,prev)<1.1f);
            }
            assert((GateLocal(p,gate,f).y>0)==(dir==1));
        }
    }
    // Every town is a square compound that faces north, east, south or west, with open ground behind its walls.
    for(int ti=0;ti<4;ti++) {
        Vector2 f=TownGateFacing(ti),gate=kTownGates[ti].wildernessPos;
        assert((fabsf(f.x)==1.f && f.y==0.f) || (fabsf(f.y)==1.f && f.x==0.f));
        int blocked=0,total=0;
        for(float lz=-40;lz>-OpenTownDepth(ti);lz-=40)for(float lx=-280;lx<=280;lx+=40) {
            Vector2 w=GateWorld({lx,lz},gate,f);total++;
            if(WildTerrainAt(w.x,w.y)&(kWTWater|kWTRidge))blocked++;
        }
        assert(OpenTownDepth(ti)>=250.f);
        assert(blocked*5<total*2); // under 40% of the yard is water or rock (Saltmere stands on a harbour deck)
    }
    // No road runs through a town's yard: the stretches inside were led round the walls to the gate.
    for(const auto& road:OpenRoadNetwork())
        for(const Vector2& p:road)
            for(int ti=0;ti<4;ti++) {
                Vector2 l=GateLocal(p,kTownGates[ti].wildernessPos,TownGateFacing(ti));
                bool yard=fabsf(l.x)<kCompoundHalfW-20.f && l.y<-30.f && l.y>-OpenTownDepth(ti)+20.f;
                assert(!yard);
            }
    // The archway is wide enough for the hero (radius 17) and both towers are solid.
    for(int ti=0;ti<4;ti++) {
        Vector2 f=TownGateFacing(ti),gate=kTownGates[ti].wildernessPos;
        for(float x:{-30.f,0.f,30.f}) {
            Vector2 p=GateWorld({x,60.f},gate,f);
            for(int k=0;k<130;k++){Vector2 prev=p;p=GateWorld({x,60.f-k},gate,f);GateCollide(p,gate,f);assert(Dist(p,GateWorld({x,60.f-k},gate,f))<.5f);(void)prev;}
        }
        Vector2 tower=GateWorld({76.f,-4.f},gate,f),p=GateWorld({80.f,0.f},gate,f);GateCollide(p,gate,f);assert(Dist(p,tower)>=42.f);
        Vector2 wall=GateWorld({160.f,-6.f},gate,f),q=wall;GateCollide(q,gate,f);assert(Dist(q,wall)>10.f);
    }
    // 3D terrain behavior remains active outside the continuous 2D view.
    g_openWorld2D=false;
    assert(TownCompoundDepth(s,0)==TownCompoundBaseDepth(0));
    puts("PASS all four towns: coordinate round trips, in-bounds buildings, walkable terrain and bidirectional gates");
}
