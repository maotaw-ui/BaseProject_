#pragma once

#include <algorithm>
#include <array>
#include <cstdint>

#include "App/Settings.hpp"

namespace Palette {
    inline constexpr std::array<uint32_t, AppSettings::ColorCount> Colors{
        0xFFFFFFFF, 0xFF24FF5A, 0xFFFF3838, 0xFF29C7FF, 0xFFA55CFF,
        0xFFFF3D91, 0xFFFFD43B, 0xFF00E5C0, 0xFFFF814A, 0xFFB7FF35,
        0xFF101010, 0xFF808080, 0xFFBFC5CC, 0xFF235CFF, 0xFF4A35FF,
        0xFF00A86B, 0xFFFFA500, 0xFFFF1744, 0xFF8B5A2B, 0xFF00FFFF
    };

    inline constexpr std::array<uint32_t, 12> RainbowColors{
        0xFFFF3B30, 0xFFFF9500, 0xFFFFD60A, 0xFFA8E10C,
        0xFF34C759, 0xFF00C7BE, 0xFF32ADE6, 0xFF007AFF,
        0xFF5856D6, 0xFFAF52DE, 0xFFFF2D92, 0xFFFF375F
    };

    [[nodiscard]] inline uint32_t Get(int index) {
        return Colors[std::clamp(index, 0, static_cast<int>(Colors.size()) - 1)];
    }

    [[nodiscard]] inline uint32_t Box() { return Get(AppSettings::values.boxColor); }
    [[nodiscard]] inline uint32_t Fill() { return Get(AppSettings::values.fillColor); }
    [[nodiscard]] inline uint32_t BoxOutline() { return Get(AppSettings::values.boxOutlineColor); }
    [[nodiscard]] inline uint32_t Health() { return Get(AppSettings::values.healthColor); }
    [[nodiscard]] inline uint32_t HealthBackground() {
        return Get(AppSettings::values.healthBackgroundColor);
    }
    [[nodiscard]] inline uint32_t HealthOutline() {
        return Get(AppSettings::values.healthOutlineColor);
    }
    [[nodiscard]] inline uint32_t Armor() { return Get(AppSettings::values.armorColor); }
    [[nodiscard]] inline uint32_t ArmorBackground() {
        return Get(AppSettings::values.armorBackgroundColor);
    }
    [[nodiscard]] inline uint32_t ArmorOutline() {
        return Get(AppSettings::values.armorOutlineColor);
    }
    [[nodiscard]] inline uint32_t Skeleton() { return Get(AppSettings::values.skeletonColor); }
    [[nodiscard]] inline uint32_t Snapline() { return Get(AppSettings::values.snaplineColor); }
    [[nodiscard]] inline uint32_t Radar() { return Get(AppSettings::values.radarColor); }
    [[nodiscard]] inline uint32_t RadarBackground() {
        return Get(AppSettings::values.radarBackgroundColor);
    }
    [[nodiscard]] inline uint32_t Trail() { return Get(AppSettings::values.trailColor); }
    [[nodiscard]] inline uint32_t RainbowAt(int position, int perimeter) {
        if (perimeter <= 0) return RainbowColors[0];
        position %= perimeter;
        if (position < 0) position += perimeter;
        const int index = position * static_cast<int>(RainbowColors.size()) / perimeter;
        return RainbowColors[index];
    }
}
