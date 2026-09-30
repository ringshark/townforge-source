#include "../combat_motion.h"
#include <cassert>
#include <iostream>
struct Vec { float x = 0, y = 0; };
struct Monster { float pendingStrikeT = -1, playerAttackCooldown = 0, swingEffectTimer = 0; };
int main() {
    using namespace tfmotion;
    Vec v{};
    DriveVelocity(v, Vec{220, 0}, 220, 1.0f/60);
    assert(v.x > 0 && v.x < 220); // first frame responds without snapping to full speed
    for (int i=0; i<4; ++i) DriveVelocity(v, Vec{220,0},220,1.0f/60);
    assert(std::abs(v.x - 220) < .01f);
    DriveVelocity(v, Vec{-220,0},220,1.0f/60); assert(v.x < 0);
    DriveVelocity(v, Vec{},220,1.0f/60); assert(v.x==0 && v.y==0);
    // Turning wraps through the short arc, and converges equally at 30 and 120 fps.
    float wrapped = Turn(3.13f, -3.13f, 1.0f/60, false); assert(wrapped > 3.13f);
    float low=0,high=0;
    for(int i=0;i<6;i++) low=Turn(low,1.57f,1.0f/30,false);
    for(int i=0;i<24;i++) high=Turn(high,1.57f,1.0f/120,false);
    assert(std::abs(low-high)<.0001f);
    Monster m;
    assert(StepStrike(m,true,false,.016f,.8f,.4f)==Strike::None);
    assert(StepStrike(m,true,true,.016f,.8f,.4f)==Strike::Started);
    assert(m.playerAttackCooldown==.8f && m.swingEffectTimer==swingDuration);
    assert(StepStrike(m,true,true,.06f,.8f,.4f)==Strike::None);
    assert(StepStrike(m,true,true,.07f,.8f,.4f)==Strike::Impact);
    assert(StepStrike(m,true,true,.016f,.8f,.4f)==Strike::None); // once only
    m.playerAttackCooldown=0;
    assert(StepStrike(m,true,true,.016f,.35f,.4f)==Strike::Started);
    assert(StepStrike(m,true,false,.14f,.35f,.4f)==Strike::Whiff);
    assert(StepStrike(m,true,true,.016f,.35f,.4f)==Strike::None);
    m.playerAttackCooldown=0; StepStrike(m,true,true,.016f,.8f,.4f);
    assert(StepStrike(m,false,true,.2f,.8f,.4f)==Strike::None && m.pendingStrikeT<0);
    // Automatic attacks retain DEX cadence, with a contact delay inside each cooldown.
    for(float cooldown : {.8f,.4f,.35f}) for(float fps : {30.f,60.f,120.f}) {
        Monster target; int starts=0,impacts=0; float lastStart=-10;
        for(int i=0;i<(int)(fps*4);++i) {
            target.playerAttackCooldown-=1/fps;
            auto event=StepStrike(target,true,true,1/fps,cooldown,.4f);
            if(event==Strike::Started) {
                float now=i/fps;
                if(starts) assert(now-lastStart >= cooldown-.0001f && now-lastStart <= cooldown+1/fps+.0001f);
                lastStart=now; starts++;
            }
            if(event==Strike::Impact) impacts++;
        }
        assert(starts>=5 && impacts>=starts-1 && impacts<=starts);
    }
    std::cout << "PASS movement response, reversal, stop, wrapped turning, delayed strikes, range/death cancellation and DEX cadence\n";
}
