#pragma once

// All sizes are screen/world pixels at the native 2D camera scale. Keep these
// independent of combat radii and the compressed coordinates of outdoor towns.
namespace tfworldart {
static constexpr float kHeroSize=34.f;
static constexpr float kBuildingHeight=94.f, kBuildingWidth=96.f;
static constexpr float kTreeMin=74.f, kTreeVariation=22.f;
struct Actor {unsigned id; Rectangle crop; float size;};
static std::vector<Actor> actors;

inline float ActorSize(const std::string& path) {
    if(path.find("/monsters_boss_v2/")!=std::string::npos)return 62;
    if(path.find("/npcs_v2/")!=std::string::npos || path.find("rival_adventurer")!=std::string::npos)return 34;
    if(path.find("/monsters_v2/")!=std::string::npos)return 38;
    if(path.find("/monsters/")!=std::string::npos) {
        if(path.find("bat.png")!=std::string::npos)return 24;
        if(path.find("imp.png")!=std::string::npos)return 28;
        if(path.find("goblin.png")!=std::string::npos)return 30;
        if(path.find("wolf.png")!=std::string::npos)return 32;
        return 34;
    }
    if(path.find("/creatures/")!=std::string::npos) {
        if(path.find("dragon.png")!=std::string::npos)return 58;
        if(path.find("wyvern.png")!=std::string::npos)return 52;
        if(path.find("griffin.png")!=std::string::npos)return 46;
        if(path.find("bear.png")!=std::string::npos || path.find("bison.png")!=std::string::npos)return 42;
        if(path.find("horse.png")!=std::string::npos || path.find("drake.png")!=std::string::npos)return 40;
        if(path.find("dog.png")!=std::string::npos)return 24;
        return 32;
    }
    return 34;
}
// Remove only uniform white matte connected to each cell's border. Interior
// highlights, eyes and pale fur remain opaque; original image files are retained.
inline void CleanMatte(Image& image,int fw,int fh) {
    if(!image.data || fw<=0 || fh<=0)return;ImageFormat(&image,PIXELFORMAT_UNCOMPRESSED_R8G8B8A8);
    auto* px=(Color*)image.data;
    for(int y0=0;y0<image.height;y0+=fh)for(int x0=0;x0<image.width;x0+=fw){
        int x1=std::min(image.width,x0+fw),y1=std::min(image.height,y0+fh);std::vector<int> queue;
        auto add=[&](int x,int y){if(x<x0 || x>=x1 || y<y0 || y>=y1)return;auto& c=px[y*image.width+x];if(c.a && c.r>=248 && c.g>=248 && c.b>=248){c.a=0;queue.push_back(y*image.width+x);}};
        for(int x=x0;x<x1;x++){add(x,y0);add(x,y1-1);}for(int y=y0;y<y1;y++){add(x0,y);add(x1-1,y);}
        for(size_t i=0;i<queue.size();i++){int x=queue[i]%image.width,y=queue[i]/image.width;add(x+1,y);add(x-1,y);add(x,y+1);add(x,y-1);}
    }
}
// One union of the opaque bounds across all usable animation cells. A single
// crop per sheet preserves stride/bob and never resizes individual frames.
inline Rectangle Bounds(const Color* pixels,int width,int height,int fw,int fh,int rows,int cols) {
    int left=fw,top=fh,right=0,bottom=0;
    if(pixels)for(int row=0;row<std::min(rows,3);row++)for(int col=0;col<cols;col++)
        for(int y=0;y<fh && row*fh+y<height;y++)for(int x=0;x<fw && col*fw+x<width;x++)
            if(pixels[(row*fh+y)*width+col*fw+x].a>96) {
                left=std::min(left,x);top=std::min(top,y);
                right=std::max(right,x+1);bottom=std::max(bottom,y+1);
            }
    if(right<=left || bottom<=top)return {0,0,(float)fw,(float)fh};
    return {(float)left,(float)top,(float)(right-left),(float)(bottom-top)};
}
inline void Register(const Texture2D& tex,const std::string& path,int fw,int fh,int rows,int cols) {
    Image image=LoadImage(path.c_str());
    CleanMatte(image,fw,fh);
    Color* pixels=image.data?LoadImageColors(image):nullptr;
    Rectangle crop=Bounds(pixels,image.width,image.height,fw,fh,rows,cols);
    actors.push_back({tex.id,crop,ActorSize(path)});
    if(pixels)UnloadImageColors(pixels);
    if(image.data)UnloadImage(image);
}
inline const Actor* Find(unsigned id) {
    for(const auto& a:actors)if(a.id==id)return &a;
    return nullptr;
}
inline Rectangle Source(Rectangle frame,Rectangle crop) {
    return {frame.x+crop.x,frame.y+crop.y,frame.width<0?-crop.width:crop.width,crop.height};
}
inline void Draw(const Texture2D& tex,Rectangle frame,Vector2 center,Color tint,float multiplier=1,float rotation=0) {
    const Actor* a=Find(tex.id);
    Rectangle src=a?Source(frame,a->crop):frame;
    float size=(a?a->size:34.f)*multiplier;
    float scale=size/std::max(fabsf(src.width),src.height);
    float w=fabsf(src.width)*scale,h=src.height*scale;
    // Same ground baseline as the compact hero, whose logical position is its centre.
    DrawTexturePro(tex,src,{center.x,center.y+12,w,h},{w*.5f,h},rotation,tint);
}
inline Vector2 BuildingExtent(const Texture2D& tex) {
    float scale=std::min(kBuildingHeight/tex.height,kBuildingWidth/tex.width);
    return {tex.width*scale,tex.height*scale};
}
}
