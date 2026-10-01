#pragma once
#include <cmath>
#include <algorithm>
#include <cstdint>

// Emberhold's reference square. Baked once into the existing ground map;
// no new textures, per-frame geometry, or collision surfaces.
namespace tfsquare {
inline float Hash(int x,int y) {
    uint32_t h=uint32_t(x)*374761393u+uint32_t(y)*668265263u;
    h=(h^(h>>13))*1274126177u;
    return float(h^(h>>16))/float(UINT32_MAX);
}
inline Color Paving(float x,float z,float radius,float noise) {
    const float d=std::sqrt(x*x+z*z);
    // A radial soldier course frames larger, staggered limestone flags.
    const bool border=d>radius-11;
    float u,v;int ix,iz;
    if(border) {
        float angle=std::atan2(z,x)+3.14159265f;
        float blocks=angle*(64.0f/(2*3.14159265f));
        ix=int(std::floor(blocks));iz=99;
        u=(blocks-std::floor(blocks))*12;v=d-(radius-11);
    } else {
        float row=std::floor(z/14);
        float col=(x+((int(row)&1)?11.0f:0.0f))/22;
        ix=int(std::floor(col));iz=int(row);
        u=(col-std::floor(col))*22;v=z-row*14;
    }
    float edge=std::min(std::min(u,(border?12.0f:22.0f)-u),std::min(v,(border?11.0f:14.0f)-v));
    float mortar=1-std::clamp(edge/1.3f,0.0f,1.0f);
    float variation=(Hash(ix,iz)-.5f)*18+(noise-.5f)*7;
    float inset=std::abs(d-(radius-12))<1.5f?10.0f:0.0f;
    float shade=variation-mortar*27-inset;
    auto channel=[&](float c){return (unsigned char)std::clamp(c+shade,0.0f,255.0f);};
    return {channel(border?161:185),channel(border?151:177),channel(border?131:155),0};
}
}
