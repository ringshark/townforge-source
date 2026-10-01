#pragma once
#include "rlgl.h"
#include "landscape_layout_rules.h"
#include <vector>
#include <set>
#include <cmath>
static float g_landscapeScroll=0;
static bool g_landscapeActive=false,g_landscapeWorld=false,g_landscapeDialog=false;
static bool g_presentedWorld=false,g_presentedDialog=true,g_landscapeRedirect=false;
static RenderTexture2D g_landscapeUI{},g_landscapeScene{},g_zoomTarget{};
static std::vector<RenderTexture2D> g_landscapeTargets;
static std::set<unsigned> g_landscapeWorldTextures;
static Vector2 LandscapeMouse() {
    Vector2 raw=::GetMousePosition();auto p=tflayout::Input({raw.x,raw.y},g_presentedWorld,g_presentedDialog,g_landscapeScroll);return {p.x,p.y};
}
static Vector2 LandscapeTouch(int index) {
    Vector2 raw=::GetTouchPosition(index);auto p=tflayout::Input({raw.x,raw.y},g_presentedWorld,g_presentedDialog,g_landscapeScroll);return {p.x,p.y};
}
static void LandscapeTextureBegin(RenderTexture2D target) {
    if(g_landscapeActive && (g_landscapeTargets.empty() || g_landscapeTargets.back().id!=target.id))g_landscapeTargets.push_back(target);
    ::BeginTextureMode(target);
}
static void LandscapeTextureEnd() {
    ::EndTextureMode();
    if(g_landscapeActive && !g_landscapeTargets.empty()) {
        g_landscapeTargets.pop_back();
        if(!g_landscapeTargets.empty())::BeginTextureMode(g_landscapeTargets.back());
    }
}
static void Landscape3DBegin(Camera3D camera) {
    g_landscapeRedirect=false;
    if(g_landscapeActive && g_landscapeWorld && !g_landscapeTargets.empty()) {
        if(g_landscapeTargets.back().id==g_landscapeUI.id){rlDrawRenderBatchActive();rlEnableFramebuffer(g_landscapeScene.id);g_landscapeRedirect=true;}
        else g_landscapeWorldTextures.insert(g_landscapeTargets.back().texture.id);
    }
    ::BeginMode3D(camera);
    if(g_landscapeActive && g_landscapeWorld && camera.projection==CAMERA_PERSPECTIVE && !g_landscapeTargets.empty()) {
        const auto& target=g_landscapeTargets.back();
        double aspect=double(target.texture.width)/target.texture.height*tflayout::aspectFactor;
        double top=10*std::tan(camera.fovy*.5*DEG2RAD);
        rlMatrixMode(RL_PROJECTION);rlLoadIdentity();rlFrustum(-top*aspect,top*aspect,-top,top,10,5000);rlMatrixMode(RL_MODELVIEW);
    }
}
static void Landscape3DEnd() {
    ::EndMode3D();
    if(g_landscapeRedirect){rlDrawRenderBatchActive();rlEnableFramebuffer(g_landscapeUI.id);g_landscapeRedirect=false;}
}
static void LandscapeTextureDraw(Texture2D texture,Rectangle source,Rectangle dest,Vector2 origin,float rotation,Color tint) {
    bool world=g_landscapeActive && g_landscapeWorld && !g_landscapeTargets.empty() && g_landscapeTargets.back().id==g_landscapeUI.id && g_landscapeWorldTextures.count(texture.id);
    if(world){rlDrawRenderBatchActive();rlEnableFramebuffer(g_landscapeScene.id);}
    ::DrawTexturePro(texture,source,dest,origin,rotation,tint);
    if(world){rlDrawRenderBatchActive();rlEnableFramebuffer(g_landscapeUI.id);}
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
static void LandscapeBeginFrame(bool world,bool dialog) {
    g_landscapeWorld=world;g_landscapeDialog=dialog;g_landscapeWorldTextures.clear();g_landscapeTargets.clear();
    ::BeginTextureMode(g_landscapeScene);::ClearBackground(Color{22,33,42,255});::EndTextureMode();
    ::BeginTextureMode(g_landscapeUI);g_landscapeTargets.push_back(g_landscapeUI);g_landscapeActive=true;
}
static void LandscapePresent() {
    g_landscapeActive=false;::EndTextureMode();g_landscapeTargets.clear();
    ::BeginDrawing();::ClearBackground(Color{22,33,42,255});
    auto blit=[](Texture2D texture,Rectangle s,Rectangle d){s.y=texture.height-s.y-s.height;s.height=-s.height;::DrawTexturePro(texture,s,d,{0,0},0,WHITE);};
    if(g_landscapeWorld)blit(g_landscapeScene.texture,{0,0,540,900},{0,0,960,540});
    if(!g_landscapeWorld || g_landscapeDialog) {
        blit(g_landscapeUI.texture,{0,g_landscapeScroll,540,540},{210,0,540,540});
        Vector2 mouse=::GetMousePosition();
        Rectangle top={786,34,140,48},more={786,458,140,48};
        ::DrawRectangleRounded(top,.15f,4,Color{48,66,78,255});::DrawRectangleRounded(more,.15f,4,Color{48,66,78,255});
        ::DrawText("Top",833,49,18,Color{238,220,183,255});::DrawText("More",824,473,18,Color{238,220,183,255});
        if(::IsMouseButtonPressed(MOUSE_BUTTON_LEFT)){if(CheckCollisionPointRec(mouse,top))g_landscapeScroll=0;if(CheckCollisionPointRec(mouse,more))g_landscapeScroll=360;}
    }
    else {
        // Field labels follow the wider projection. HUD and touch controls keep native sizes.
        blit(g_landscapeUI.texture,{0,230,540,370},{0,138,960,222});
        blit(g_landscapeUI.texture,{0,0,140,110},{0,0,140,110});
        blit(g_landscapeUI.texture,{0,110,540,120},{210,12,540,120});
        blit(g_landscapeUI.texture,{0,600,170,300},{0,240,170,300});
        blit(g_landscapeUI.texture,{170,600,370,300},{590,240,370,300});
    }
    if(g_landscapeWorld && !g_landscapeDialog)g_landscapeScroll=0;
    g_presentedWorld=g_landscapeWorld;g_presentedDialog=g_landscapeDialog;
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
