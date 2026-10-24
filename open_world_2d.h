#pragma once
// Continuous 2D outdoor presentation. Gameplay stays in the existing wilderness,
// shops, combat and town systems; town positions are projected into its coordinates.
static Texture2D g_openWildGround{};
static constexpr int kOpenGroundSize=1024;
static constexpr float kOpenSceneryCell=86.f;
static constexpr float kOpenBuildingSize=138.f;
static bool OpenVisible(Vector2 p,float pad=180) {
    return p.x>kViewport.x-pad && p.x<kViewport.x+kViewport.width+pad &&
           p.y>kViewport.y-pad && p.y<kViewport.y+kViewport.height+pad;
}
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
    for(const auto& road:g_wtRoads)for(size_t i=1;i<road.size();i++) {
        Vector2 a=road[i-1],b=road[i];
        float dx=b.x-a.x,dy=b.y-a.y,dd=dx*dx+dy*dy;
        float t=dd>0?std::clamp(((p.x-a.x)*dx+(p.y-a.y)*dy)/dd,0.f,1.f):0;
        if(Dist(p,{a.x+t*dx,a.y+t*dy})<48)return true;
    }
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
           OpenTownAt(w)>=0 || WildBlocked(w) || OpenNearRoad(w))continue;
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
        float size=tree?110+tf2dart::Noise(y+9,x)*45:28+n*28;
        tf2dart::Shadow(p,size*.26f,size*.065f,tree?.20f:.12f);
        // Foot-anchored art; trunks stay behind the hero until the hero walks north.
        float width=size*tex.width/tex.height;
        DrawTexturePro(tex,{0,0,(float)tex.width,(float)tex.height},
            {p.x-width*.5f,p.y-size,width,size},{0,0},0,snow?Color{215,229,228,255}:WHITE);
        if(!foreground && !tree) {
            for(int i=0;i<3;i++) {
                float a=tf2dart::Noise(x+i*17,y)*6.28f;
                DrawCircleV({p.x+cosf(a)*22,p.y+sinf(a)*12},1.5f,Color{181,172,100,180});
            }
        }
    }
}
static void OpenTownDraw(const GameState& s,Vector2 camera,bool foreground) {
    for(int ti=0;ti<4;ti++) {
        Vector2 center=OpenTownWorld(ti,TS(500,480)),sp=WorldToScreen(center,camera);
        if(!OpenVisible(sp,640))continue;
        auto world=[&](Vector2 p){return OpenTownWorld(ti,p);};
        auto screen=[&](Vector2 p){return WorldToScreen(world(p),camera);};
        if(!foreground) {
            auto corner=[&](float x,float y){return WorldToScreen(GateWorld({x,y},kTownGates[ti].wildernessPos,TownGateFacing(ti)),camera);};
            Vector2 corners[4]={corner(-286,-kOpenTownDepth),corner(286,-kOpenTownDepth),corner(286,0),corner(-286,0)};
            Color ground=ti==2?Color{205,220,220,255}:ti==3?Color{121,122,109,255}:Color{79,92,57,255};
            DrawTriangle(corners[0],corners[2],corners[1],ground);
            DrawTriangle(corners[0],corners[3],corners[2],ground);
            Vector2 plaza=screen(TS(500,500)),gate=WorldToScreen(kTownGates[ti].wildernessPos,camera);
            DrawLineEx(plaza,gate,36,Color{153,145,112,255});
            for(const auto& node:ActiveTownNodes(ti)) {
                Vector2 p=screen(node.pos);
                DrawLineEx(plaza,p,24,Color{143,137,109,255});
                DrawCircleV(p,24,Color{143,137,109,255});
            }
            // Stone perimeter and an open gateway on the same terrain/camera.
            for(int side=-1;side<=1;side+=2) {
                Vector2 a=WorldToScreen(GateWorld({side*286.f,0},kTownGates[ti].wildernessPos,TownGateFacing(ti)),camera);
                Vector2 b=WorldToScreen(GateWorld({side*286.f,-kOpenTownDepth},kTownGates[ti].wildernessPos,TownGateFacing(ti)),camera);
                DrawLineEx(a,b,14,Color{91,94,82,255});DrawLineEx({a.x-2,a.y-3},{b.x-2,b.y-3},6,Color{164,162,140,255});
            }
            Vector2 a=WorldToScreen(GateWorld({-286,-kOpenTownDepth},kTownGates[ti].wildernessPos,TownGateFacing(ti)),camera);
            Vector2 b=WorldToScreen(GateWorld({286,-kOpenTownDepth},kTownGates[ti].wildernessPos,TownGateFacing(ti)),camera);
            DrawLineEx(a,b,14,Color{125,127,108,255});
        }
        for(const auto& node:ActiveTownNodes(ti)) {
            Vector2 w=world(node.pos),p=WorldToScreen(w,camera);
            if((w.y>s.wildernessPlayerPos.y)!=foreground || !OpenVisible(p))continue;
            const Texture2D* art=ti==0?tf2dart::Building(node.key):nullptr;
            if(!art)art=ti==2?FindFrostmereBuildingTexture(node.key):ti==3?FindCragmoorBuildingTexture(node.key):ti==1?FindSaltmereBuildingTexture(node.key):FindTownBuildingTexture(node.key);
            bool near=Dist(w,s.wildernessPlayerPos)<80;
            if(art && art->id) {
                tf2dart::Shadow(p,48,13,.22f);
                float width=kOpenBuildingSize*art->width/art->height;
                DrawTexturePro(*art,{0,0,(float)art->width,(float)art->height},
                    {p.x-width*.5f,p.y-kOpenBuildingSize+22,width,kOpenBuildingSize},{0,0},0,ti==2?Color{216,233,239,255}:WHITE);
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
            DrawActorSprite(sheets[i],WanderFacing(i,s.worldTime),ActorAnim::Walk,s.worldTime,p,42);
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
