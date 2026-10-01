#pragma once
#include <algorithm>
#include <cmath>

// Small, render-independent rules shared by live wilderness and dungeon combat.
namespace tfmotion {
constexpr float swingDuration = 0.32f;
// Arena visuals have their own duration; server damage/cooldowns stay authoritative.
constexpr float arenaSwingDuration = 0.65f;
inline float ArenaSwingPhase(float elapsed) {
    return elapsed >= 0.0f && elapsed < arenaSwingDuration ? elapsed / arenaSwingDuration : -1.0f;
}
// Distance prevents standing footsteps; the time gate prevents rapid sound bursts.
struct FootstepCadence {
    float walked = 0.0f;
    double lastSound = -100.0;
    bool Step(float distance, double now) {
        if (!std::isfinite(distance) || distance < 0.01f || distance > 40.0f) { walked = 0.0f; return false; }
        walked += distance;
        if (walked < 72.0f || now - lastSound < 0.42) return false;
        walked = 0.0f; lastSound = now;
        return true;
    }
};
enum class Strike { None, Started, Impact, Whiff };
template<class Monster>
Strike StepStrike(Monster& m, bool alive, bool inRange, float dt, float cooldown, float contactPhase) {
    if (!alive) { m.pendingStrikeT = -1.0f; return Strike::None; }
    if (m.pendingStrikeT >= 0.0f) {
        m.pendingStrikeT -= std::max(0.0f, dt);
        if (m.pendingStrikeT > 0.00001f) return Strike::None;
        m.pendingStrikeT = -1.0f; // consume before damage/death can replace the target
        return inRange ? Strike::Impact : Strike::Whiff;
    }
    if (!inRange || m.playerAttackCooldown > 0.0f) return Strike::None;
    m.playerAttackCooldown = cooldown;
    m.swingEffectTimer = swingDuration;
    m.pendingStrikeT = swingDuration * contactPhase;
    return Strike::Started;
}
template<class V>
void DriveVelocity(V& v, V target, float topSpeed, float dt) {
    if (target.x == 0.0f && target.y == 0.0f) { v = {}; return; }
    // Releasing stops immediately; reversing never carries the player the wrong way.
    if (v.x * target.x + v.y * target.y < 0.0f) v = {};
    float dx = target.x - v.x, dy = target.y - v.y;
    float length = std::hypot(dx, dy);
    float amount = std::max(0.0f, topSpeed * 16.0f * dt);
    if (length <= amount || length < 0.0001f) v = target;
    else { v.x += dx / length * amount; v.y += dy / length * amount; }
}
inline float Turn(float current, float target, float dt, bool attacking) {
    float delta = std::atan2(std::sin(target - current), std::cos(target - current));
    return current + delta * (1.0f - std::exp(-std::max(0.0f, dt) * (attacking ? 32.0f : 22.0f)));
}
}
