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
    // 3D terrain behavior remains active outside the continuous 2D view.
    g_openWorld2D=false;
    assert(TownCompoundDepth(s,0)==TownCompoundBaseDepth(0));
    puts("PASS all four towns: coordinate round trips, in-bounds buildings, walkable terrain and bidirectional gates");
}
