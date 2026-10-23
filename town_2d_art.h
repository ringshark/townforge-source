#pragma once

// Town-only presentation. No collision, input, save, or 3D state lives here.
namespace tf2dart {
static constexpr int kTile = 256;
static constexpr float kBuildingSize = 164.0f;
static constexpr float kActorScale = 1.16f;
static constexpr int kMotes = 24;
static constexpr const char* kKeys[] = {"smith", "carpenter", "tailor", "alchemy",
    "provisioner", "stable", "healer", "bank", "townhall", "house"};
static Texture2D buildings[10]{}, scenery[8]{}, grass{}, paving{};
static constexpr const char* kScenery[] = {"tree_leafy","tree_autumn","tree_pine","prop_hedge",
    "prop_lamp","town_fountain","prop_barrel","prop_crates"};

inline float Noise(int x, int y) {
    unsigned h = unsigned(x)*374761393u + unsigned(y)*668265263u;
    h = (h^(h>>13))*1274126177u;
    return float(h^(h>>16))/float(0xffffffffu);
}
inline void Load() {
    for (int i=0;i<10;i++) {
        buildings[i] = LoadTexture((std::string("assets/town_polish/")+kKeys[i]+".png").c_str());
        if (buildings[i].id) {
            SetTextureFilter(buildings[i], TEXTURE_FILTER_BILINEAR);
        }
    }
    for(int i=0;i<8;i++) {
        scenery[i]=LoadTexture((std::string("assets/town_polish/")+kScenery[i]+".png").c_str());
        if(scenery[i].id)SetTextureFilter(scenery[i],TEXTURE_FILTER_POINT);
    }
    Image turf = GenImageColor(kTile,kTile,BLANK), stone = GenImageColor(kTile,kTile,BLANK);
    auto* gp = (Color*)turf.data; auto* sp = (Color*)stone.data;
    for (int y=0;y<kTile;y++) for (int x=0;x<kTile;x++) {
        // Periodic broad color variation: quieter than repeating tiny grass tiles.
        float broad = sinf(x*6.2831853f/kTile)*sinf(y*6.2831853f/kTile)*5;
        float grain = (Noise(x,y)-.5f)*9 + broad;
        bool blade = Noise(x/2,y/3)>.96f;
        gp[y*kTile+x] = {(unsigned char)(66+grain+(blade?9:0)),
            (unsigned char)(84+grain+(blade?12:0)),(unsigned char)(53+grain),255};
        int row=y/16, xx=(x+(row%2)*16)%32;
        float v=(Noise((x+(row%2)*16)/32,row)-.5f)*15+(Noise(x,y)-.5f)*5;
        bool seam=xx<1 || y%16<1;
        float bevel=(xx==1 || y%16==1)?7.f:0.f;
        sp[y*kTile+x] = {(unsigned char)(seam?85:137+v+bevel),
            (unsigned char)(seam?87:137+v+bevel),(unsigned char)(seam?75:118+v+bevel),255};
    }
    grass=LoadTextureFromImage(turf); paving=LoadTextureFromImage(stone);
    UnloadImage(turf); UnloadImage(stone);
    if(grass.id)SetTextureFilter(grass,TEXTURE_FILTER_POINT);
    if(paving.id)SetTextureFilter(paving,TEXTURE_FILTER_POINT);
}
inline void Unload() {
    for(auto& t:buildings) {if(t.id)UnloadTexture(t);t={};}
    for(auto& t:scenery){if(t.id)UnloadTexture(t);t={};}
    if(grass.id)UnloadTexture(grass); if(paving.id)UnloadTexture(paving);
    grass={};paving={};
}
inline const Texture2D* Building(const std::string& key) {
    for(int i=0;i<10;i++)if(key==kKeys[i] && buildings[i].id)return &buildings[i];
    return nullptr;
}
inline void Shadow(Vector2 foot,float width,float height,float alpha=.16f) {
    for(int i=3;i>0;i--)DrawEllipse((int)(foot.x+4),(int)(foot.y+2),
        width*(1+i*.12f),height*(1+i*.18f),Fade(Color{22,30,24,255},alpha/3));
}
// Clip each texture sample to the band, with world-aligned UVs. The old
// DrawWallBand draws whole tiles beyond a narrow road's right and bottom edge.
inline void Band(Rectangle world,Vector2 camera,Rectangle view,Color tint=WHITE) {
    if(world.width<=0 || world.height<=0)return;
    float left=std::max(world.x,camera.x), top=std::max(world.y,camera.y);
    float right=std::min(world.x+world.width,camera.x+view.width);
    float bottom=std::min(world.y+world.height,camera.y+view.height);
    if(!paving.id || right<=left || bottom<=top)return;
    for(float y=top;y<bottom;) {
        float sy=fmodf(y,(float)kTile), h=std::min(bottom-y,kTile-sy);
        for(float x=left;x<right;) {
            float sx=fmodf(x,(float)kTile), w=std::min(right-x,kTile-sx);
            DrawTexturePro(paving,{sx,sy,w,h},{view.x+x-camera.x,view.y+y-camera.y,w,h},{0,0},0,tint);
            x+=w;
        }
        y+=h;
    }
}
inline void Road(Vector2 pos,Rectangle plaza,Vector2 camera,Rectangle view,Color tint=WHITE) {
    constexpr float width=38;
    float cx=plaza.x+plaza.width/2, cy=plaza.y+plaza.height/2;
    if(pos.x>=plaza.x && pos.x<=plaza.x+plaza.width && pos.y>=plaza.y && pos.y<=plaza.y+plaza.height)return;
    // Same center-column L bends as the existing town, exactly the same destinations.
    Band({std::min(pos.x,cx)-width/2,pos.y-width/2,fabsf(pos.x-cx)+width,width},camera,view,tint);
    Band({cx-width/2,std::min(pos.y,cy),width,fabsf(pos.y-cy)},camera,view,tint);
}
inline void Motes(Vector2 camera,Rectangle view,float time,float worldSize,float night) {
    for(int i=0;i<kMotes;i++) {
        Vector2 p={Noise(i,7)*worldSize+sinf(time*.23f+i)*19,
            fmodf(Noise(i,11)*worldSize+time*(2+i%3),worldSize)};
        p={view.x+p.x-camera.x,view.y+p.y-camera.y};
        if(p.x<view.x || p.x>view.x+view.width || p.y<view.y || p.y>view.y+view.height)continue;
        float pulse=.5f+.5f*sinf(time*1.7f+i*2);
        Color c=night>.3f ? Color{220,233,134,255}:Color{202,172,101,255};
        if(night>.3f)DrawCircleV(p,5,Fade(c,pulse*.06f));
        DrawEllipse((int)p.x,(int)p.y,1.8f,1.0f,Fade(c,.25f+pulse*.35f));
    }
}
}
