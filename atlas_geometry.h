#pragma once
namespace tfatlas {
static constexpr float kWorld = 1.f, kMinZoom=1.f, kMaxZoom=6.f;
static constexpr Rectangle kCanvas={20,76,678,412};
struct View {
    Vector2 center{};float zoom=1;
    float Scale(float world)const{return kCanvas.height/world*zoom;}
    Vector2 Project(Vector2 p,float world)const{float s=Scale(world);return {kCanvas.x+kCanvas.width/2+(p.x-center.x)*s,kCanvas.y+kCanvas.height/2+(p.y-center.y)*s};}
    Vector2 Unproject(Vector2 p,float world)const{float s=Scale(world);return {center.x+(p.x-kCanvas.x-kCanvas.width/2)/s,center.y+(p.y-kCanvas.y-kCanvas.height/2)/s};}
    void Clamp(float world){
        zoom=std::clamp(zoom,kMinZoom,kMaxZoom);float s=Scale(world),hx=kCanvas.width/(2*s),hy=kCanvas.height/(2*s);
        center.x=hx>=world/2?world/2:std::clamp(center.x,hx,world-hx);
        center.y=hy>=world/2?world/2:std::clamp(center.y,hy,world-hy);
    }
    void ZoomAt(float factor,Vector2 anchor,float world){Vector2 before=Unproject(anchor,world);zoom*=factor;Clamp(world);Vector2 after=Unproject(anchor,world);center.x+=before.x-after.x;center.y+=before.y-after.y;Clamp(world);}
};
}
