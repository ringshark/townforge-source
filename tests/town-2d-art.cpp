#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <cassert>
#include <iostream>
#include "../town_2d_art.h"

struct Sample {Rectangle src,dst;};
static std::vector<Sample> samples;
void DrawTexturePro(Texture2D,Rectangle src,Rectangle dst,Vector2,float,Color) {
    samples.push_back({src,dst});
}
int main() {
    tf2dart::paving.id=1;
    Rectangle world{250,252,38,23},view{0,0,960,540};
    tf2dart::Band(world,{0,0},view);
    float area=0;
    for(auto& s:samples) {
        assert(s.src.x>=0 && s.src.y>=0);
        assert(s.src.x+s.src.width<=256 && s.src.y+s.src.height<=256);
        assert(s.dst.x>=250 && s.dst.y>=252);
        assert(s.dst.x+s.dst.width<=288 && s.dst.y+s.dst.height<=275);
        area+=s.dst.width*s.dst.height;
    }
    assert(area==38*23); // full coverage, no enlarged final tiles
    samples.clear();
    tf2dart::Band(world,{270,260},view);
    area=0;
    for(auto& s:samples) {
        assert(s.dst.x>=0 && s.dst.y>=0);
        assert(s.dst.x+s.dst.width<=18 && s.dst.y+s.dst.height<=15);
        area+=s.dst.width*s.dst.height;
    }
    assert(area==18*15);
    samples.clear();
    tf2dart::Band({10,10,0,20},{0,0},view);
    tf2dart::Band({-100,-100,10,10},{0,0},view);
    assert(samples.empty());
    tf2dart::paving.id=0;
    tf2dart::Band(world,{0,0},view);
    assert(samples.empty());
    std::cout<<"PASS road edges: exact tile clipping, UV bounds, camera clipping and missing-texture fallback\n";
}
