#include <algorithm>
#include <cmath>
#include <cassert>
#include <cstdio>
struct Vector2{float x,y;};struct Rectangle{float x,y,width,height;};
#include "../atlas_geometry.h"
int main(){
    constexpr float world=5760;tfatlas::View v;v.center={world/2,world/2};
    for(float zoom:{1.f,2.f,4.f,6.f}){
        v.zoom=zoom;v.Clamp(world);
        for(Vector2 p: {Vector2{0,0},Vector2{world,world},Vector2{1620,3150}}){auto q=v.Unproject(v.Project(p,world),world);assert(hypotf(q.x-p.x,q.y-p.y)<.01f);}
        Vector2 anchor=v.Project({2800,2800},world),before=v.Unproject(anchor,world);v.ZoomAt(1.1f,anchor,world);Vector2 after=v.Unproject(anchor,world);if(zoom>=2)assert(hypotf(before.x-after.x,before.y-after.y)<.01f);
    }
    v.center={-20000,20000};v.zoom=20;v.Clamp(world);assert(v.zoom==6 && v.center.x>0 && v.center.y<world);
    v.zoom=0;v.Clamp(world);assert(v.zoom==1 && fabsf(v.center.x-world/2)<.01f);
    puts("PASS atlas projection, zoom anchor, zoom limits and bounded pan");
}
