#pragma once
#include "memory.hpp"
#include "vec.hpp"
#include <vector>
#include <string>

struct CS2Entity {
    std::string name;
    uintptr_t controller = 0;
    uintptr_t pawn = 0;
    Vector3 feet{};
    Vector3 head{};
    int health = 0;
    int team = 0;
    int armor = 0;
    bool spotted = false;
    bool valid = false;
};

class CS2 {
public:
    explicit CS2(Memory* m) : mem(m) {}

    bool Update();
    std::vector<CS2Entity> GetEntities(int maxPlayers = 64);
    Matrix4 GetViewMatrix() const { return viewMatrix; }
    int GetLocalTeam() const { return localTeam; }
    uintptr_t GetLocalPawn() const { return localPawn; }

    static bool WorldToScreen(const Vector3& world, const Matrix4& vm, Vector2 screenSize, Vector2& out);
    Vector3 GetBonePos(uintptr_t pawn, int boneIdx);

    // aim / movement helpers
    Vector3 GetEyePos() const;
    Vector3 GetVelocity(uintptr_t pawn) const;
    bool SetVelocity(uintptr_t pawn, const Vector3& v) const;
    bool IsOnGround(uintptr_t pawn) const;
    Vector3 GetPunch() const;
    int GetShotsFired() const;
    bool ReadAngles(Vector3& out) const;
    bool WriteAngles(const Vector3& ang) const;

    uintptr_t clientBase = 0;
    uintptr_t entityList = 0;
    uintptr_t localController = 0;
    uintptr_t localPawn = 0;
    int localTeam = 0;
    Matrix4 viewMatrix{};

private:
    Memory* mem = nullptr;

    std::string ReadControllerName(uintptr_t controller);
};
