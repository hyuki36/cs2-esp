#include "features.hpp"
#include "offsets.hpp"
#include <cmath>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

namespace Features {

void NormalizeAngles(Vector3& a) {
    while (a.y > 180.0f) a.y -= 360.0f;
    while (a.y < -180.0f) a.y += 360.0f;
    if (a.x > 89.0f) a.x = 89.0f;
    if (a.x < -89.0f) a.x = -89.0f;
    a.z = 0.0f;
}

bool KeyDown(int vk) {
    if (vk <= 0) return false;
    return (GetAsyncKeyState(vk) & 0x8000) != 0;
}

static void SendSpace(bool down) {
    INPUT in_{};
    in_.type = INPUT_KEYBOARD;
    in_.ki.wVk = VK_SPACE;
    in_.ki.dwFlags = down ? 0 : KEYEVENTF_KEYUP;
    SendInput(1, &in_, sizeof(in_));
}

void BhopTick(Memory& mem, CS2& game, const Config& cfg, FrameInfo& info) {
    static bool spaceDown = false;
    static bool wasGround = true;

    uintptr_t lp = game.GetLocalPawn();
    if (!lp) return;

    bool ground = game.IsOnGround(lp);
    Vector3 vel = game.GetVelocity(lp);
    float hspeed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
    info.speed = hspeed;
    info.onGround = ground;

    // takeoff detection for HUD ("last jump" speed)
    if (wasGround && !ground)
        info.lastTakeoff = hspeed;
    wasGround = ground;

    if (!cfg.bhop.enabled || !KeyDown(cfg.bhop.key)) {
        if (spaceDown) { SendSpace(false); spaceDown = false; }
        return;
    }

    if (ground) {
        if (!spaceDown) { SendSpace(true); spaceDown = true; }
    } else {
        if (spaceDown) { SendSpace(false); spaceDown = false; }
        // velocity boost toward slider target (5-30 -> 100-600 u/s)
        if (cfg.bhop.speedBoost && hspeed > 50.0f) {
            float target = cfg.bhop.speed * 20.0f;
            if (hspeed < target) {
                float k = target / hspeed;
                vel.x *= k;
                vel.y *= k;
                game.SetVelocity(lp, vel);
            }
        }
    }
}

static void CalcAimAngles(const Vector3& eye, const Vector3& dst, Vector3& out) {
    Vector3 d = dst - eye;
    float hyp = std::sqrt(d.x * d.x + d.y * d.y);
    out.y = (float)(std::atan2(d.y, d.x) * 180.0 / M_PI);
    out.x = (float)(-std::atan2(d.z, hyp) * 180.0 / M_PI);
    out.z = 0.0f;
}

bool AimbotTick(Memory& mem, CS2& game, const Config& cfg,
                const std::vector<CS2Entity>& ents,
                const Matrix4& vm, const Vector2& screen) {
    static bool toggled = false;
    static bool lastKey = false;
    static Vector3 oldPunch{};

    if (!cfg.aim.enabled)
        return false;

    bool key = KeyDown(cfg.aim.key);
    if (cfg.aim.toggleMode) {
        if (key && !lastKey) toggled = !toggled;
        lastKey = key;
        if (!toggled) { oldPunch = {}; return false; }
    } else if (!key) {
        oldPunch = {};
        return false;
    }

    Vector3 eye = game.GetEyePos();
    if (eye.x == 0 && eye.y == 0 && eye.z == 0)
        return false;

    // pick target closest to crosshair inside FOV
    const CS2Entity* best = nullptr;
    Vector3 bestAim{};
    float bestDist = cfg.aim.fovPx;
    Vector2 cx{ screen.x / 2.0f, screen.y / 2.0f };

    for (auto& e : ents) {
        if (e.team == game.GetLocalTeam()) continue;
        if (cfg.aim.visCheck && !e.spotted) continue;
        Vector3 bp = game.GetBonePos(e.pawn, cfg.aim.bone);
        bool boneOk = !(bp.x == 0 && bp.y == 0 && bp.z == 0);
        Vector3 dst = boneOk ? bp : e.head;
        Vector2 s{};
        if (!CS2::WorldToScreen(dst, vm, screen, s)) continue;
        float dx = s.x - cx.x, dy = s.y - cx.y;
        float d = std::sqrt(dx * dx + dy * dy);
        if (d < bestDist) {
            bestDist = d;
            best = &e;
            CalcAimAngles(eye, dst, bestAim);
        }
    }
    if (!best) { oldPunch = {}; return false; }

    Vector3 cur{};
    if (!game.ReadAngles(cur)) return false;
    NormalizeAngles(bestAim);

    float smooth = cfg.aim.smooth < 1.0f ? 1.0f : cfg.aim.smooth;
    Vector3 delta = bestAim - cur;
    while (delta.y > 180.0f) delta.y -= 360.0f;
    while (delta.y < -180.0f) delta.y += 360.0f;
    Vector3 out{ cur.x + delta.x / smooth, cur.y + delta.y / smooth, 0 };

    // standalone RCS: compensate punch delta
    if (cfg.aim.rcs && game.GetShotsFired() > 1) {
        Vector3 punch = game.GetPunch();
        out.x -= (punch.x - oldPunch.x) * cfg.aim.rcsScale;
        out.y -= (punch.y - oldPunch.y) * cfg.aim.rcsScale;
        oldPunch = punch;
    } else {
        oldPunch = game.GetPunch();
    }

    NormalizeAngles(out);
    game.WriteAngles(out);
    return true;
}

void RageTick(Memory& mem, CS2& game, const Config& cfg, bool aimActive) {
    static float spinYaw = 0.0f;
    static bool flip = false;

    if (aimActive) return; // aimbot wins

    bool wantSpin = cfg.spin.enabled;
    bool wantAA = cfg.aa.enabled;
    if (!wantSpin && !wantAA) return;

    Vector3 cur{};
    if (!game.ReadAngles(cur)) return;

    if (wantSpin) {
        spinYaw += cfg.spin.speed;
        if (spinYaw > 180.0f) spinYaw -= 360.0f;
        cur.y = spinYaw;
        cur.x = cfg.spin.pitch;
    } else if (wantAA) {
        flip = !flip;
        switch (cfg.aa.pitchMode) {
        case 1: cur.x = 89.0f; break;
        case 2: cur.x = -89.0f; break;
        case 3: cur.x = flip ? 89.0f : -89.0f; break;
        default: break;
        }
        switch (cfg.aa.yawMode) {
        case 1: cur.y += 180.0f; break;
        case 2:
            spinYaw += cfg.aa.spinSpeed;
            if (spinYaw > 180.0f) spinYaw -= 360.0f;
            cur.y = spinYaw;
            break;
        case 3: cur.y += flip ? 90.0f : -90.0f; break;
        default: break;
        }
    }

    NormalizeAngles(cur);
    game.WriteAngles(cur);
}

} // namespace Features
