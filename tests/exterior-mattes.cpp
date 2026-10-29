#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <cassert>
#include <cstdio>
#define STB_IMAGE_IMPLEMENTATION
#include "external/stb_image.h"
#include <cstdlib>
#include "../world_visuals_2d.h"
Image GenImageColor(int w,int h,Color c){auto* p=(Color*)malloc(w*h*sizeof(Color));std::fill(p,p+w*h,c);return {p,w,h,1,PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};}
Image LoadImage(const char* path){int w=0,h=0,n=0;auto* p=stbi_load(path,&w,&h,&n,4);return {p,w,h,1,PIXELFORMAT_UNCOMPRESSED_R8G8B8A8};}
void ImageFormat(Image* image,int format){assert(image->format==format);}
void UnloadImage(Image image){free(image.data);}

int main(){
 // The reported export: a tinted semi-opaque border enclosing solid white.
 Image sample=GenImageColor(9,9,Color{227,238,248,167});auto* px=(Color*)sample.data;
 for(int y=1;y<8;y++)for(int x=1;x<8;x++)px[y*9+x]=WHITE;
 for(int y=3;y<=5;y++)for(int x=3;x<=5;x++)px[y*9+x]=Color{45,35,30,255};px[4*9+4]=WHITE;
 tfworldart::CleanExteriorMatte(sample);assert(px[0].a==0 && px[10].a==0);assert(px[40].a==255 && px[40].r==255);assert(px[30].a==255);UnloadImage(sample);
 const char* frost[]={"bank","smith","furtrader","healer","provisioner"};
 const char* crag[]={"bank","smith","minersguild","healer","provisioner"};
 for(int town=0;town<2;town++)for(int i=0;i<5;i++){
  auto path=std::string("assets/")+(town?"cragmoor_buildings/":"frostmere_buildings/")+(town?crag[i]:frost[i])+".png";
  Image image=LoadImage(path.c_str());assert(image.data);tfworldart::CleanExteriorMatte(image);auto* p=(Color*)image.data;
  assert(p[image.width+1].a==0); // Exact inset white rectangle shown in the screenshot.
  int visible=0;for(int j=0;j<image.width*image.height;j++)visible+=p[j].a>240;
  assert(visible>image.width*image.height*.20f);assert(visible<image.width*image.height*.90f);
  printf("PASS transparent matte, retained silhouette: %s (%d opaque pixels)\n",path.c_str(),visible);UnloadImage(image);
 }
}
