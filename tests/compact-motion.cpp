#include "../compact_motion.h"
#include <cassert>
#include <cmath>
int main() {
    using tfcompact::Mining;
    auto rest=Mining(0), top=Mining(1.4f*.42f), hit=Mining(1.4f*.60f);
    assert(top.shoulder < rest.shoulder-99);
    assert(std::fabs(hit.shoulder-rest.shoulder)<.001f);
    assert(top.elbow < hit.elbow && top.lean < hit.lean);
    // Repeating a stroke has continuous joints, including at the loop seam.
    auto previous=Mining(0);
    for(int frame=1;frame<=14000;++frame) {
        auto pose=Mining(frame*.001f);
        assert(std::isfinite(pose.shoulder) && std::isfinite(pose.elbow));
        assert(std::fabs(pose.shoulder-previous.shoulder)<.7f);
        assert(pose.shoulder>=-145.001f && pose.shoulder<=-44.999f);
        previous=pose;
    }
    // Return has ten times the sampling period of tolerable loop error.
    assert(std::fabs(Mining(1.4f-.0001f).shoulder-rest.shoulder)<.01f);
}
