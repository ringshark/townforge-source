#include "../landscape_layout_rules.h"
#include <cassert>
#include <cmath>
int main(){
 using namespace tflayout;
 auto near=[](float a,float b){return std::fabs(a-b)<.01f;};
 auto menu=Input({72,76},true,false);assert(near(menu.x,72)&&near(menu.y,76));
 auto spell=Input({687,432},true,false);assert(near(spell.x,267)&&near(spell.y,792));
 auto stick=Input({85,455},true,false);assert(near(stick.x,85)&&near(stick.y,815));
 auto hud=Input({480,72},true,false);assert(near(hud.x,270)&&near(hud.y,170));
 auto field=Input({400,200},true,false);assert(near(field.x,225)&&near(field.y,333.333f));
 auto panel=Input({480,270},true,true);assert(near(panel.x,270)&&near(panel.y,270));
 auto legacy=Input({480,270},false,false);assert(near(legacy.x,270)&&near(legacy.y,270));
 auto bottom=Input({480,270},true,true,360);assert(near(bottom.y,630));
 // Smooth menu scroll retains exact picking at intermediate offsets.
 auto middle=Input({480,270},true,true,Scroll(180));assert(near(middle.x,270)&&near(middle.y,450));
 assert(near(Scroll(-80),0)&&near(Scroll(999),360));
 // Status is above the resting stick and still follows the control transform.
 auto status=Input({85,275},true,false);assert(near(status.x,85)&&near(status.y,635));
 assert(near((540.f/790)*aspectFactor,960.f/(790*.6f)));
}
