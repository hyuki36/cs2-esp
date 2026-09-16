#pragma once
#include <cstdint>
#include <cstddef>

// CS2 offsets — generated 2026-09-10 via a2x/cs2-dumper
// If ESP shows nothing after a CS2 update, re-run cs2-dumper and
// replace the client_dll values below. Schema (m_*) values change rarely.

namespace Offsets {

// ---- client.dll globals (RVA from client.dll base) ----
namespace client {
    constexpr std::ptrdiff_t dwEntityList           = 0x2577BE0;
    constexpr std::ptrdiff_t dwLocalPlayerController= 0x23A78D0;
    constexpr std::ptrdiff_t dwLocalPlayerPawn      = 0x23CCC08;
    constexpr std::ptrdiff_t dwViewMatrix           = 0x23D21F0;
    constexpr std::ptrdiff_t dwViewAngles           = 0x23E2C98;
    constexpr std::ptrdiff_t dwGameEntitySystem_highestEntityIndex = 0x2090;
}

// ---- CCSPlayerController ----
namespace controller {
    constexpr std::ptrdiff_t m_hPlayerPawn          = 0x914; // CHandle<C_CSPlayerPawn>
    constexpr std::ptrdiff_t m_iszPlayerName        = 0x6F4; // char[128]
    constexpr std::ptrdiff_t m_sSanitizedPlayerName = 0x868; // CUtlString
    constexpr std::ptrdiff_t m_iTeamNum_unused      = 0x0;   // team lives on pawn, not controller
}

// ---- C_CSPlayerPawn / C_BaseEntity ----
namespace pawn {
    constexpr std::ptrdiff_t m_pGameSceneNode = 0x330; // CGameSceneNode*
    constexpr std::ptrdiff_t m_iHealth        = 0x34C; // int32
    constexpr std::ptrdiff_t m_lifeState      = 0x354; // uint8, 0 = alive
    constexpr std::ptrdiff_t m_iTeamNum       = 0x3E7; // uint8 (2 = T, 3 = CT)
    constexpr std::ptrdiff_t m_vOldOrigin     = 0x13B8; // Vector (feet)
    constexpr std::ptrdiff_t m_ArmorValue     = 0x1CA4; // int32
}

// ---- CGameSceneNode ----
namespace scene {
    constexpr std::ptrdiff_t m_vecAbsOrigin = 0xC8; // VectorWS (fallback origin)
    constexpr std::ptrdiff_t m_bDormant     = 0x103; // bool
    // Bone array: GameSceneNode + BONE_ARRAY -> ptr to bone structs (stride 0x20, pos at +0x0)
    // Standard public value 0x1E0. If head pos reads garbage, try 0x1C0.
    constexpr std::ptrdiff_t BONE_ARRAY      = 0x1E0;
    constexpr std::ptrdiff_t BONE_ARRAY_ALT  = 0x1C0;
    constexpr int BONE_HEAD = 6;
    constexpr int BONE_NECK = 5;
}

// ---- EntityList traversal ----
namespace entitylist {
    constexpr uintptr_t ENTRY_STRIDE_BITS9 = 0x8;  // EntityList + 0x8*(idx>>9) + 0x10
    constexpr uintptr_t ENTRY_OFF          = 0x10;
    constexpr uintptr_t SUBENTRY_STRIDE    = 0x70; // + 0x70*(idx & 0x1FF)
}
}
