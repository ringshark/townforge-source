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
 auto panel=Input({480,270},true,true);assert(near(panel.x,270)&&near(panel.y,202.5));
 auto legacy=Input({480,270},false,false);assert(near(legacy.x,270)&&near(legacy.y,202.5));
 auto bottom=Input({480,270},true,true,360);assert(near(bottom.y,562.5));
 // Smooth menu scroll retains exact picking at intermediate offsets.
 auto middle=Input({480,270},true,true,Scroll(180));assert(near(middle.x,270)&&near(middle.y,382.5));
 assert(near(Scroll(-80),0)&&near(Scroll(999),maxScroll));
 // Status is above the resting stick and still follows the control transform.
 auto status=Input({85,275},true,false);assert(near(status.x,85)&&near(status.y,635));
 assert(near((540.f/790)*aspectFactor,960.f/(790*.6f)));
 // Native nameplates stay centered on the projected building across the frame.
 auto center=WorldPoint({270,450});assert(near(center.x,480)&&near(center.y,270));
 auto corner=WorldPoint({540,900});assert(near(corner.x,960)&&near(corner.y,540));
 // At default zoom, landscape restores >90% of portrait actor height.
 float portraitHeight=900/650.f,wideHeight=540/CameraDistance(650);
 assert(wideHeight/portraitHeight>.9f && wideHeight/portraitHeight<1.0f);
 auto chat=Input({70,28},true,false);assert(near(chat.x,70)&&near(chat.y,28));
 auto drag=Map({195,455},Region::Left);assert(near(drag.x,195)&&near(drag.y,815));
 auto ground=Map({700,420},Region::World);assert(near(ground.x,393.75f)&&near(ground.y,700));
 // A full-phone camera uses the displayed aspect, preserving world proportions.
 DisplayAspect(844,390);assert(near((540.f/900)*aspectFactor,844.f/390));
 float phoneAspect=aspectFactor;DisplayAspect(0,0);assert(near(aspectFactor,phoneAspect));
 DisplayAspect(960,540);assert(near(aspectFactor,baseAspectFactor));
 auto nav=Map({480,76},Region::PanelHeader,360);assert(near(nav.x,270)&&near(nav.y,57));
 auto bodyDrag=Map({480,76},Region::Panel,360);assert(near(bodyDrag.y,417));
 auto gear=Map({24+98*gearScale,122+(134-110)*gearScale},Region::GearLeft);
 assert(near(gear.x,98)&&near(gear.y,134));
 auto details=Map({496+270*gearScale,122+(850-562)*gearScale},Region::GearRight);
 assert(near(details.x,270)&&near(details.y,850));
 assert(122+452*gearScale<540 && 496+440<960);
}
