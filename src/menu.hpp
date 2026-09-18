#pragma once
#include "imgui.h"
#include "features.hpp"
#include "esp.hpp"
#include <windows.h>
#include <string>

namespace Menu {

inline void ApplyStyle() {
    ImGuiStyle& s = ImGui::GetStyle();
    s.WindowRounding = 10.0f;
    s.FrameRounding = 6.0f;
    s.GrabRounding = 6.0f;
    s.TabRounding = 6.0f;
    s.ScrollbarRounding = 6.0f;
    s.FramePadding = ImVec2(8, 4);
    s.ItemSpacing = ImVec2(8, 6);

    ImVec4* c = s.Colors;
    c[ImGuiCol_WindowBg] = ImVec4(0.078f, 0.074f, 0.110f, 0.96f);
    c[ImGuiCol_ChildBg] = ImVec4(0.10f, 0.096f, 0.14f, 1.0f);
    c[ImGuiCol_Border] = ImVec4(0.55f, 0.35f, 1.0f, 0.35f);
    c[ImGuiCol_FrameBg] = ImVec4(0.16f, 0.15f, 0.22f, 1.0f);
    c[ImGuiCol_FrameBgHovered] = ImVec4(0.24f, 0.20f, 0.34f, 1.0f);
    c[ImGuiCol_FrameBgActive] = ImVec4(0.32f, 0.24f, 0.48f, 1.0f);
    c[ImGuiCol_TitleBg] = ImVec4(0.42f, 0.24f, 0.78f, 1.0f);
    c[ImGuiCol_TitleBgActive] = ImVec4(0.50f, 0.30f, 0.90f, 1.0f);
    c[ImGuiCol_CheckMark] = ImVec4(0.70f, 0.45f, 1.0f, 1.0f);
    c[ImGuiCol_SliderGrab] = ImVec4(0.62f, 0.38f, 1.0f, 1.0f);
    c[ImGuiCol_SliderGrabActive] = ImVec4(0.74f, 0.50f, 1.0f, 1.0f);
    c[ImGuiCol_Button] = ImVec4(0.34f, 0.20f, 0.62f, 1.0f);
    c[ImGuiCol_ButtonHovered] = ImVec4(0.46f, 0.28f, 0.78f, 1.0f);
    c[ImGuiCol_ButtonActive] = ImVec4(0.56f, 0.36f, 0.90f, 1.0f);
    c[ImGuiCol_Header] = ImVec4(0.34f, 0.20f, 0.62f, 1.0f);
    c[ImGuiCol_HeaderHovered] = ImVec4(0.46f, 0.28f, 0.78f, 1.0f);
    c[ImGuiCol_HeaderActive] = ImVec4(0.56f, 0.36f, 0.90f, 1.0f);
    c[ImGuiCol_Tab] = ImVec4(0.20f, 0.16f, 0.30f, 1.0f);
    c[ImGuiCol_TabHovered] = ImVec4(0.46f, 0.28f, 0.78f, 1.0f);
    c[ImGuiCol_TabActive] = ImVec4(0.42f, 0.24f, 0.78f, 1.0f);
    c[ImGuiCol_Text] = ImVec4(0.92f, 0.90f, 0.96f, 1.0f);
}

inline const char* KeyName(int vk) {
    switch (vk) {
    case 0: return "NONE";
    case VK_LBUTTON: return "Mouse1";
    case VK_RBUTTON: return "Mouse2";
    case VK_MBUTTON: return "Mouse3";
    case VK_XBUTTON1: return "Mouse4";
    case VK_XBUTTON2: return "Mouse5";
    case VK_SPACE: return "Space";
    case VK_INSERT: return "Insert";
    case VK_DELETE: return "Delete";
    case VK_SHIFT: return "Shift";
    case VK_CONTROL: return "Ctrl";
    case VK_MENU: return "Alt";
    default: break;
    }
    static char buf[32];
    UINT sc = MapVirtualKeyA((UINT)vk, MAPVK_VK_TO_VSC);
    if (sc && GetKeyNameTextA((LONG)(sc << 16), buf, sizeof(buf)) > 0)
        return buf;
    static char unk[16];
    snprintf(unk, sizeof(unk), "VK_%d", vk);
    return unk;
}

// Click-to-bind button. Returns true when a new key was captured.
inline bool KeyBindButton(const char* label, int* vk) {
    static int* capturing = nullptr;
    bool changed = false;
    bool isCap = (capturing == vk);

    char btnLabel[64];
    snprintf(btnLabel, sizeof(btnLabel), "%s", isCap ? "..." : KeyName(*vk));
    ImGui::PushID(label);
    if (ImGui::Button(btnLabel, ImVec2(110, 0)))
        capturing = vk;
    ImGui::PopID();

    if (isCap) {
        for (int k = 1; k < 256; k++) {
            if (k == VK_LBUTTON) continue; // keep left click for UI
            if (GetAsyncKeyState(k) & 1) {
                if (k == VK_ESCAPE) { capturing = nullptr; break; }
                if (k == VK_DELETE) *vk = 0;
                else *vk = k;
                capturing = nullptr;
                changed = true;
                break;
            }
        }
    }
    return changed;
}

inline void RenderMenu(Config& cfg, ESP::Style& esp) {
    ImGui::SetNextWindowSize(ImVec2(600, 440), ImGuiCond_FirstUseEver);
    ImGui::Begin("cs2 external", nullptr,
                 ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize);

    if (ImGui::BeginTabBar("tabs")) {
        if (ImGui::BeginTabItem("Aimbot")) {
            ImGui::Checkbox("Enable aimbot", &cfg.aim.enabled);
            ImGui::SameLine();
            ImGui::Text("Key:");
            ImGui::SameLine();
            KeyBindButton("aimkey", &cfg.aim.key);
            ImGui::Checkbox("Toggle mode (unchecked = hold)", &cfg.aim.toggleMode);
            ImGui::SliderFloat("FOV (px)", &cfg.aim.fovPx, 10.0f, 400.0f, "%.0f");
            ImGui::SliderFloat("Smooth", &cfg.aim.smooth, 1.0f, 20.0f, "%.1f");
            const char* bones[] = { "Head", "Neck", "Chest", "Pelvis" };
            const int boneIds[] = { 6, 5, 4, 0 };
            int sel = 0;
            for (int i = 0; i < 4; i++) if (cfg.aim.bone == boneIds[i]) sel = i;
            if (ImGui::Combo("Target bone", &sel, bones, 4))
                cfg.aim.bone = boneIds[sel];
            ImGui::Checkbox("Visible check (spotted only)", &cfg.aim.visCheck);
            ImGui::Checkbox("Recoil control (RCS)", &cfg.aim.rcs);
            if (cfg.aim.rcs)
                ImGui::SliderFloat("RCS scale", &cfg.aim.rcsScale, 0.5f, 3.0f, "%.1f");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Visuals")) {
            ImGui::Checkbox("2D Box", &esp.box);
            ImGui::SameLine();
            ImGui::Checkbox("Corners only", &esp.corners);
            ImGui::Checkbox("Skeleton ESP", &esp.skeleton);
            ImGui::Checkbox("Tracers", &esp.tracer);
            if (esp.tracer) {
                const char* from[] = { "Bottom", "Top", "Crosshair" };
                ImGui::Combo("Tracer from", &esp.tracerFrom, from, 3);
            }
            ImGui::Checkbox("Head dot", &esp.headDot);
            ImGui::Separator();
            ImGui::Checkbox("Name", &esp.showName);
            ImGui::Checkbox("Health bar", &esp.showHealth);
            ImGui::Checkbox("Armor bar", &esp.showArmor);
            ImGui::Checkbox("Distance", &esp.showDistance);
            ImGui::Checkbox("Show teammates", &cfg.showTeamEsp);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Misc")) {
            ImGui::Checkbox("Bhop", &cfg.bhop.enabled);
            if (cfg.bhop.enabled) {
                ImGui::SliderFloat("Speed (5-30)", &cfg.bhop.speed, 5.0f, 30.0f, "%.0f");
                ImGui::Checkbox("Speed boost (velocity)", &cfg.bhop.speedBoost);
                ImGui::Checkbox("Speed HUD", &cfg.bhop.showHud);
            }
            ImGui::Separator();
            ImGui::Checkbox("Spinbot", &cfg.spin.enabled);
            if (cfg.spin.enabled) {
                ImGui::SliderFloat("Spin speed", &cfg.spin.speed, 1.0f, 60.0f, "%.0f");
                ImGui::SliderFloat("Spin pitch", &cfg.spin.pitch, -89.0f, 89.0f, "%.0f");
            }
            ImGui::Separator();
            ImGui::Checkbox("AntiAim", &cfg.aa.enabled);
            if (cfg.aa.enabled) {
                const char* pm[] = { "Off", "Up", "Down", "Jitter" };
                const char* ym[] = { "Off", "Backwards", "Spin", "Jitter" };
                ImGui::Combo("AA pitch", &cfg.aa.pitchMode, pm, 4);
                ImGui::Combo("AA yaw", &cfg.aa.yawMode, ym, 4);
                if (cfg.aa.yawMode == 2)
                    ImGui::SliderFloat("AA spin", &cfg.aa.spinSpeed, 1.0f, 60.0f, "%.0f");
            }
            ImGui::Separator();
            ImGui::Checkbox("Watermark", &cfg.watermark);
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Keys")) {
            ImGui::Text("Menu key:");
            ImGui::SameLine();
            KeyBindButton("menukey", &cfg.menuKey);
            ImGui::Text("Aim key:");
            ImGui::SameLine();
            KeyBindButton("aimkey2", &cfg.aim.key);
            ImGui::Text("Bhop key:");
            ImGui::SameLine();
            KeyBindButton("bhopkey", &cfg.bhop.key);
            ImGui::Spacing();
            ImGui::TextWrapped("Click a key button, then press any key/mouse button. "
                               "ESC cancels, DEL clears. END exits the cheat.");
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
    ImGui::End();
}

} // namespace Menu
