#include "cs2.hpp"
#include "offsets.hpp"
#include <cstring>

using namespace Offsets;

bool CS2::Update() {
    if (!mem || !mem->IsValid())
        return false;

    clientBase = mem->base; // attached with module = client.dll
    if (!clientBase)
        return false;

    entityList = mem->ReadValue<uintptr_t>(clientBase + client::dwEntityList, 0);
    localController = mem->ReadValue<uintptr_t>(clientBase + client::dwLocalPlayerController, 0);
    localPawn = mem->ReadValue<uintptr_t>(clientBase + client::dwLocalPlayerPawn, 0);

    mem->Read(clientBase + client::dwViewMatrix, viewMatrix);

    localTeam = 0;
    if (localPawn)
        localTeam = mem->ReadValue<uint8_t>(localPawn + pawn::m_iTeamNum, 0);

    return entityList != 0;
}

Vector3 CS2::GetBonePos(uintptr_t pawnAddr, int boneIdx) {
    Vector3 out{};
    if (!mem || !pawnAddr)
        return out;

    uintptr_t scene = mem->ReadValue<uintptr_t>(pawnAddr + pawn::m_pGameSceneNode, 0);
    if (!scene)
        return out;

    // try primary then alt offset (survives small version shifts)
    for (auto off : { scene::BONE_ARRAY, scene::BONE_ARRAY_ALT }) {
        uintptr_t bones = mem->ReadValue<uintptr_t>(scene + off, 0);
        if (!bones || bones < 0x10000)
            continue;
        Vector3 pos = mem->ReadValue<Vector3>(bones + (uintptr_t)boneIdx * 0x20, {});
        // sanity: bones near map origin, not NaN, not absurd
        if (pos.x != 0 || pos.y != 0 || pos.z != 0) {
            if (pos.x > -4000 && pos.x < 4000 &&
                pos.y > -4000 && pos.y < 4000 &&
                pos.z > -2000 && pos.z < 2000) {
                return pos;
            }
        }
    }
    return {};
}

std::string CS2::ReadControllerName(uintptr_t controller) {
    if (!mem || !controller)
        return {};
    // m_iszPlayerName is inline char[128] — most reliable external
    char buf[128] = {};
    if (mem->ReadRaw(controller + controller::m_iszPlayerName, buf, sizeof(buf) - 1)) {
        buf[127] = '\0';
        size_t len = strnlen(buf, sizeof(buf));
        if (len > 0 && len < 64)
            return std::string(buf, len);
    }
    // fallback: m_sSanitizedPlayerName (CUtlString: len + ptr)
    // layout: +0x0 char* buffer-ish, dump-dependent; try pointer read
    uintptr_t namePtr = mem->ReadValue<uintptr_t>(controller + controller::m_sSanitizedPlayerName, 0);
    if (namePtr && namePtr > 0x10000) {
        char b2[64] = {};
        if (mem->ReadRaw(namePtr, b2, sizeof(b2) - 1)) {
            b2[63] = '\0';
            size_t len = strnlen(b2, sizeof(b2));
            if (len > 0 && len < 64)
                return std::string(b2, len);
        }
    }
    return {};
}

std::vector<CS2Entity> CS2::GetEntities(int maxPlayers) {
    std::vector<CS2Entity> out;
    if (!mem || !entityList)
        return out;
    out.reserve(maxPlayers);

    for (int i = 1; i <= maxPlayers; i++) {
        uintptr_t listEntry = mem->ReadValue<uintptr_t>(
            entityList + entitylist::ENTRY_STRIDE_BITS9 * (uintptr_t)(i >> 9) + entitylist::ENTRY_OFF, 0);
        if (!listEntry)
            continue;

        uintptr_t controller = mem->ReadValue<uintptr_t>(
            listEntry + entitylist::SUBENTRY_STRIDE * (uintptr_t)(i & 0x1FF), 0);
        if (!controller || controller == localController)
            continue;

        uint32_t pawnHandle = mem->ReadValue<uint32_t>(controller + controller::m_hPlayerPawn, 0);
        if (!pawnHandle)
            continue;

        uintptr_t pawnEntry = mem->ReadValue<uintptr_t>(
            entityList + entitylist::ENTRY_STRIDE_BITS9 * (uintptr_t)((pawnHandle & 0x7FFF) >> 9) + entitylist::ENTRY_OFF, 0);
        if (!pawnEntry)
            continue;

        uintptr_t pawnAddr = mem->ReadValue<uintptr_t>(
            pawnEntry + entitylist::SUBENTRY_STRIDE * (uintptr_t)(pawnHandle & 0x1FF), 0);
        if (!pawnAddr || pawnAddr == localPawn)
            continue;

        int health = mem->ReadValue<int>(pawnAddr + pawn::m_iHealth, 0);
        if (health <= 0 || health > 100)
            continue;

        uint8_t life = mem->ReadValue<uint8_t>(pawnAddr + pawn::m_lifeState, 1);
        if (life != 0)
            continue;

        // dormant check via scene node
        uintptr_t scene = mem->ReadValue<uintptr_t>(pawnAddr + pawn::m_pGameSceneNode, 0);
        if (scene) {
            uint8_t dormant = mem->ReadValue<uint8_t>(scene + scene::m_bDormant, 0);
            if (dormant)
                continue;
        }

        int team = (int)mem->ReadValue<uint8_t>(pawnAddr + pawn::m_iTeamNum, 0);
        if (team != 2 && team != 3)
            continue;

        Vector3 feet = mem->ReadValue<Vector3>(pawnAddr + pawn::m_vOldOrigin, {});
        if (feet.x == 0 && feet.y == 0 && feet.z == 0) {
            // fallback to abs origin
            if (scene)
                feet = mem->ReadValue<Vector3>(scene + scene::m_vecAbsOrigin, {});
            if (feet.x == 0 && feet.y == 0 && feet.z == 0)
                continue;
        }

        Vector3 head = GetBonePos(pawnAddr, scene::BONE_HEAD);
        if (head.x == 0 && head.y == 0 && head.z == 0) {
            head = feet;
            head.z += 72.0f; // standing eye approx
        } else {
            head.z += 8.0f; // pad above skull for box top
        }

        // sanity: head above feet, not absurd height
        float h = head.z - feet.z;
        if (h < 30.0f || h > 120.0f) {
            head = feet;
            head.z += 72.0f;
        }

        CS2Entity e;
        e.controller = controller;
        e.pawn = pawnAddr;
        e.feet = feet;
        e.head = head;
        e.health = health;
        e.team = team;
        e.armor = mem->ReadValue<int>(pawnAddr + pawn::m_ArmorValue, 0);
        e.name = ReadControllerName(controller);
        e.valid = true;
        out.push_back(e);
    }
    return out;
}

bool CS2::WorldToScreen(const Vector3& world, const Matrix4& vm, Vector2 screenSize, Vector2& out) {
    float x = vm.m[0] * world.x + vm.m[1] * world.y + vm.m[2] * world.z + vm.m[3];
    float y = vm.m[4] * world.x + vm.m[5] * world.y + vm.m[6] * world.z + vm.m[7];
    float w = vm.m[12] * world.x + vm.m[13] * world.y + vm.m[14] * world.z + vm.m[15];

    if (w < 0.01f)
        return false;

    float inv = 1.0f / w;
    x *= inv;
    y *= inv;

    out.x = (screenSize.x * 0.5f) + (x * screenSize.x * 0.5f);
    out.y = (screenSize.y * 0.5f) - (y * screenSize.y * 0.5f);
    return true;
}
