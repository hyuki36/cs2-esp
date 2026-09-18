#pragma once
#include <windows.h>
#include "vec.hpp"
#include "cs2.hpp"

// Shared config + feature ticks (bhop / aimbot / spinbot / antiaim).

struct AimCfg {
    bool enabled = true;
    int key = VK_RBUTTON;   // default: hold RightClick
    bool toggleMode = false; // false = hold, true = toggle
    float fovPx = 100.0f;    // radius around crosshair, pixels
    float smooth = 5.0f;     // 1 = snap, higher = slower
    int bone = 6;            // 6 head, 5 neck, 4 chest, 0 pelvis
    bool visCheck = true;    // only aim spotted enemies
    bool rcs = true;         // recoil control
    float rcsScale = 2.0f;
};

struct BhopCfg {
    bool enabled = true;
    int key = VK_SPACE;
    bool speedBoost = true;
    float speed = 15.0f; // 5 - 30  (target = speed * 20 units/s)
    bool showHud = true;
};

struct SpinCfg {
    bool enabled = false;
    float speed = 20.0f; // deg per frame
    float pitch = 0.0f;
};

struct AaCfg {
    bool enabled = false;
    int pitchMode = 0; // 0 off, 1 up, 2 down, 3 jitter
    int yawMode = 0;   // 0 off, 1 backwards, 2 spin, 3 jitter
    float spinSpeed = 12.0f;
};

struct Config {
    AimCfg aim;
    BhopCfg bhop;
    SpinCfg spin;
    AaCfg aa;
    int menuKey = VK_INSERT;
    bool watermark = true;
    bool showTeamEsp = false;
};

namespace Features {

void NormalizeAngles(Vector3& a);
bool KeyDown(int vk);

// Returns per-frame status for HUD.
struct FrameInfo {
    bool aimActive = false;
    float speed = 0;      // local horizontal speed u/s
    float lastTakeoff = 0;
    bool onGround = true;
};

// Runs bhop (+velocity boost). Updates info.
void BhopTick(Memory& mem, CS2& game, const Config& cfg, FrameInfo& info);

// Runs aimbot. Returns true if it aimed this frame (wins over spin/AA).
bool AimbotTick(Memory& mem, CS2& game, const Config& cfg,
                const std::vector<CS2Entity>& ents,
                const Matrix4& vm, const Vector2& screen);

// Spinbot + AntiAim (skipped automatically while aimbot is active).
void RageTick(Memory& mem, CS2& game, const Config& cfg, bool aimActive);

} // namespace Features
