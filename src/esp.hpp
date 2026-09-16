#pragma once
#include "imgui.h"
#include "cs2.hpp"
#include <string>
#include <cstdio>

namespace ESP {

struct Style {
    bool showName = true;
    bool showHealth = true;
    bool showArmor = true;
    bool showDistance = true;
    bool showTeam = false; // if true, draw teammates too (dimmed)
    ImU32 enemyT = IM_COL32(255, 80, 80, 255);   // T red
    ImU32 enemyCT = IM_COL32(80, 160, 255, 255); // CT blue
    ImU32 teamDim = IM_COL32(120, 120, 120, 180);
};

inline void Render(ImDrawList* dl, const CS2Entity& e,
                   const Vector2& head2d, const Vector2& feet2d,
                   float distMeters, int localTeam, const Style& st = {}) {
    float h = feet2d.y - head2d.y;
    if (h <= 4.0f || h > 1200.0f)
        return;
    float w = h * 0.55f;
    float x = feet2d.x - w / 2.0f;
    float y = head2d.y;

    bool isTeammate = (localTeam == 2 || localTeam == 3) && (e.team == localTeam);
    if (isTeammate && !st.showTeam)
        return;

    ImU32 boxCol;
    if (isTeammate)
        boxCol = st.teamDim;
    else
        boxCol = (e.team == 2) ? st.enemyT : st.enemyCT;

    ImU32 black = IM_COL32(0, 0, 0, 255);

    // outline + box
    dl->AddRect(ImVec2(x - 1, y - 1), ImVec2(x + w + 1, y + h + 1), black, 0.0f, 0, 2.0f);
    dl->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), boxCol, 0.0f, 0, 1.5f);

    // health bar (left)
    if (st.showHealth) {
        float pct = (float)e.health / 100.0f;
        if (pct < 0) pct = 0;
        if (pct > 1) pct = 1;
        float bh = h * pct;
        float barX = x - 6.0f;
        dl->AddRectFilled(ImVec2(barX - 1, y - 1), ImVec2(barX + 3, y + h + 1), black);
        ImU32 hc = pct > 0.6f ? IM_COL32(0, 255, 0, 255)
                 : pct > 0.3f ? IM_COL32(255, 165, 0, 255)
                              : IM_COL32(255, 0, 0, 255);
        dl->AddRectFilled(ImVec2(barX, y + h - bh), ImVec2(barX + 2, y + h), hc);
        // hp number
        if (e.health < 100) {
            char hbuf[16];
            snprintf(hbuf, sizeof(hbuf), "%d", e.health);
            dl->AddText(ImVec2(barX - 8, y + h - bh - 12), IM_COL32(255, 255, 255, 255), hbuf);
        }
    }

    // armor bar (right, thin)
    if (st.showArmor && e.armor > 0) {
        float pct = (float)e.armor / 100.0f;
        if (pct > 1) pct = 1;
        float bh = h * pct;
        float barX = x + w + 3.0f;
        dl->AddRectFilled(ImVec2(barX - 1, y - 1), ImVec2(barX + 3, y + h + 1), black);
        dl->AddRectFilled(ImVec2(barX, y + h - bh), ImVec2(barX + 2, y + h), IM_COL32(80, 200, 255, 255));
    }

    // name (top)
    if (st.showName && !e.name.empty()) {
        ImVec2 ts = ImGui::CalcTextSize(e.name.c_str());
        float tx = x + w / 2.0f - ts.x / 2.0f;
        dl->AddText(ImVec2(tx + 1, y - ts.y - 2 + 1), black, e.name.c_str());
        dl->AddText(ImVec2(tx, y - ts.y - 2), IM_COL32_WHITE, e.name.c_str());
    }

    // distance + HP line (bottom)
    if (st.showDistance) {
        char dbuf[48];
        snprintf(dbuf, sizeof(dbuf), "%dhp %.0fm", e.health, distMeters);
        ImVec2 ts = ImGui::CalcTextSize(dbuf);
        float tx = x + w / 2.0f - ts.x / 2.0f;
        dl->AddText(ImVec2(tx + 1, y + h + 2 + 1), black, dbuf);
        dl->AddText(ImVec2(tx, y + h + 2), IM_COL32(200, 200, 200, 255), dbuf);
    }
}

} // namespace ESP
