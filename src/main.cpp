#include <windows.h>
#include <iostream>
#include "memory.hpp"
#include "cs2.hpp"
#include "offsets.hpp"
#include "overlay.hpp"
#include "esp.hpp"
#include "features.hpp"
#include "menu.hpp"
#include "imgui.h"

int main() {
    SetConsoleTitleW(L"cs2 external");

    Memory mem;
    printf("[*] waiting for cs2.exe (client.dll)...\n");
    while (!mem.Attach(L"cs2.exe", L"client.dll")) {
        printf("[!] OpenProcess / module failed. Run as admin, start CS2 first. retry in 2s...\n");
        Sleep(2000);
    }
    printf("[+] attached pid=%lu client=0x%llx\n", mem.pid, (unsigned long long)mem.base);

    CS2 game(&mem);
    if (!game.Update() || !game.entityList) {
        printf("[!] bad offsets - update src/offsets.hpp after CS2 update (cs2-dumper).\n");
    } else {
        printf("[+] entityList=0x%llx localPawn=0x%llx\n",
            (unsigned long long)game.entityList, (unsigned long long)game.localPawn);
    }

    Overlay ov;
    if (!ov.Create(L"Counter-Strike 2")) {
        printf("[!] overlay create failed\n");
        return 1;
    }
    printf("[+] overlay %dx%d - INSERT menu, END exit\n", ov.width, ov.height);

    Menu::ApplyStyle();

    Config cfg;
    ESP::Style style;
    style.showTeam = false;

    bool menuOpen = false;
    Vector2 screenSize{ (float)ov.width, (float)ov.height };
    DWORD lastSlow = 0;
    Vector3 localOrigin{};
    Features::FrameInfo info{};

    while (!(GetAsyncKeyState(VK_END) & 1)) {
        if (cfg.menuKey > 0 && (GetAsyncKeyState(cfg.menuKey) & 1)) {
            menuOpen = !menuOpen;
            ov.SetClickable(menuOpen);
            printf("[*] menu=%d\n", (int)menuOpen);
            Sleep(200);
        }
        style.showTeam = cfg.showTeamEsp;

        DWORD now = GetTickCount();
        if (now - lastSlow > 1000) {
            lastSlow = now;
            uint8_t probe = 0;
            if (!mem.IsValid() || !mem.Read(mem.base, probe)) {
                printf("[!] lost process, re-attaching...\n");
                mem.Detach();
                while (!mem.Attach(L"cs2.exe", L"client.dll")) Sleep(2000);
                printf("[+] re-attached pid=%lu client=0x%llx\n", mem.pid, (unsigned long long)mem.base);
            }
            game.Update();
            ov.UpdateBounds();
            screenSize = { (float)ov.width, (float)ov.height };

            if (game.localPawn)
                localOrigin = mem.ReadValue<Vector3>(game.localPawn + Offsets::pawn::m_vOldOrigin, {});
        } else {
            if (game.clientBase)
                mem.Read(game.clientBase + Offsets::client::dwViewMatrix, game.viewMatrix);
        }

        // ---- features (skip aim movement while menu is open) ----
        auto ents = game.GetEntities(64);
        Features::BhopTick(mem, game, cfg, info);
        bool aimActive = false;
        if (!menuOpen)
            aimActive = Features::AimbotTick(mem, game, cfg, ents,
                                             game.GetViewMatrix(), screenSize);
        if (!menuOpen)
            Features::RageTick(mem, game, cfg, aimActive);

        // ---- draw ----
        ov.BeginFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(screenSize.x, screenSize.y));
        ImGui::Begin("esp", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing);
        ImDrawList* dl = ImGui::GetWindowDrawList();

        Matrix4 vm = game.GetViewMatrix();

        for (auto& e : ents) {
            Vector2 head2d, feet2d;
            if (!CS2::WorldToScreen(e.head, vm, screenSize, head2d)) continue;
            if (!CS2::WorldToScreen(e.feet, vm, screenSize, feet2d)) continue;
            if (head2d.x < -300 || head2d.x > screenSize.x + 300 ||
                feet2d.y < -100 || feet2d.y > screenSize.y + 300) continue;

            float distM = localOrigin.Dist(e.feet) * 0.0254f;
            if (distM < 0.5f) distM = e.feet.Dist(e.head);
            ESP::Render(dl, e, head2d, feet2d, distM, game.GetLocalTeam(), style);
            ESP::RenderExtras(dl, game, e, vm, screenSize, head2d, feet2d,
                              game.GetLocalTeam(), style);
        }

        dl->AddCircle(ImVec2(screenSize.x / 2, screenSize.y / 2), 4.0f, IM_COL32(255, 255, 255, 255));

        // bhop speed HUD (bottom-center, like movement recorders)
        if (cfg.bhop.showHud && cfg.bhop.enabled) {
            char hud[96];
            snprintf(hud, sizeof(hud), "%.0f (%.0f)", (double)info.speed, (double)info.lastTakeoff);
            ImVec2 ts = ImGui::CalcTextSize(hud);
            float hx = screenSize.x / 2 - ts.x / 2;
            float hy = screenSize.y - 70;
            dl->AddText(ImVec2(hx + 1, hy + 1), IM_COL32(0, 0, 0, 255), hud);
            dl->AddText(ImVec2(hx, hy), IM_COL32(190, 130, 255, 255), hud);
            const char* st = info.onGround ? "ground" : "air";
            ImVec2 ts2 = ImGui::CalcTextSize(st);
            dl->AddText(ImVec2(screenSize.x / 2 - ts2.x / 2, hy + 20),
                        IM_COL32(160, 160, 160, 255), st);
        }

        if (cfg.watermark) {
            char wm[128];
            snprintf(wm, sizeof(wm), "cs2ext | %llu ent | %s",
                (unsigned long long)ents.size(), menuOpen ? "menu open" : "INSERT menu");
            dl->AddText(ImVec2(10, 10), IM_COL32(178, 132, 255, 255), wm);
        }

        ImGui::End();

        if (menuOpen)
            Menu::RenderMenu(cfg, style);

        ov.EndFrame();
    }

    ov.Destroy();
    mem.Detach();
    return 0;
}
