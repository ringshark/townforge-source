#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <cassert>
#include <cstdlib>
#include <cstdio>
#include "../town_2d_art.h"
#include "../scene_style_2d.h"
static std::vector<Color> captured;
Image GenImageColor(int width,int height,Color color){auto* px=(Color*)malloc(width*height*sizeof(Color));std::fill(px,px+width*height,color);return {px,width,height,1,PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};}
Texture2D LoadTextureFromImage(Image image){captured.assign((Color*)image.data,(Color*)image.data+image.width*image.height);return {1,image.width,image.height,1,image.format};}
void UnloadImage(Image image){free(image.data);}
void SetTextureFilter(Texture2D,int){}
void UnloadTexture(Texture2D){}
static std::vector<Rectangle> lines;
void DrawRectangle(int x,int y,int width,int height,Color){lines.push_back({(float)x,(float)y,(float)width,(float)height});}
bool CheckCollisionPointRec(Vector2 p,Rectangle r){return p.x>=r.x && p.x<=r.x+r.width && p.y>=r.y && p.y<=r.y+r.height;}
static float luminance(){float n=0;for(auto p:captured){assert(p.a==255);n+=(p.r+p.g+p.b)/3.f;}return n/captured.size();}
int main(){
 for(int theme=0;theme<6;theme++){
  auto floor=tfscene::DungeonSurface(theme,false);assert(floor && floor->width==128 && floor->height==128);
  float floorLight=luminance();auto first=captured;tfscene::dungeonFloors[theme]={};tfscene::DungeonSurface(theme,false);
  for(size_t i=0;i<first.size();i++)assert(first[i].r==captured[i].r && first[i].g==captured[i].g && first[i].b==captured[i].b);
  tfscene::DungeonSurface(theme,true);assert(floorLight-luminance()>20);
 }
 std::vector<Rectangle> rooms={{0,0,64,64},{64,16,64,32}};
 tfscene::RoomEdges(rooms,{0,0},{0,0,200,200});assert(!lines.empty());
 for(auto r:lines)assert(!(r.x>=57 && r.x<=64 && r.y==32 && r.width==1));
 tfscene::Unload();for(auto t:tfscene::dungeonFloors)assert(!t.id);
 puts("PASS six deterministic dungeon palettes, opaque surfaces, readable floor/wall contrast, open corridor seams and texture cleanup");
}
