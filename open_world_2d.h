#pragma once
// Continuous 2D outdoor presentation. Gameplay stays in the existing wilderness,
// shops, combat and town systems; town positions are projected into its coordinates.
static Texture2D g_openWildGround{};
static Texture2D g_openTownGround[4]{};
static constexpr int kOpenGroundSize=1024;
static constexpr float kOpenSceneryCell=86.f;
static bool OpenVisible(Vector2 p,float pad=180) {
    return p.x>kViewport.x-pad && p.x<kViewport.x+kViewport.width+pad &&
           p.y>kViewport.y-pad && p.y<kViewport.y+kViewport.height+pad;
}
#include "world_design_2d.h"
static void OpenWildGround(Vector2 camera) {
    if(!g_openWildGround.id) {
        Image img=GenImageColor(kOpenGroundSize,kOpenGroundSize,BLANK);
        auto* px=(Color*)img.data;
        for(int y=0;y<kOpenGroundSize;y++)for(int x=0;x<kOpenGroundSize;x++) {
            float wx=(x+.5f)*kWildernessWorldSize/kOpenGroundSize;
            float wy=(y+.5f)*kWildernessWorldSize/kOpenGroundSize;
            float grain=(tf2dart::Noise(x,y)-.5f)*12;
            float broad=sinf(wx*.008f)*sinf(wy*.006f)*8+sinf((wx+wy)*.021f)*3;
            float snow=std::clamp((700*kWS+100-wy)/200,0.f,1.f);
            float coast=std::clamp((wx-1950*kWS+120)/240,0.f,1.f)*(1-snow);
            float stone=std::clamp((500*kWS+100-wx)/200,0.f,1.f)*(1-snow);
            float r=76+broad+grain,g=98+broad+grain,b=58+broad*.5f+grain;
            auto mix=[&](float a,float R,float G,float B){r+=(R-r)*a;g+=(G-g)*a;b+=(B-b)*a;};
            mix(coast,173+grain,158+grain,113+grain);
            mix(stone,124+grain,126+grain,112+grain);
            mix(snow,213+grain,225+grain,224+grain);
            px[y*kOpenGroundSize+x]={(unsigned char)r,(unsigned char)g,(unsigned char)b,255};
        }
        g_openWildGround=LoadTextureFromImage(img);UnloadImage(img);
        SetTextureFilter(g_openWildGround,TEXTURE_FILTER_BILINEAR);
    }
    Vector2 p=WorldToScreen({0,0},camera);
    DrawTexturePro(g_openWildGround,{0,0,kOpenGroundSize,kOpenGroundSize},
        {p.x,p.y,kWildernessWorldSize,kWildernessWorldSize},{0,0},0,WHITE);
}
static bool OpenNearRoad(Vector2 p) {
    for(float dx:{-40.f,0.f,40.f})for(float dy:{-40.f,0.f,40.f})
        if(WildTerrainAt(p.x+dx,p.y+dy)&kWTRoad)return true;
    return false;
}
static void OpenWildScenery(const GameState& s,Vector2 camera,bool foreground) {
    int x0=std::max(0,(int)((camera.x-180)/kOpenSceneryCell));
    int y0=std::max(0,(int)((camera.y-180)/kOpenSceneryCell));
    int x1=(int)((camera.x+kViewport.width+180)/kOpenSceneryCell);
    int y1=(int)((camera.y+kViewport.height+180)/kOpenSceneryCell);
    for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++) {
        float n=tf2dart::Noise(x*13+4,y*7+3);
        if(n>.46f)continue;
        Vector2 w={(x+.15f+tf2dart::Noise(x,y)*.7f)*kOpenSceneryCell,
                   (y+.15f+tf2dart::Noise(y,x)*.7f)*kOpenSceneryCell};
        if(w.x>=kWildernessWorldSize || w.y>=kWildernessWorldSize ||
           OpenTownAt(w)>=0 || WildBlocked(w) || OpenNearRoad(w) ||
           (s.life.farm && Dist(w,{s.life.x,s.life.y})<150))continue;
        // Keep harvest nodes, creatures, dungeon doors and homes legible and clear.
        bool clear=true;
        for(const auto& node:kWildernessGatherNodes)if(Dist(w,node.pos)<80){clear=false;break;}
        for(const auto& door:kWildernessDungeonEntrances)if(Dist(w,door.pos)<120){clear=false;break;}
        for(const auto& plot:kHousePlots)if(Dist(w,plot.pos)<170){clear=false;break;}
        if(!clear || (w.y>s.wildernessPlayerPos.y)!=foreground)continue;
        Vector2 p=WorldToScreen(w,camera);if(!OpenVisible(p))continue;
        bool snow=w.y<700*kWS;
        bool coast=w.x>1950*kWS;
        bool tree=n<.22f && !coast;
        int index=tree?(snow?2:(n<.05f?1:0)):3;
        auto& tex=tf2dart::scenery[index];if(!tex.id)continue;
        float size=tree?tfworldart::kTreeMin+tf2dart::Noise(y+9,x)*tfworldart::kTreeVariation:18+n*20;
        tf2dart::Shadow(p,size*.26f,size*.065f,tree?.20f:.12f);
        // Foot-anchored art; trunks stay behind the hero until the hero walks north.
        float width=size*tex.width/tex.height;
        Vector2 hero=WorldToScreen(s.wildernessPlayerPos,camera);
        bool occludes=foreground && fabsf(hero.x-p.x)<width*.45f && hero.y>p.y-size && hero.y<p.y+12;
        Color tint=snow?Color{215,229,228,255}:WHITE;
        if(occludes)tint=Fade(tint,.38f);
        DrawTexturePro(tex,{0,0,(float)tex.width,(float)tex.height},
            {p.x-width*.5f,p.y-size,width,size},{0,0},0,tint);
        if(!foreground && !tree) {
            for(int i=0;i<3;i++) {
                float a=tf2dart::Noise(x+i*17,y)*6.28f;
                DrawCircleV({p.x+cosf(a)*22,p.y+sinf(a)*12},1.5f,Color{181,172,100,180});
            }
        }
    }
}
static void OpenResourceTrees(const GameState& s,Vector2 camera,bool foreground) {
    const auto& tex=tf2dart::scenery[0];if(!tex.id)return;
    for(const auto& node:kWildernessGatherNodes) {
        if(node.resource!="wood" || OpenTownAt(node.pos)>=0 ||
           (node.pos.y>s.wildernessPlayerPos.y)!=foreground)continue;
        Vector2 p=WorldToScreen(node.pos,camera);if(!OpenVisible(p,100))continue;
        const float height=82,width=height*tex.width/tex.height,foot=p.y+12;
        tf2dart::Shadow({p.x,foot},width*.25f,5,.20f);
        bool near=Dist(node.pos,s.wildernessPlayerPos)<kInteractRange+35;
        if(near)DrawEllipseLines((int)p.x,(int)foot,23,7,Color{231,200,128,230});
        Vector2 hero=WorldToScreen(s.wildernessPlayerPos,camera);
        bool occludes=foreground && fabsf(hero.x-p.x)<width*.4f && hero.y>foot-height;
        DrawTexturePro(tex,{0,0,(float)tex.width,(float)tex.height},
            {p.x-width/2,foot-height,width,height},{0,0},0,Fade(WHITE,occludes?.38f:1.f));
        if(near)DrawWorldLabel("Harvest wood",{p.x,foot+19},11,Color{241,225,188,255});
    }
}
// Paved lanes have a quiet stone border and world-aligned joints. Keep the
// widths in world pixels, independent of each town's coordinate compression.
static void OpenStreet(Vector2 a,Vector2 b,float width,Color stone) {
    float length=Dist(a,b);if(length<.1f)return;
    Vector2 tangent={(b.x-a.x)/length,(b.y-a.y)/length},normal={-tangent.y,tangent.x};
    DrawLineEx(a,b,width+4,Color{89,91,77,255});
    DrawCircleV(a,(width+4)*.5f,Color{89,91,77,255});
    DrawCircleV(b,(width+4)*.5f,Color{89,91,77,255});
    DrawLineEx(a,b,width,stone);DrawCircleV(a,width*.5f,stone);DrawCircleV(b,width*.5f,stone);
    for(float d=12;d<length-6;d+=14) {
        Vector2 p={a.x+tangent.x*d,a.y+tangent.y*d};
        DrawLineEx({p.x-normal.x*(width*.5f-2),p.y-normal.y*(width*.5f-2)},
                   {p.x+normal.x*(width*.5f-2),p.y+normal.y*(width*.5f-2)},.7f,Fade(Color{79,83,72,255},.20f));
    }
}
static void OpenTownGround(int ti,Vector2 camera) {
    const int tile=128;
    auto& texture=g_openTownGround[ti];
    if(!texture.id) {
        const Color colors[4]={{79,94,59,255},{151,139,108,255},{205,220,220,255},{121,122,109,255}};
        Image image=GenImageColor(tile,tile,colors[ti]);auto* pixels=(Color*)image.data;
        for(int y=0;y<tile;y++)for(int x=0;x<tile;x++) {
            float grain=(tf2dart::Noise(x+ti*137,y)-.5f)*9;
            if(ti==0 && tf2dart::Noise(x/2,y/3)>.96f)grain+=6;
            Color base=colors[ti];
            pixels[y*tile+x]={(unsigned char)(base.r+grain),(unsigned char)(base.g+grain),(unsigned char)(base.b+grain),255};
        }
        texture=LoadTextureFromImage(image);UnloadImage(image);
        if(texture.id)SetTextureFilter(texture,TEXTURE_FILTER_POINT);
    }
    if(!texture.id)return;
    Vector2 facing=TownGateFacing(ti),gate=kTownGates[ti].wildernessPos;
    float depth=OpenTownDepth(ti),rotation=atan2f(-facing.x,facing.y)*RAD2DEG;
    // Clip the last tile to the wall. Rotation follows the actual gate basis,
    // while buildings retain their top-down art orientation.
    for(float y=0;y<depth;y+=tile)for(float x=0;x<kOpenTownHalfW*2;x+=tile) {
        float w=std::min((float)tile,kOpenTownHalfW*2-x),h=std::min((float)tile,depth-y);
        Vector2 p=WorldToScreen(GateWorld({x-kOpenTownHalfW,y-depth},gate,facing),camera);
        DrawTexturePro(texture,{0,0,w,h},{p.x,p.y,w,h},{0,0},rotation,WHITE);
    }
}
// Five spans close all four sides while leaving the gatehouse passage open.
// The front curtain shares GateCollide's y=-6 line; towers cover its inner ends.
struct OpenTownWallSpan { Vector2 a,b; };
static std::array<OpenTownWallSpan,5> OpenTownWallSpans(int ti) {
    auto point=[&](float x,float y){return GateWorld({x,y},kTownGates[ti].wildernessPos,TownGateFacing(ti));};
    float w=kOpenTownHalfW,d=OpenTownDepth(ti);
    return {{{point(-w,0),point(-w,-d)},{point(w,0),point(w,-d)},
             {point(-w,-d),point(w,-d)},
             {point(-w,-6),point(-56,-6)},{point(56,-6),point(w,-6)}}};
}
static void OpenTownDraw(const GameState& s,Vector2 camera,bool foreground) {
    for(int ti=0;ti<4;ti++) {
        Vector2 center=OpenTownWorld(ti,TS(500,480)),sp=WorldToScreen(center,camera);
        if(!OpenVisible(sp,640))continue;
        auto world=[&](Vector2 p){return OpenTownWorld(ti,p);};
        auto screen=[&](Vector2 p){return WorldToScreen(world(p),camera);};
        if(!foreground) {
            auto corner=[&](float x,float y){return WorldToScreen(GateWorld({x,y},kTownGates[ti].wildernessPos,TownGateFacing(ti)),camera);};
            Vector2 corners[4]={corner(-kOpenTownHalfW,-OpenTownDepth(ti)),corner(kOpenTownHalfW,-OpenTownDepth(ti)),corner(kOpenTownHalfW,0),corner(-kOpenTownHalfW,0)};
            Color ground=ti==2?Color{205,220,220,255}:ti==3?Color{121,122,109,255}:Color{79,92,57,255};
            DrawTriangle(corners[0],corners[2],corners[1],ground);
            DrawTriangle(corners[0],corners[3],corners[2],ground);
            OpenTownGround(ti,camera);
            tfdesign::DrawTownStreets(ti,camera);
            // Orchard-like planting at the edge of each district; never in a
            // street or the approach to a shop door. Existing trees are reused.
            for(Vector2 site:std::array<Vector2,4>{{TS(95,440),TS(905,440),TS(95,720),TS(905,720)}}){
                Vector2 w=world(site);bool clear=OpenTownAt(w)==ti;
                for(const auto& n:ActiveTownNodes(ti))if(Dist(w,world(n.pos))<85)clear=false;
                for(const auto& lane:tfdesign::TownLanes(ti))if(Dist(w,tfdesign::NearestOn(w,lane.a,lane.b))<48)clear=false;
                const auto& tree=tf2dart::scenery[ti==2?2:0];
                if(clear && tree.id){Vector2 p=WorldToScreen(w,camera);float h=72,width=h*tree.width/tree.height;tf2dart::Shadow(p,16,5,.16f);DrawTexturePro(tree,{0,0,(float)tree.width,(float)tree.height},{p.x-width/2,p.y-h,width,h},{0,0},0,WHITE);}
            }
            // Low planting and lamps occupy the edge of each street junction.
            // They never become new collision objects or interfere with doors.
            float night=std::clamp((.62f-T3DLightAt(T3DLocalTimeOfDay()).sunI)/.34f,0.f,1.f);
            for(float x:{350.f,650.f})for(float y:{350.f,650.f}) {
                Vector2 p=screen(TS(x+45,y-45));
                tf2dart::Shadow(p,7,2,.18f);
                const auto& lamp=tf2dart::scenery[4];
                if(lamp.id) {
                    float h=30,w=h*lamp.width/lamp.height;
                    DrawTexturePro(lamp,{0,0,(float)lamp.width,(float)lamp.height},{p.x-w/2,p.y-h,w,h},{0,0},0,WHITE);
                    if(night>.05f)for(int r=3;r>0;r--)DrawCircleV({p.x,p.y-h+5},r*5,Fade(Color{255,205,120,255},night*.035f));
                }
            }
            // A continuous stone perimeter, split only at the usable gate.
            for(const auto& span:OpenTownWallSpans(ti)) {
                Vector2 a=WorldToScreen(span.a,camera),b=WorldToScreen(span.b,camera);
                DrawLineEx(a,b,14,Color{91,94,82,255});
                DrawLineEx({a.x-2,a.y-3},{b.x-2,b.y-3},6,Color{164,162,140,255});
            }
        }
        for(const auto& node:ActiveTownNodes(ti)) {
            Vector2 w=world(node.pos),p=WorldToScreen(w,camera);
            if((w.y>s.wildernessPlayerPos.y)!=foreground || !OpenVisible(p))continue;
            const Texture2D* art=ti==0?tf2dart::Building(node.key):nullptr;
            if(!art)art=ti==2?FindFrostmereBuildingTexture(node.key):ti==3?FindCragmoorBuildingTexture(node.key):ti==1?FindSaltmereBuildingTexture(node.key):FindTownBuildingTexture(node.key);
            bool near=Dist(w,s.wildernessPlayerPos)<80;
            if(art && art->id) {
                Vector2 extent=tfworldart::BuildingExtent(*art);
                float foot=p.y+16;
                tf2dart::Shadow({p.x,foot},extent.x*.38f,8,.20f);
                if(near)DrawEllipseLines((int)p.x,(int)foot,25,7,Color{228,199,126,220});
                Vector2 hero=WorldToScreen(s.wildernessPlayerPos,camera);
                bool occludes=foreground && fabsf(hero.x-p.x)<extent.x*.45f && hero.y>foot-extent.y && hero.y<foot;
                Color tint=ti==2?Color{216,233,239,255}:WHITE;
                if(occludes)tint=Fade(tint,.42f);
                DrawTexturePro(*art,{0,0,(float)art->width,(float)art->height},
                    {p.x-extent.x*.5f,foot-extent.y,extent.x,extent.y},{0,0},0,tint);
            } else DrawBuildingNode(p,TileColorFor(node.key),TileNameFor(node.key),near,"",FindBuildingTexture(node.key),DoorAnimForBuilding(node.key),nullptr,WHITE,.7f);
            if(near)DrawWorldLabel(TileNameFor(node.key),{p.x,p.y+27},12,Color{240,222,175,255});
        }
        const auto& npcs=ActiveTownNPCs(ti);
        const auto& sheets=ti==0?g_assets.townNPCSheets:g_assets.saltmereNPCSheets;
        for(int i=0;i<(int)npcs.size();i++) {
            Vector2 w=world(TownNPCLivePos(i,s.worldTime,ti));
            if((w.y>s.wildernessPlayerPos.y)!=foreground)continue;
            Vector2 p=WorldToScreen(w,camera);if(!OpenVisible(p) || !sheets[i].ok)continue;
            tf2dart::Shadow(p,10,3,.18f);
            DrawActorSprite(sheets[i],WanderFacing(i,s.worldTime),ActorAnim::Walk,s.worldTime,p,tfworldart::kHeroSize);
        }
    }
}

static void OpenWildWater(Vector2 camera,float time) {
    const float step=72;
    int x0=std::max(0,(int)(camera.x/step)),y0=std::max(0,(int)(camera.y/step));
    for(int y=y0;y<(camera.y+kViewport.height)/step+1;y++)
        for(int x=x0;x<(camera.x+kViewport.width)/step+1;x++) {
            Vector2 w={(x+.5f)*step,(y+.5f)*step};
            if(OpenTownAt(w)>=0 || !(WildTerrainAt(w.x,w.y)&kWTWater))continue;
            float phase=time*.8f+tf2dart::Noise(x,y)*6.28f;
            Vector2 p=WorldToScreen(w,camera);
            p.x+=sinf(phase)*9;p.y+=cosf(phase*.7f)*4;
            float width=8+tf2dart::Noise(y,x)*14;
            DrawLineEx({p.x-width,p.y},{p.x+width,p.y-2},1.4f,
                Fade(Color{175,219,218,255},.10f+.08f*(1+sinf(phase))));
        }
}
