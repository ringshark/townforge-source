#include "raylib.h"
#include <algorithm>
#include <cmath>
#include <string>
#include <vector>
#include <cassert>
#include <cstdio>
#include "../world_visuals_2d.h"

static Rectangle lastSource{},lastDest{};
static Vector2 lastOrigin{};
void DrawTexturePro(Texture2D,Rectangle source,Rectangle dest,Vector2 origin,float,Color) {
    lastSource=source;lastDest=dest;lastOrigin=origin;
}
int main() {
    // Uneven padding and a wider stride in another cell must produce ONE crop,
    // with no per-frame zoom or position jump and no neighboring-cell sampling.
    std::vector<Color> pixels(40*60,BLANK);
    pixels[3*40+4]=WHITE;pixels[17*40+15]=WHITE;
    pixels[(20+4)*40+20+2]=WHITE;pixels[(20+18)*40+20+17]=WHITE;
    Rectangle crop=tfworldart::Bounds(pixels.data(),40,60,20,20,3,2);
    assert(crop.x==2 && crop.y==3 && crop.width==16 && crop.height==16);
    tfworldart::actors.push_back({7,crop,34});
    Texture2D tex{7,40,60,1,7};
    tfworldart::Draw(tex,{20,20,20,20},{100,100},WHITE);
    assert(lastSource.x==22 && lastSource.y==23 && lastSource.width==16);
    assert(lastDest.width==34 && lastDest.height==34);
    assert(lastDest.y-lastOrigin.y+lastDest.height==112);
    tfworldart::Draw(tex,{20,20,-20,20},{100,100},WHITE);
    assert(lastSource.x==22 && lastSource.width==-16);
    assert(lastSource.x+fabsf(lastSource.width)<=40);
    assert(lastDest.width==34 && lastDest.height==34);
    tfworldart::Draw(tex,{0,0,20,20},{100,100},WHITE,1.25f);
    assert(lastDest.height==42.5f); // death growth uses the same base scale
    auto empty=tfworldart::Bounds(nullptr,0,0,20,20,3,2);
    assert(empty.width==20 && empty.height==20);
    assert(tfworldart::ActorSize("assets/wilderness_v2/monsters/bandit.png")==tfworldart::kHeroSize);
    assert(tfworldart::ActorSize("assets/wilderness_v2/monsters/bat.png")<tfworldart::kHeroSize);
    assert(tfworldart::ActorSize("assets/wilderness_v2/creatures/dragon.png")>tfworldart::kHeroSize);
    // Wide coastal art cannot consume the walking gap between 135px grid slots.
    for(Vector2 dimensions:std::vector<Vector2>{{500,100},{100,500},{300,300}}) {
        Texture2D building{1,(int)dimensions.x,(int)dimensions.y,1,7};
        auto e=tfworldart::BuildingExtent(building);
        assert(e.x<=96 && e.y<=94);
        assert(135-e.x>=tfworldart::kHeroSize+5);
        assert(fabsf(e.x/e.y-dimensions.x/dimensions.y)<.001f);
    }
    puts("PASS stable silhouette bounds, mirrored cell safety, shared actor/death sizes and coastal building clearance");
}
