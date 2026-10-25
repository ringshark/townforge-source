#pragma once
// Bounded RGBA strokes for the atlas bake. No image format conversion or
// intermediate triangle images while painting many short road segments.
namespace tfraster {
inline void Pixel(Image& image,int x,int y,Color c) {
    if(x<0 || y<0 || x>=image.width || y>=image.height)return;
    auto& p=((Color*)image.data)[y*image.width+x];unsigned a=c.a,b=255-a;
    p={(unsigned char)((p.r*b+c.r*a)/255),(unsigned char)((p.g*b+c.g*a)/255),(unsigned char)((p.b*b+c.b*a)/255),255};
}
inline void Dot(Image& image,Vector2 center,float radius,Color color) {
    int x0=std::max(0,(int)floorf(center.x-radius)),x1=std::min(image.width-1,(int)ceilf(center.x+radius));
    int y0=std::max(0,(int)floorf(center.y-radius)),y1=std::min(image.height-1,(int)ceilf(center.y+radius));
    for(int y=y0;y<=y1;y++)for(int x=x0;x<=x1;x++) {
        float d=hypotf(x+.5f-center.x,y+.5f-center.y);
        if(d<=radius+.5f){Color c=color;c.a=(unsigned char)(color.a*std::clamp(radius+.5f-d,0.f,1.f));Pixel(image,x,y,c);}
    }
}
inline void Line(Image& image,Vector2 a,Vector2 b,float width,Color color) {
    float length=hypotf(b.x-a.x,b.y-a.y);
    int steps=std::max(1,(int)ceilf(length*1.5f));
    for(int i=0;i<=steps;i++){float t=(float)i/steps;Dot(image,{a.x+(b.x-a.x)*t,a.y+(b.y-a.y)*t},width*.5f,color);}
}
}
