#pragma once
namespace tflayout {
constexpr float width=960, height=540, legacyWidth=540, legacyHeight=900;
constexpr float aspectFactor=(width/height)/(legacyWidth/legacyHeight);
struct Point {float x,y;};
inline Point WorldPoint(Point p) {return {p.x*width/legacyWidth,p.y*height/legacyHeight};}
// A wide viewport has 60% of the portrait height. Keep actors readable rather
// than preserving the portrait camera distance and shrinking the whole scene.
inline float CameraDistance(float distance) {return distance*.65f;}
constexpr float maxScroll=legacyHeight-height;
inline float Scroll(float value) {return value<0 ? 0:value>maxScroll ? maxScroll:value;}
inline Point Input(Point p,bool world,bool dialog,float scroll=0) {
    if(!world || dialog)return {p.x-(width-legacyWidth)*.5f,p.y+scroll};
    if(p.x<140 && p.y<110)return p; // menu
    if(p.x>=210 && p.x<=750 && p.y>=12 && p.y<132)return {p.x-210,p.y+98};
    if(p.x>=590 && p.y>=240)return {p.x-420,p.y+360}; // combat controls
    if(p.x<170 && p.y>=240)return {p.x,p.y+360}; // movement controls
    return {p.x*legacyWidth/width,p.y*legacyHeight/height};
}
}
