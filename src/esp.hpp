#pragma once
#include "imgui.h"
#include "cs2.hpp"
#include "offsets.hpp"
#include <string>
#include <cstdio>

namespace ESP {

struct Style {
    bool showName = true;
    bool showHealth = true;
    bool showArmor = true;
    bool showDistance = true;
    bool showTeam = false; // if true, draw teammates too (dimmed)
    bool box = true;
    bool corners = false;  // corner box instead of full box
    bool skeleton = true;
    bool tracer = false;
    int tracerFrom = 0; // 0 bottom-center, 1 top-center, 2 crosshair
    bool headDot = false;
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
    if (st.box) {
        if (st.corners) {
            float cl = w * 0.25f;
            if (cl < 4) cl = 4;
            auto corner = [&](float px, float py, float dx, float dy) {
                dl->AddLine(ImVec2(px, py), ImVec2(px + dx * cl, py), boxCol, 2.0f);
                dl->AddLine(ImVec2(px, py), ImVec2(px, py + dy * cl), boxCol, 2.0f);
            };
            corner(x, y, 1, 1);
            corner(x + w, y, -1, 1);
            corner(x, y + h, 1, -1);
            corner(x + w, y + h, -1, -1);
        } else {
            dl->AddRect(ImVec2(x - 1, y - 1), ImVec2(x + w + 1, y + h + 1), black, 0.0f, 0, 2.0f);
            dl->AddRect(ImVec2(x, y), ImVec2(x + w, y + h), boxCol, 0.0f, 0, 1.5f);
        }
    }

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

// Skeleton + tracer + head dot. Needs live bone reads (heavier — toggleable).
inline void RenderExtras(ImDrawList* dl, CS2& game, const CS2Entity& e,
                         const Matrix4& vm, const Vector2& screen,
                         const Vector2& head2d, const Vector2& feet2d,
                         int localTeam, const Style& st) {
    bool isTeammate = (localTeam == 2 || localTeam == 3) && (e.team == localTeam);
    if (isTeammate && !st.showTeam)
        return;
    ImU32 col = isTeammate ? st.teamDim
        : (e.team == 2) ? st.enemyT : st.enemyCT;

    if (st.tracer) {
        ImVec2 from;
        if (st.tracerFrom == 1) from = ImVec2(screen.x / 2, 0);
        else if (st.tracerFrom == 2) from = ImVec2(screen.x / 2, screen.y / 2);
        else from = ImVec2(screen.x / 2, screen.y);
        ImVec2 to(feet2d.x, feet2d.y);
        dl->AddLine(from, to, col & IM_COL32(255, 255, 255, 160), 1.0f);
    }

    if (st.headDot)
        dl->AddCircleFilled(ImVec2(head2d.x, head2d.y), 2.5f, IM_COL32(255, 0, 0, 255));

    if (!st.skeleton)
        return;

    static const int chains[][6] = {
        { bone::HEAD, bone::NECK, bone::SPINE1, bone::SPINE2, bone::PELVIS, -1 }, // trunk
        { bone::NECK, bone::ARM_UP_L, bone::ARM_LO_L, bone::HAND_L, -1, -1 },     // arm L
        { bone::NECK, bone::ARM_UP_R, bone::ARM_LO_R, bone::HAND_R, -1, -1 },     // arm R
        { bone::PELVIS, bone::LEG_UP_L, bone::LEG_LO_L, bone::FOOT_L, -1, -1 },   // leg L
        { bone::PELVIS, bone::LEG_UP_R, bone::LEG_LO_R, bone::FOOT_R, -1, -1 },   // leg R
    };
    for (auto& ch : chains) {
        Vector2 prev{};
        bool havePrev = false;
        for (int k = 0; k < 6 && ch[k] >= 0; k++) {
            Vector3 bp = game.GetBonePos(e.pawn, ch[k]);
            if (bp.x == 0 && bp.y == 0 && bp.z == 0) { havePrev = false; continue; }
            Vector2 s{};
            if (!CS2::WorldToScreen(bp, vm, screen, s)) { havePrev = false; continue; }
            if (havePrev)
                dl->AddLine(ImVec2(prev.x, prev.y), ImVec2(s.x, s.y), col, 1.5f);
            prev = s;
            havePrev = true;
        }
    }
}

} // namespace ESP
