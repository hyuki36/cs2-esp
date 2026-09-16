#pragma once
#include <cmath>
#include <string>

struct Vector2 {
    float x = 0, y = 0;
};

struct Vector3 {
    float x = 0, y = 0, z = 0;

    Vector3() = default;
    Vector3(float _x, float _y, float _z) : x(_x), y(_y), z(_z) {}

    Vector3 operator-(const Vector3& o) const { return {x - o.x, y - o.y, z - o.z}; }
    Vector3 operator+(const Vector3& o) const { return {x + o.x, y + o.y, z + o.z}; }

    float Length() const { return std::sqrt(x * x + y * y + z * z); }
    float Dist(const Vector3& o) const { return (*this - o).Length(); }
};

// Row-major 4x4 as stored in Roblox Camera ViewMatrix
struct Matrix4 {
    float m[16] = {0};
};
