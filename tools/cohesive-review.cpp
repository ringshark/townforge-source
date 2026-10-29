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
        s.interior3DView=false;if(s.interiorKey=="wildhouse")s.interiorPlayerPos={260,350};
    }
}
extern "C" EMSCRIPTEN_KEEPALIVE const char* TF_ReviewMatte(int town,int index) {
    const char* frost[]={"bank","smith","furtrader","healer","provisioner"};
    const char* crag[]={"bank","smith","minersguild","healer","provisioner"};
    auto* tex=town==3?FindCragmoorBuildingTexture(crag[index]):FindFrostmereBuildingTexture(frost[index]);
    if(!tex)return "{}";Image image=LoadImageFromTexture(*tex);if(!image.data)return "{}";
    ImageFormat(&image,PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);auto* px=(Color*)image.data;
    int inset=px[image.width+1].a,visible=0;for(int i=0;i<image.width*image.height;i++)visible+=px[i].a>240;
    int width=image.width,height=image.height;UnloadImage(image);
    return TextFormat("{\"alpha\":%d,\"visible\":%d,\"pixels\":%d}",inset,visible,width*height);
}
extern "C" EMSCRIPTEN_KEEPALIVE const char* TF_ReviewRoomSize(){return TextFormat("{\"width\":%.0f,\"height\":%.0f}",g_intW,g_intH);}
int main(){return TFGameMain();}
