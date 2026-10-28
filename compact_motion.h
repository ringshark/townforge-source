#pragma once
#include <algorithm>
#include <cmath>
namespace tfcompact {
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
