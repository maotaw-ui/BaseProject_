#pragma once

#include <cmath>

struct vec3 {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;
};

struct Matrix4x4 {
    float m[4][4]{};
};

namespace Math {
    inline constexpr float UnitsPerMeter = 39.37f;

    [[nodiscard]] inline float DistanceSquared(const vec3& a, const vec3& b) {
        const float dx = a.x - b.x;
        const float dy = a.y - b.y;
        const float dz = a.z - b.z;
        return dx * dx + dy * dy + dz * dz;
    }

    [[nodiscard]] inline bool WithinMeters(const vec3& a, const vec3& b,
                                           float meters) {
        const float units = meters * UnitsPerMeter;
        return DistanceSquared(a, b) <= units * units;
    }

    inline bool WorldToScreen(const vec3& world, const Matrix4x4& matrix,
                              int screenWidth, int screenHeight,
                              int& screenX, int& screenY) {
        if (screenWidth <= 0 || screenHeight <= 0) return false;

        const float clipW = matrix.m[3][0] * world.x +
                            matrix.m[3][1] * world.y +
                            matrix.m[3][2] * world.z + matrix.m[3][3];
        if (!std::isfinite(clipW) || clipW <= 0.01f) return false;

        const float clipX = matrix.m[0][0] * world.x +
                            matrix.m[0][1] * world.y +
                            matrix.m[0][2] * world.z + matrix.m[0][3];
        const float clipY = matrix.m[1][0] * world.x +
                            matrix.m[1][1] * world.y +
                            matrix.m[1][2] * world.z + matrix.m[1][3];

        const float x = clipX / clipW;
        const float y = clipY / clipW;
        if (!std::isfinite(x) || !std::isfinite(y)) return false;

        screenX = static_cast<int>((1.0f + x) * screenWidth * 0.5f);
        screenY = static_cast<int>((1.0f - y) * screenHeight * 0.5f);
        return true;
    }
}
