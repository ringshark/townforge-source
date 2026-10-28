// Separate review binary; scene controls are absent from the shipping build.
#define main TFGameMain
#include "../main.cpp"
#undef main
extern "C" EMSCRIPTEN_KEEPALIVE void TF_ReviewScene(int scene,int detail) {
    g_state=GameState{};auto& s=g_state;s.hp=s.maxHp=10000;
    s.town3DView=s.wild3DView=s.hunt3DView=s.interior3DView=false;
    g_openWorld2D=true;s.screen=Screen::Wilderness;s.exploreMenuOpen=false;
    s.selectedTown=scene==0?detail:0;
    if(scene==0) {
        auto nodes=ActiveTownNodes(detail);auto n=nodes.begin();
        s.wildernessPlayerPos=OpenTownWorld(detail,n->pos);s.wildernessPlayerPos.y+=110;
    } else if(scene==1) {
        s.screen=Screen::Hunt;s.selectedDungeon=detail;s.dungeonPlayerPos={1800,2600};
    } else {
        EnterInterior(s,kInteriorRooms[detail].key);
        s.interior3DView=false;s.interiorPlayerPos={260,350};
    }
}
int main(){return TFGameMain();}
