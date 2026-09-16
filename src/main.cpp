#include <windows.h>
#include <iostream>
#include "memory.hpp"
#include "cs2.hpp"
#include "offsets.hpp"
#include "overlay.hpp"
#include "esp.hpp"
#include "imgui.h"

int main() {
    SetConsoleTitleW(L"cs2 external - ESP");

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
    // CS2 window is "Counter-Strike 2"
    if (!ov.Create(L"Counter-Strike 2")) {
        printf("[!] overlay create failed\n");
        return 1;
    }
    printf("[+] overlay %dx%d - press END to exit, INS to toggle teammates\n", ov.width, ov.height);

    ESP::Style style;
    style.showTeam = false;
    Vector2 screenSize{ (float)ov.width, (float)ov.height };
    DWORD lastSlow = 0;

    // local pawn origin for distance calc
    Vector3 localOrigin{};

    while (!(GetAsyncKeyState(VK_END) & 1)) {
        if (GetAsyncKeyState(VK_INSERT) & 1) {
            style.showTeam = !style.showTeam;
            printf("[*] showTeam=%d\n", (int)style.showTeam);
            Sleep(200);
        }

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
            // per-frame viewmatrix refresh (camera moves every frame)
            if (game.clientBase)
                mem.Read(game.clientBase + Offsets::client::dwViewMatrix, game.viewMatrix);
        }

        ov.BeginFrame();

        ImGui::SetNextWindowPos(ImVec2(0, 0));
        ImGui::SetNextWindowSize(ImVec2(screenSize.x, screenSize.y));
        ImGui::Begin("esp", nullptr,
            ImGuiWindowFlags_NoDecoration | ImGuiWindowFlags_NoBackground |
            ImGuiWindowFlags_NoInputs | ImGuiWindowFlags_NoNav | ImGuiWindowFlags_NoFocusOnAppearing);
        ImDrawList* dl = ImGui::GetWindowDrawList();

        Matrix4 vm = game.GetViewMatrix();
        auto ents = game.GetEntities(64);

        for (auto& e : ents) {
            Vector2 head2d, feet2d;
            if (!CS2::WorldToScreen(e.head, vm, screenSize, head2d)) continue;
            if (!CS2::WorldToScreen(e.feet, vm, screenSize, feet2d)) continue;
            if (head2d.x < -300 || head2d.x > screenSize.x + 300 ||
                feet2d.y < -100 || feet2d.y > screenSize.y + 300) continue;

            float distM = localOrigin.Dist(e.feet) * 0.0254f; // units->meters (1u = 1 inch)
            if (distM < 0.5f) distM = e.feet.Dist(e.head); // fallback before local cached
            ESP::Render(dl, e, head2d, feet2d, distM, game.GetLocalTeam(), style);
        }

        dl->AddCircle(ImVec2(screenSize.x / 2, screenSize.y / 2), 4.0f, IM_COL32(255, 255, 255, 255));
        char status[160];
        snprintf(status, sizeof(status), "cs2 esp | %llu | END exit INS teammates",
            (unsigned long long)ents.size());
        dl->AddText(ImVec2(10, 10), IM_COL32(0, 255, 128, 255), status);

        ImGui::End();
        ov.EndFrame();
    }

    ov.Destroy();
    mem.Detach();
    return 0;
}
