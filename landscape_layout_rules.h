#pragma once
namespace tflayout {
constexpr float width=960, height=540, legacyWidth=540, legacyHeight=900;
constexpr float baseAspectFactor=(width/height)/(legacyWidth/legacyHeight);
inline float aspectFactor=baseAspectFactor;
inline void DisplayAspect(float w,float h) {
    if(w>0 && h>0)aspectFactor=(w/h)/(legacyWidth/legacyHeight);
}
struct Point {float x,y;};
enum class Region {World,Panel,Menu,Hud,Left,Right,Field,Map,GearHeader,GearLeft,GearRight,PanelHeader};
constexpr float gearScale=440.0f/540.0f;
inline Region RegionAt(Point p,bool world,bool dialog) {
    if(!world || dialog)return Region::Panel;
    if(p.x<140 && p.y<110)return Region::Menu;
    if(p.x>=210 && p.x<=750 && p.y>=12 && p.y<132)return Region::Hud;
    if(p.x>=590 && p.y>=240)return Region::Right;
    if(p.x<170 && p.y>=240)return Region::Left;
    return Region::World;
}
inline Point Map(Point p,Region region,float scroll=0) {
    switch(region) {
        case Region::Panel:return {p.x-210,p.y+scroll};
        case Region::PanelHeader:return {p.x-210,p.y};
        case Region::GearHeader:return {p.x-210,p.y};
        case Region::GearLeft:return {(p.x-24)/gearScale,(p.y-122)/gearScale+110};
        case Region::GearRight:return {(p.x-496)/gearScale,(p.y-122)/gearScale+562};
        case Region::Menu:return p;
        case Region::Hud:return {p.x-210,p.y+98};
        case Region::Left:return {p.x,p.y+360};
        case Region::Right:return {p.x-420,p.y+360};
        default:return {p.x*legacyWidth/width,p.y*legacyHeight/height};
    }
}
inline Point WorldPoint(Point p) {return {p.x*width/legacyWidth,p.y*height/legacyHeight};}
// A wide viewport has 60% of the portrait height. Keep actors readable rather
// than preserving the portrait camera distance and shrinking the whole scene.
inline float CameraDistance(float distance) {return distance*.65f;}
constexpr float maxScroll=legacyHeight-height;
inline float Scroll(float value) {return value<0 ? 0:value>maxScroll ? maxScroll:value;}
inline Point Input(Point p,bool world,bool dialog,float scroll=0) {
    if(!world || dialog)return Map(p,p.y<110 ? Region::PanelHeader:Region::Panel,scroll);
    if(p.x<140 && p.y<110)return p; // menu
    if(p.x>=210 && p.x<=750 && p.y>=12 && p.y<132)return {p.x-210,p.y+98};
    if(p.x>=590 && p.y>=240)return {p.x-420,p.y+360}; // combat controls
    if(p.x<170 && p.y>=240)return {p.x,p.y+360}; // movement controls
    return {p.x*legacyWidth/width,p.y*legacyHeight/height};
}
}
