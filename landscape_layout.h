#pragma once
#include "rlgl.h"
#include "landscape_layout_rules.h"
#include <vector>
#include <set>
#include <cmath>
#include <map>
#include <string>
#ifdef PLATFORM_WEB
#include <emscripten/emscripten.h>
#endif
static float g_landscapeScroll=0;
static bool g_landscapeActive=false,g_landscapeWorld=false,g_landscapeDialog=false;
static bool g_presentedWorld=false,g_presentedDialog=true,g_landscapeRedirect=false;
static RenderTexture2D g_landscapeUI{},g_landscapeScene{},g_zoomTarget{};
static std::vector<RenderTexture2D> g_landscapeTargets;
static std::set<unsigned> g_landscapeWorldTextures;
// Keep world raster detail across the wide screen while retaining the existing
// logical canvas for picking, collision and HUD coordinates.
static std::map<unsigned,Vector2> g_landscapeSceneSizes;
struct LandscapeCaption {std::string text;int size;Color color;bool prompt;};
static std::vector<LandscapeCaption> g_landscapeCaptions;
struct LandscapeNotice {std::vector<std::string> lines;Color color;float alpha;};
static std::vector<LandscapeNotice> g_landscapeNotices;
struct LandscapeWorldLabel {std::string text;Vector2 point;int size;Color color;float alpha;};
static std::vector<LandscapeWorldLabel> g_landscapeLabels;
static std::vector<Rectangle> g_landscapeHitRects;
static bool g_presentedMap=false,g_landscapeMap=false;
static Rectangle g_presentedMapSource{},g_presentedMapDest{},g_landscapeMapSource{},g_landscapeMapDest{};
static RenderTexture2D g_landscapeMapTarget{};
static tflayout::Point LandscapePointerMap(Vector2 raw,tflayout::Region region) {
    if(region==tflayout::Region::Map) {
        return {g_presentedMapSource.x+(raw.x-g_presentedMapDest.x)*g_presentedMapSource.width/g_presentedMapDest.width,
                g_presentedMapSource.y+(raw.y-g_presentedMapDest.y)*g_presentedMapSource.height/g_presentedMapDest.height};
    }
    return tflayout::Map({raw.x,raw.y},region,g_landscapeScroll);
}
static tflayout::Region g_landscapePointerRegion=tflayout::Region::Panel;
static bool g_landscapePointerHeld=false;
static double g_landscapePressTime=-1;
static tflayout::Region LandscapeRegion(Vector2 raw) {
    if(g_presentedMap && CheckCollisionPointRec(raw,g_presentedMapDest))return tflayout::Region::Map;
    auto region=tflayout::RegionAt({raw.x,raw.y},g_presentedWorld,g_presentedDialog);
    if(region==tflayout::Region::Panel)return region;
    if(region==tflayout::Region::Left && raw.y>=370)return region; // stick
    auto p=tflayout::Map({raw.x,raw.y},region,g_landscapeScroll);
    if(region!=tflayout::Region::World)
        for(auto r:g_landscapeHitRects)if(CheckCollisionPointRec({p.x,p.y},r))return region;
    // The middle HUD strip uses the world scale but contains real controls.
    // Keep those taps distinct from unclaimed ground gestures.
    if(raw.y>=138 && raw.y<360) {
        p=tflayout::Map({raw.x,raw.y},tflayout::Region::Field);
        for(auto r:g_landscapeHitRects)if(CheckCollisionPointRec({p.x,p.y},r))return tflayout::Region::Field;
    }
    return tflayout::Region::World;
}
static bool LandscapeUIAllowed() {
    return !g_presentedWorld || g_presentedDialog || g_landscapePointerRegion!=tflayout::Region::World;
}
static RenderTexture2D LandscapeLoadScene(int width,int height) {
    RenderTexture2D target=::LoadRenderTexture((int)std::round(width*tflayout::baseAspectFactor),height);
    g_landscapeSceneSizes[target.texture.id]={(float)width,(float)height};
    return target;
}
static void LandscapeRestoreViewport() {
    if(!g_landscapeTargets.empty()) {
        auto t=g_landscapeTargets.back();rlViewport(0,0,t.texture.width,t.texture.height);
    }
}
static Vector2 LandscapeMouse() {
    Vector2 raw=::GetMousePosition();
    bool held=::IsMouseButtonDown(MOUSE_BUTTON_LEFT) || ::IsMouseButtonReleased(MOUSE_BUTTON_LEFT);
    double now=::GetTime();
    bool freshPress=::IsMouseButtonPressed(MOUSE_BUTTON_LEFT) && g_landscapePressTime!=now;
    if(freshPress || !g_landscapePointerHeld)g_landscapePointerRegion=LandscapeRegion(raw);
    if(freshPress)g_landscapePressTime=now;
    g_landscapePointerHeld=held;
    auto p=LandscapePointerMap(raw,g_landscapePointerRegion);return {p.x,p.y};
}
static Vector2 LandscapeTouch(int index) {
    // These positions feed pinch zoom, so never switch scales over HUD regions.
    Vector2 raw=::GetTouchPosition(index);auto p=tflayout::Map({raw.x,raw.y},tflayout::Region::World);return {p.x,p.y};
}
static void LandscapeTextureBegin(RenderTexture2D target) {
    if(g_landscapeActive && (g_landscapeTargets.empty() || g_landscapeTargets.back().id!=target.id))g_landscapeTargets.push_back(target);
    ::BeginTextureMode(target);
    auto logical=g_landscapeSceneSizes.find(target.texture.id);
    if(logical!=g_landscapeSceneSizes.end()) {
        rlMatrixMode(RL_PROJECTION);rlLoadIdentity();
        rlOrtho(0,logical->second.x,logical->second.y,0,0,1);rlMatrixMode(RL_MODELVIEW);
    }
}
static void LandscapeTextureEnd() {
    ::EndTextureMode();
    if(g_landscapeActive && !g_landscapeTargets.empty()) {
        g_landscapeTargets.pop_back();
        if(!g_landscapeTargets.empty())LandscapeTextureBegin(g_landscapeTargets.back());
    }
}
// Isolate a complete map widget before compositing. It must never be sliced
// across the portrait HUD bands; its picking uses the same source/destination.
struct LandscapeMapWidget {
    bool active;
    LandscapeMapWidget(Rectangle source):active(g_landscapeActive && g_landscapeWorld && !g_landscapeDialog) {
        if(!active)return;
        if(!g_landscapeMapTarget.id) {
            g_landscapeMapTarget=::LoadRenderTexture(540,900);
            ::SetTextureFilter(g_landscapeMapTarget.texture,TEXTURE_FILTER_BILINEAR);
        }
        g_landscapeMap=true;g_landscapeMapSource=source;
        g_landscapeMapDest={944-source.width,92,source.width,source.height};
        LandscapeTextureBegin(g_landscapeMapTarget);::ClearBackground(BLANK);
    }
    ~LandscapeMapWidget(){if(active)LandscapeTextureEnd();}
};
static void Landscape3DBegin(Camera3D camera) {
    g_landscapeRedirect=false;
    if(g_landscapeActive && g_landscapeWorld && !g_landscapeTargets.empty()) {
        if(g_landscapeTargets.back().id==g_landscapeUI.id){rlDrawRenderBatchActive();rlEnableFramebuffer(g_landscapeScene.id);rlViewport(0,0,g_landscapeScene.texture.width,g_landscapeScene.texture.height);g_landscapeRedirect=true;}
        else g_landscapeWorldTextures.insert(g_landscapeTargets.back().texture.id);
    }
    ::BeginMode3D(camera);
    if(g_landscapeActive && g_landscapeWorld && camera.projection==CAMERA_PERSPECTIVE && !g_landscapeTargets.empty()) {
        const auto& target=g_landscapeTargets.back();
        auto logical=g_landscapeSceneSizes.find(target.texture.id);
        double aspect=logical==g_landscapeSceneSizes.end() ? double(target.texture.width)/target.texture.height*tflayout::aspectFactor : double(logical->second.x)/logical->second.y*tflayout::aspectFactor;
        double top=10*std::tan(camera.fovy*.5*DEG2RAD);
        rlMatrixMode(RL_PROJECTION);rlLoadIdentity();rlFrustum(-top*aspect,top*aspect,-top,top,10,5000);rlMatrixMode(RL_MODELVIEW);
    }
}
static void Landscape3DEnd() {
    ::EndMode3D();
    if(g_landscapeRedirect){rlDrawRenderBatchActive();rlEnableFramebuffer(g_landscapeUI.id);LandscapeRestoreViewport();g_landscapeRedirect=false;}
}
static void LandscapeTextureDraw(Texture2D texture,Rectangle source,Rectangle dest,Vector2 origin,float rotation,Color tint) {
    bool world=g_landscapeActive && g_landscapeWorld && !g_landscapeTargets.empty() && g_landscapeTargets.back().id==g_landscapeUI.id && g_landscapeWorldTextures.count(texture.id);
    auto logical=g_landscapeSceneSizes.find(texture.id);
    if(logical!=g_landscapeSceneSizes.end()){float scale=texture.width/logical->second.x;source.x*=scale;source.width*=scale;}
    if(world){rlDrawRenderBatchActive();rlEnableFramebuffer(g_landscapeScene.id);rlViewport(0,0,g_landscapeScene.texture.width,g_landscapeScene.texture.height);}
    ::DrawTexturePro(texture,source,dest,origin,rotation,tint);
    if(world){rlDrawRenderBatchActive();rlEnableFramebuffer(g_landscapeUI.id);LandscapeRestoreViewport();}
}
static Vector2 LandscapeProjection(Vector3 pos,Camera camera,int w,int h) {
    if(g_landscapeActive && g_landscapeWorld){Vector2 p=::GetWorldToScreenEx(pos,camera,(int)(w*tflayout::aspectFactor),h);p.x/=tflayout::aspectFactor;return p;}
    return ::GetWorldToScreenEx(pos,camera,w,h);
}
static Vector2 LandscapeScreen(Vector3 pos,Camera camera) {
    return g_landscapeActive && g_landscapeWorld ? LandscapeProjection(pos,camera,540,900): ::GetWorldToScreen(pos,camera);
}
static Ray LandscapeRay(Vector2 pos,Camera camera,int w,int h) {
    if(g_landscapeActive && g_landscapeWorld){pos.x*=tflayout::aspectFactor;return ::GetScreenToWorldRayEx(pos,camera,(int)(w*tflayout::aspectFactor),h);}
    return ::GetScreenToWorldRayEx(pos,camera,w,h);
}
static void LandscapeBeginFrame(bool world,bool dialog,int screen) {
#ifdef PLATFORM_WEB
    // Use the displayed landscape aspect for both projection and picking.
    // CSS fills the phone; a wider camera keeps characters from stretching.
    double aspect=EM_ASM_DOUBLE({
        var canvas=document.getElementById('canvas');
        return canvas && canvas.clientHeight ? canvas.clientWidth/canvas.clientHeight : 960/540;
    });
    tflayout::DisplayAspect((float)aspect,1);
#endif
    static int previousScreen=-1;
    if(previousScreen!=screen){g_landscapeScroll=0;previousScreen=screen;}
    g_landscapeWorld=world;g_landscapeDialog=dialog;g_landscapeMap=false;g_landscapeWorldTextures.clear();g_landscapeTargets.clear();g_landscapeCaptions.clear();g_landscapeNotices.clear();g_landscapeLabels.clear();
    ::BeginTextureMode(g_landscapeScene);::ClearBackground(Color{22,33,42,255});::EndTextureMode();
    ::BeginTextureMode(g_landscapeUI);g_landscapeTargets.push_back(g_landscapeUI);g_landscapeActive=true;
}
static void LandscapePresent(Font font,const std::vector<Rectangle>& controls) {
    g_landscapeActive=false;::EndTextureMode();g_landscapeTargets.clear();
    ::BeginDrawing();::ClearBackground(Color{22,33,42,255});
    auto blit=[](Texture2D texture,Rectangle s,Rectangle d){s.y=texture.height-s.y-s.height;s.height=-s.height;::DrawTexturePro(texture,s,d,{0,0},0,WHITE);};
    if(g_landscapeWorld) {
        blit(g_landscapeScene.texture,{0,0,(float)g_landscapeScene.texture.width,900},{0,0,960,540});
        ::DrawRectangleGradientV(0,0,960,100,Color{7,13,19,75},BLANK);
        ::DrawRectangleGradientV(0,440,960,100,BLANK,Color{7,13,19,95});
        if(!g_landscapeDialog)for(const auto& label:g_landscapeLabels) {
            float w=::MeasureTextEx(font,label.text.c_str(),label.size,1).x;
            float x=label.point.x-w*.5f,y=label.point.y-label.size;
            if(x<4 || x+w>956 || y<8 || y+label.size>532)continue;
            ::DrawRectangleRounded({x-6,y-3,w+12,(float)label.size+8},.2f,4,Fade(Color{12,21,27,255},label.alpha*.8f));
            ::DrawTextEx(font,label.text.c_str(),{x,y},label.size,1,label.color);
        }
    }
    if(!g_landscapeWorld || g_landscapeDialog) {
        if(g_landscapeWorld && !g_presentedDialog)g_landscapeScroll=0;
        Vector2 mouse=::GetMousePosition();
        Rectangle top={786,34,140,48},more={786,458,140,48};
        Rectangle rail={786,104,140,332};
        static bool dragging=false;
        if(::IsMouseButtonPressed(MOUSE_BUTTON_LEFT)) {
            dragging=CheckCollisionPointRec(mouse,rail);
            if(CheckCollisionPointRec(mouse,top))g_landscapeScroll=0;
            if(CheckCollisionPointRec(mouse,more))g_landscapeScroll=tflayout::maxScroll;
        }
        if(!::IsMouseButtonDown(MOUSE_BUTTON_LEFT))dragging=false;
        if(dragging)g_landscapeScroll=tflayout::Scroll((mouse.y-rail.y-30)/(rail.height-60)*tflayout::maxScroll);
        if(CheckCollisionPointRec(mouse,rail))g_landscapeScroll=tflayout::Scroll(g_landscapeScroll-::GetMouseWheelMove()*48);
        if(g_landscapeWorld)::DrawRectangle(0,0,960,540,Color{6,12,18,155});
        ::DrawRectangleGradientH(0,0,210,540,Color{12,21,28,255},Color{28,38,44,255});
        ::DrawRectangleGradientH(750,0,210,540,Color{28,38,44,255},Color{12,21,28,255});
        ::DrawLine(208,24,208,516,Color{141,114,71,255});::DrawLine(752,24,752,516,Color{141,114,71,255});
        ::DrawTextEx(font,"TOWN",{48,224},26,1,Color{230,211,174,255});::DrawTextEx(font,"FORGE",{48,257},26,1,Color{230,211,174,255});
        ::DrawLine(48,302,149,302,Color{141,114,71,255});
        blit(g_landscapeUI.texture,{0,g_landscapeScroll,540,540},{210,0,540,540});
        for(auto r:{top,more}) {::DrawRectangleRounded(r,.15f,4,Color{37,48,54,255});::DrawRectangleRoundedLines(r,.15f,4,Color{163,132,82,255});}
        ::DrawText("Top",833,49,18,Color{238,220,183,255});::DrawText("More",824,473,18,Color{238,220,183,255});
        ::DrawRectangleRounded({848,104,16,332},.6f,6,Color{9,17,23,255});
        ::DrawRectangleRounded({850,108+g_landscapeScroll/tflayout::maxScroll*264,12,60},.6f,6,Color{177,145,91,255});
        ::DrawTextEx(font,"Drag to scroll",{798,418},13,1,Color{160,177,182,255});
    }
    else {
        // Field labels follow the wider projection. HUD and touch controls keep native sizes.
        blit(g_landscapeUI.texture,{0,230,540,370},{0,138,960,222});
        blit(g_landscapeUI.texture,{0,0,140,110},{0,0,140,110});
        blit(g_landscapeUI.texture,{0,110,540,120},{210,12,540,120});
        blit(g_landscapeUI.texture,{0,600,170,300},{0,240,170,300});
        blit(g_landscapeUI.texture,{170,600,370,300},{590,240,370,300});
        float noticeBottom=368;
        for(const auto& notice:g_landscapeNotices) {
            float h=notice.lines.size()*19.0f+14,top=noticeBottom-h;
            ::DrawRectangleRounded({176,top,408,h},.2f,5,Fade(Color{15,24,30,255},notice.alpha*.92f));
            ::DrawRectangleRoundedLines({176,top,408,h},.2f,5,Fade(notice.color,notice.alpha*.7f));
            for(size_t i=0;i<notice.lines.size();++i) {
                float w=::MeasureTextEx(font,notice.lines[i].c_str(),15,1).x;
                ::DrawTextEx(font,notice.lines[i].c_str(),{380-w*.5f,top+7+i*19},15,1,Fade(notice.color,notice.alpha));
            }
            noticeBottom=top-6;
        }
        int y=394;
        for(const auto& caption:g_landscapeCaptions) {
            int w=(int)::MeasureTextEx(font,caption.text.c_str(),caption.size,1).x;if(w>396)continue;
            int cy=caption.prompt ? 465:y;
            Rectangle plate={380-w*.5f-12,(float)cy-6,(float)w+24,(float)caption.size+14};
            ::DrawRectangleRounded(plate,.25f,5,Color{15,24,30,230});
            ::DrawRectangleRoundedLines(plate,.25f,5,caption.prompt ? Color{175,140,82,255}:Color{65,83,92,255});
            ::DrawTextEx(font,caption.text.c_str(),{380-w*.5f,(float)cy},caption.size,1,caption.color);
            if(!caption.prompt)y+=caption.size+24;
        }
    }
    if(g_landscapeWorld && !g_landscapeDialog)g_landscapeScroll=0;
    g_presentedMap=g_landscapeMap && g_landscapeWorld && !g_landscapeDialog;
    if(g_presentedMap) {
        blit(g_landscapeMapTarget.texture,g_landscapeMapSource,g_landscapeMapDest);
        g_presentedMapSource=g_landscapeMapSource;g_presentedMapDest=g_landscapeMapDest;
    }
    g_presentedWorld=g_landscapeWorld;g_presentedDialog=g_landscapeDialog;
    g_landscapeHitRects=controls;
    if(g_presentedMap) {
        // Its source-space control is now displayed only in the native widget.
        auto src=g_presentedMapSource;
        for(auto it=g_landscapeHitRects.begin();it!=g_landscapeHitRects.end();) {
            auto r=*it;
            if(r.x>=src.x && r.y>=src.y && r.x+r.width<=src.x+src.width && r.y+r.height<=src.y+src.height)it=g_landscapeHitRects.erase(it);
            else ++it;
        }
    }
}
#define GetMousePosition LandscapeMouse
#define GetTouchPosition LandscapeTouch
#define BeginTextureMode LandscapeTextureBegin
#define EndTextureMode LandscapeTextureEnd
#define BeginMode3D Landscape3DBegin
#define EndMode3D Landscape3DEnd
#define DrawTexturePro LandscapeTextureDraw
#define GetWorldToScreen LandscapeScreen
#define GetWorldToScreenEx LandscapeProjection
#define GetScreenToWorldRayEx LandscapeRay
