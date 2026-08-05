#pragma once

#include "../Core/Memory.hpp"

#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <vector>

struct Vec3 {
    float x{};
    float y{};
    float z{};
};

struct Matrix4x4 {
    float m[4][4]{};
};

namespace Target {
inline constexpr auto Process = L"cs2.exe";
inline constexpr auto Module = L"client.dll";
}

// Update these values when the target updates.
namespace Offsets {
inline constexpr std::ptrdiff_t EntityList = 0x254FE80;
inline constexpr std::ptrdiff_t LocalPlayerPawn = 0x23A5238;
inline constexpr std::ptrdiff_t ViewMatrix = 0x23AA340;
inline constexpr std::ptrdiff_t Team = 0x3E7;
inline constexpr std::ptrdiff_t Health = 0x34C;
inline constexpr std::ptrdiff_t Armor = 0x1CA4;
inline constexpr std::ptrdiff_t Origin = 0x13B8;
inline constexpr std::ptrdiff_t PawnHandle = 0x914;
}

namespace Math {
[[nodiscard]] inline bool WorldToScreen(
    const Vec3& world,
    const Matrix4x4& matrix,
    float screenWidth,
    float screenHeight,
    float& screenX,
    float& screenY) noexcept {

    const float clipW =
        matrix.m[3][0] * world.x +
        matrix.m[3][1] * world.y +
        matrix.m[3][2] * world.z +
        matrix.m[3][3];

    if (!std::isfinite(clipW) || clipW <= 0.01f) {
        return false;
    }

    const float clipX =
        matrix.m[0][0] * world.x +
        matrix.m[0][1] * world.y +
        matrix.m[0][2] * world.z +
        matrix.m[0][3];

    const float clipY =
        matrix.m[1][0] * world.x +
        matrix.m[1][1] * world.y +
        matrix.m[1][2] * world.z +
        matrix.m[1][3];

    screenX = (1.0f + clipX / clipW) * screenWidth * 0.5f;
    screenY = (1.0f - clipY / clipW) * screenHeight * 0.5f;

    return std::isfinite(screenX) && std::isfinite(screenY);
}
}

struct GameEntity {
    int index{};
    std::uintptr_t controller{};
    std::uintptr_t pawn{};
    int team{};
    int health{};
    int armor{};
    Vec3 origin{};
};

struct LocalPlayer {
    std::uintptr_t pawn{};
    int team{};
    Matrix4x4 viewMatrix{};
    bool valid{};
};

class EntityCache final {
public:
    void Initialize(const Memory& memory, std::uintptr_t client);
    void Update();

    [[nodiscard]] const LocalPlayer& Local() const noexcept;
    [[nodiscard]] const std::vector<GameEntity>& Entities() const noexcept;

private:
    void RebuildCache();
    void RefreshLiveData();
    [[nodiscard]] std::uintptr_t ResolvePawn(std::uint32_t handle) const;

    static constexpr int MaxPlayers = 64;
    static constexpr auto RebuildInterval = std::chrono::seconds{ 2 };

    const Memory* memory_{};
    std::uintptr_t client_{};
    std::uintptr_t entityList_{};
    LocalPlayer local_{};
    std::vector<GameEntity> entities_;
    std::chrono::steady_clock::time_point nextRebuild_{};
};
