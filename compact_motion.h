#pragma once
#include <algorithm>
#include <cmath>
#include "combat_motion.h"
namespace tfcompact {
enum class AttackStyle { Slash, Heavy, Thrust, Punch, Bow };
struct AttackPose { float shoulder, elbow, sweep, twist, lean, reach, trail; };
inline float Smooth(float t) {t=std::clamp(t,0.f,1.f);return t*t*(3.f-2.f*t);}
inline AttackPose Attack(float phase, AttackStyle style) {
    const AttackPose guard{-25,-20,0,0,0,0,0};
    if(phase<0 || phase>1)return guard;
    AttackPose wind{-100,-55,-55,-18,-4,0,0};
    AttackPose hit{-65,-10,35,16,8,4,1};
    AttackPose follow{-15,-12,80,24,10,2.5f,.35f};
    float contact=tfmotion::meleeContactPhase;
    if(style==AttackStyle::Heavy){
        wind={-145,-45,-12,-10,-5,0,0};
        hit={-60,-10,5,8,12,4.5f,1};follow={-15,-5,12,12,14,3,.4f};
    }else if(style==AttackStyle::Thrust || style==AttackStyle::Punch){
        wind={-45,-90,-12,-12,-3,0,0};
        hit={-88,-3,0,12,6,5,0};follow={-72,-12,8,8,4,3,0};
    }else if(style==AttackStyle::Bow){
        contact=tfmotion::bowContactPhase;
        wind={-65,-105,0,-8,-2,0,0};
        hit={-80,-45,0,-8,0,0,0};follow={-70,-40,0,-4,0,0,0};
    }
    AttackPose a,b;float t;
    float ready=style==AttackStyle::Bow ? .48f:.24f;
    float finish=style==AttackStyle::Bow ? .76f:.58f;
    if(phase<ready){a=guard;b=wind;t=phase/ready;}
    else if(phase<contact){a=wind;b=hit;t=(phase-ready)/(contact-ready);}
    else if(phase<finish){a=hit;b=follow;t=(phase-contact)/(finish-contact);}
    else {a=follow;b=guard;t=(phase-finish)/(1-finish);}
    t=Smooth(t);
    auto mix=[&](float x,float y){return x+(y-x)*t;};
    return {mix(a.shoulder,b.shoulder),mix(a.elbow,b.elbow),mix(a.sweep,b.sweep),
            mix(a.twist,b.twist),mix(a.lean,b.lean),mix(a.reach,b.reach),mix(a.trail,b.trail)};
}
struct MiningPose { float shoulder, elbow, lean; };
// A slow lift, quick strike, short contact hold, then recovery. Endpoints
// match so repeated work never snaps back to the first pose.
inline MiningPose Mining(float seconds) {
    float phase = seconds / 1.4f - std::floor(seconds / 1.4f);
    auto ease = [](float t) { t = std::clamp(t, 0.f, 1.f); return t*t*(3.f-2.f*t); };
    float lift;
    if (phase < .42f) lift = ease(phase/.42f);
    else if (phase < .60f) lift = 1.f-ease((phase-.42f)/.18f);
    else if (phase < .70f) lift = 0.f;
    else lift = .12f*std::sin((phase-.70f)/.30f*3.14159265f);
    return {-45.f-100.f*lift, -12.f-28.f*lift, 6.f*(1.f-lift)};
}
}
