#include <algorithm>
#include <cmath>
#include <cassert>
#include <cstdio>
#include <vector>
struct Vector2{float x,y;};struct Color{unsigned char r,g,b,a;};struct Image{void* data;int width,height;};
#include "../atlas_raster.h"
int main(){
 std::vector<Color> data(18,{11,22,33,255});Image im{data.data()+1,4,4};
 tfraster::Line(im,{-20,-20},{20,20},3,{255,200,100,255});
 assert(data.front().r==11 && data.back().r==11);assert(data[1].r==255 && data[16].r==255);
 tfraster::Line(im,{1,1},{1,1},2,{0,0,0,255});assert(data.front().r==11 && data.back().r==11);
 Color p=data[1];tfraster::Pixel(im,-1,0,{255,0,0,255});assert(data[1].r==p.r);
 tfraster::Pixel(im,0,0,{255,255,255,0});assert(data[1].r==p.r);
 puts("PASS map raster bounds, clipped and zero-length roads, and alpha blending");
}
