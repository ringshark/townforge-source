#include "../compact_motion.h"
#include <cassert>
#include <cmath>
int main(){
    using namespace tfcompact;
    for(auto style:{AttackStyle::Slash,AttackStyle::Heavy,AttackStyle::Thrust,AttackStyle::Punch,AttackStyle::Bow}){
        const float contact=style==AttackStyle::Bow ? tfmotion::bowContactPhase:tfmotion::meleeContactPhase;
        auto guard=Attack(-1,style),start=Attack(0,style),end=Attack(1,style),hit=Attack(contact,style);
        assert(start.shoulder==guard.shoulder && end.shoulder==guard.shoulder);
        assert(start.reach==0 && end.reach==0);
        if(style!=AttackStyle::Bow)assert(hit.reach>=4);
        auto previous=start;
        for(int frame=1;frame<=1000;frame++){
            float phase=frame/1000.f;auto pose=Attack(phase,style);
            assert(std::isfinite(pose.shoulder) && std::isfinite(pose.elbow));
            assert(std::fabs(pose.shoulder-previous.shoulder)<2);
            assert(std::fabs(pose.sweep-previous.sweep)<2);
            assert(pose.reach>=0 && pose.reach<=5.001f);
            previous=pose;
        }
    }
    assert(Attack(.24f,AttackStyle::Heavy).shoulder<Attack(.24f,AttackStyle::Slash).shoulder);
    assert(Attack(tfmotion::meleeContactPhase,AttackStyle::Slash).sweep>0);
    assert(Attack(tfmotion::meleeContactPhase,AttackStyle::Thrust).elbow>-5);
    tfmotion::ImpactPause pause;pause.Trigger(.1f);
    assert(pause.remaining<=.035f);
    // A crowd's hits cannot extend the first pause or retrigger it every frame.
    for(int i=0;i<20;i++){pause.Tick(.01f);pause.Trigger(.035f);}
    assert(pause.remaining==0);
    pause.Tick(.03f);pause.Trigger(.018f);assert(pause.remaining>0);
    pause.Tick(1);assert(pause.remaining==0 && pause.gap==0);
    // A hit moves away from contact and settles to zero without a frame-zero jump.
    assert(tfmotion::Recoil(0,.25f,8)==0);
    assert(tfmotion::Recoil(.045f,.25f,8)>7.9f);
    assert(tfmotion::Recoil(.25f,.25f,8)==0);
    assert(tfmotion::Recoil(.2499f,.25f,8)<.001f);
}
