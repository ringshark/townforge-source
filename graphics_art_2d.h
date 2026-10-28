#pragma once
// Atlas metadata follows the inspected art, including its nonuniform row spacing.
namespace tfgfx {
struct Atlas {Texture2D texture{};std::vector<Rectangle> frames;};
static Atlas hero,north,props,details;
static int lastHeroFrame=-1;static bool lastHeroNorth=false;
static void LoadAtlas(Atlas& atlas,const char* path,int columns,const std::vector<float>& rows) {
    Image image=LoadImage(path);if(!image.data)return;
    ImageAlphaClear(&image,BLANK,.12f);
    atlas.texture=LoadTextureFromImage(image);SetTextureFilter(atlas.texture,TEXTURE_FILTER_BILINEAR);
    Color* pixels=LoadImageColors(image);
    for(size_t row=0;row+1<rows.size();++row)for(int col=0;col<columns;++col){
        int x0=(int)(col*image.width/(float)columns),x1=(int)((col+1)*image.width/(float)columns);
        int y0=(int)(rows[row]*image.height),y1=(int)(rows[row+1]*image.height);
        int left=x1,right=x0,top=y1,bottom=y0;
        if(pixels)for(int y=y0;y<y1;y++)for(int x=x0;x<x1;x++)if(pixels[y*image.width+x].a>48){left=std::min(left,x);right=std::max(right,x+1);top=std::min(top,y);bottom=std::max(bottom,y+1);}
        atlas.frames.push_back(right>left?Rectangle{(float)left,(float)top,(float)(right-left),(float)(bottom-top)}:Rectangle{});
    }
    if(pixels)UnloadImageColors(pixels);UnloadImage(image);
}
static void Load(){
    LoadAtlas(hero,"assets/graphics_pass/hero-actions.png",8,{0,183/1254.f,350/1254.f,517/1254.f,688/1254.f,862/1254.f,1035/1254.f,1});
    LoadAtlas(north,"assets/graphics_pass/hero-north.png",4,{0,.51f,1});
    LoadAtlas(props,"assets/graphics_pass/interior-props.png",6,{0,.193f,.36f,.533f,.684f,.842f,1});
    LoadAtlas(details,"assets/graphics_pass/interior-details.png",6,{0,.25f,.5f,.75f,1});
}
static void Unload(){for(auto* a:{&hero,&north,&props,&details}){if(a->texture.id)UnloadTexture(a->texture);*a={};}}
static void DrawFrame(const Atlas& a,int frame,Vector2 foot,float scale,Color tint=WHITE){
    if(&a==&hero || &a==&north){lastHeroFrame=frame;lastHeroNorth=&a==&north;}
    if(!a.texture.id || frame<0 || frame>=(int)a.frames.size())return;auto r=a.frames[frame];if(r.height<=0)return;
    DrawTexturePro(a.texture,r,{foot.x-r.width*scale*.5f,foot.y-r.height*scale,r.width*scale,r.height*scale},{0,0},0,tint);
}
static int Phase(float elapsed,float duration,int frames){return std::clamp((int)(std::clamp(elapsed/std::max(.001f,duration),0.f,.999f)*frames),0,frames-1);}
static bool HeroAction(const GameState& s,Vector2 pos,int facing,ActorAnim anim,float size,Color tint){
    if(!hero.texture.id)return false;
    int frame=-1;const Atlas* atlas=&hero;
    const float foot=pos.y+size*.48f;
    if(s.playerDeathAnimT>0)frame=(3+facing)*8+2+Phase(1.2f-s.playerDeathAnimT,1.2f,3);
    else if(s.playerHurtT>=0)frame=(3+facing)*8+Phase(s.playerHurtT,.30f,2);
    else if(s.gatheringResource.has_value() && *s.gatheringResource!="fish")frame=(3+facing)*8+5+(int)(s.worldTime*7)%3;
    else if(anim==ActorAnim::Attack || anim==ActorAnim::Cast){
        float left=0;
        if(s.screen==Screen::Wilderness && s.wildEngaged)left=anim==ActorAnim::Attack?s.wildEngaged->swingEffectTimer:s.wildEngaged->castEffectTimer;
        if(s.screen==Screen::Hunt && s.dungeonEngaged)left=anim==ActorAnim::Attack?s.dungeonEngaged->swingEffectTimer:s.dungeonEngaged->castEffectTimer;
        float duration=anim==ActorAnim::Attack?.32f:.35f;int phase=Phase(duration-left,duration,4);
        const Item* weapon=s.equipped.rightHand?&*s.equipped.rightHand:(s.equipped.leftHand?&*s.equipped.leftHand:nullptr);
        if(anim==ActorAnim::Attack && (!weapon || (weapon->category!="Swordsmanship" && weapon->category!="Fencing"))){frame=(3+facing)*8+(phase<2?0:1);DrawFrame(hero,frame,{pos.x,foot},size/139.f,tint);return true;}
        if(facing==3){atlas=&north;frame=(anim==ActorAnim::Cast?4:0)+phase;}
        else frame=facing*8+(anim==ActorAnim::Cast?4:0)+phase;
    }
    if(frame<0)return false;
    // Scale by the standing body's reference height, never each pose's crop:
    // a fallen body must remain short rather than growing to standing height.
    float reference=atlas==&north?290.f:139.f;
    DrawFrame(*atlas,frame,{pos.x,foot},size/reference,tint);return true;
}
static int PropIndex(std::string label,std::string model){
    std::string key=label+" "+model;std::transform(key.begin(),key.end(),key.begin(),[](unsigned char c){return (char)std::tolower(c);});
    const std::pair<const char*,int> detailNames[]={{"candle",36},{"lantern",36},{"bottle",37},{"flask",37},{"salve",37},{"cider",37},{"mortar",38},{"glue",38},{"drying herb",39},{"potted",40},{"coal bucket",41},{"feed bucket",42},{"grain sack",43},{"bread",44},{"cheese",44},{"ingot",45},{"bellows",46},{"dress form",47},{"pelt frame",48},{"pelt",49},{"fur bolt",49},{"dresser",50},{"pillar",51},{"portrait",52},{"crest",52},{"shield display",53},{"weapon rack",53},{"pick rack",53},{"whetstone",54},{"keg",55},{"coin",56},{"banner",57},{"grimoire",58},{"pattern book",58},{"wash",59},{"water trough",59},{"quench",59},{"cutting table",9},{"mixing table",10},{"reagent",11},{"tonic",11},{"display cabinet",13},{"high seat",27},{"fur trunk",17},{"trunk",17},{"runner",33},{"stand",25}};
    for(auto pair:detailNames)if(key.find(pair.first)!=std::string::npos)return pair.second;
    const std::pair<const char*,int> names[]={
      {"exit",35},{"door",16},{"vault",16},{"anvil",0},{"forge",1},{"furnace",1},{"loom",6},{"spinning",7},{"cloth",8},{"fabric",8},{"sewing",9},{"alchemy",10},{"alchem",10},{"potion",11},{"cauldron",12},{"bookshelf",13},{"bookcase",13},{"spell",14},{"strongbox",17},{"chest",17},{"counter",15},{"stall",18},{"hitch",19},{"hay",20},{"trough",21},{"cot",22},{"medicine",23},{"herbal",23},{"altar",24},{"lectern",25},{"desk",26},{"chair",27},{"throne",27},{"dining",28},{"produce",29},{"barrel",30},{"crate",31},{"box",31},{"bed",32},{"rug",33},{"carpet",33},{"torch",34},{"brazier",34},{"lumber",4},{"plank",4},{"sawhorse",3},{"tool",5},{"workbench",2},{"worktable",2},{"rack",4},{"shelf",13},{"shelves",13},{"table",28},{"bench",2},{"banner",33},{"coin",17},{"basket",29}};
    for(auto pair:names)if(key.find(pair.first)!=std::string::npos)return pair.second;
    return 31; // a furnishing sprite rather than a colored placeholder block
}
static bool Prop(int index,Vector2 center,float width,float height,bool highlight){
    const Atlas& atlas=index>=36?details:props;int cell=index>=36?index-36:index;
    if(!atlas.texture.id || cell<0 || cell>=(int)atlas.frames.size())return false;auto r=atlas.frames[cell];if(r.height<=0)return false;
    bool tall=index==4 || index==5 || index==6 || index==11 || index==13 || index==16 || index==23;
    float scale=std::min(std::max(tall?32.f:18.f,width)*1.15f/r.width,std::max(tall?38.f:24.f,height)*1.65f/r.height);
    if(highlight)DrawEllipseLines((int)center.x,(int)(center.y+height*.28f),std::max(10.f,width*.5f),6,Color{235,205,138,255});
    DrawFrame(atlas,cell,{center.x,center.y+height*.35f},scale);return true;
}
}
