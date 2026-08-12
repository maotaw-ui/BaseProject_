#pragma once

#include <algorithm>
#include <cmath>
#include <vector>

#include "App/Settings.hpp"
#include "Core/MathCore.hpp"
#include "Core/RendererCore.hpp"
#include "Game/EntityCache.hpp"
#include "UI/Palette.hpp"

namespace Radar {
    inline void Render(
        const EntityCache::FrameData& frame,
        const std::vector<EntityCache::CachedEntity>& entities) {
        const auto& settings = AppSettings::values;
        if (!settings.radar || !frame.valid || !frame.viewAnglesValid) return;

        const int size = std::clamp(settings.radarSize, 80, 260);
        const int padding = 22;
        const int radarX = std::max(8, RendererCore::width - size - padding);
        const int radarY = 42;
        const int centerX = radarX + size / 2;
        const int centerY = radarY + size / 2;
        const int innerRadius = std::max(8, size / 2 - 7);

        const uint32_t background = RendererCore::WithAlpha(
            Palette::RadarBackground(), 190);
        const uint32_t border = RendererCore::WithAlpha(Palette::Radar(), 205);
        const uint32_t grid = RendererCore::WithAlpha(Palette::Radar(), 75);

        RendererCore::DrawRoundedBox(
            radarX, radarY, size, size, border, background, 1, 0);
        RendererCore::DrawLineFast(centerX, radarY + 4,
                                   centerX, radarY + size - 5, grid);
        RendererCore::DrawLineFast(radarX + 4, centerY,
                                   radarX + size - 5, centerY, grid);
        RendererCore::FillCircleFast(centerX, centerY, 2, 0xFFFFFFFF);

        const float rangeUnits = std::max(1.0f,
            static_cast<float>(settings.radarRange) * Math::UnitsPerMeter);

        // World-static radar. Source yaw increases when turning left, so use
        // -sin(yaw) for the horizontal screen component: left turn -> left,
        // right turn -> right. World-space enemy projection below uses the
        // exact same axis convention.
        const float yawDegrees = frame.viewAngles.y;
        const float yaw = yawDegrees * 3.14159265358979323846f / 180.0f;
        const float cosYaw = std::cos(yaw);
        const float sinYaw = std::sin(yaw);

        // Source yaw 0 faces +X. Positive yaw turns toward +Y (left).
        // Radar screen: +X world = up, +Y world = left.
        const float arrowDirX = -sinYaw;
        const float arrowDirY = -cosYaw;
        const float arrowPerpX = -arrowDirY;
        const float arrowPerpY = arrowDirX;

        // Filled two-layer pointer: accent outline + white inner arrow.
        const float outerTipX = static_cast<float>(centerX) + arrowDirX * 15.0f;
        const float outerTipY = static_cast<float>(centerY) + arrowDirY * 15.0f;
        const float outerBaseX = static_cast<float>(centerX) - arrowDirX * 4.0f;
        const float outerBaseY = static_cast<float>(centerY) - arrowDirY * 4.0f;
        const float outerLeftX = outerBaseX + arrowPerpX * 6.5f;
        const float outerLeftY = outerBaseY + arrowPerpY * 6.5f;
        const float outerRightX = outerBaseX - arrowPerpX * 6.5f;
        const float outerRightY = outerBaseY - arrowPerpY * 6.5f;

        RendererCore::FillTriangleFast(
            static_cast<int>(outerTipX), static_cast<int>(outerTipY),
            static_cast<int>(outerLeftX), static_cast<int>(outerLeftY),
            static_cast<int>(outerRightX), static_cast<int>(outerRightY), border);

        const float innerTipX = static_cast<float>(centerX) + arrowDirX * 12.5f;
        const float innerTipY = static_cast<float>(centerY) + arrowDirY * 12.5f;
        const float innerBaseX = static_cast<float>(centerX) - arrowDirX * 2.5f;
        const float innerBaseY = static_cast<float>(centerY) - arrowDirY * 2.5f;
        const float innerLeftX = innerBaseX + arrowPerpX * 4.2f;
        const float innerLeftY = innerBaseY + arrowPerpY * 4.2f;
        const float innerRightX = innerBaseX - arrowPerpX * 4.2f;
        const float innerRightY = innerBaseY - arrowPerpY * 4.2f;

        RendererCore::FillTriangleFast(
            static_cast<int>(innerTipX), static_cast<int>(innerTipY),
            static_cast<int>(innerLeftX), static_cast<int>(innerLeftY),
            static_cast<int>(innerRightX), static_cast<int>(innerRightY),
            0xFFFFFFFF);
        RendererCore::FillCircleFast(centerX, centerY, 2, border);

        for (const auto& entity : entities) {
            if (!entity.pawn || !entity.originValid ||
                entity.team == frame.localTeam ||
                entity.health <= 0 || entity.health > 100) continue;

            const float dx = entity.origin.x - frame.localPosition.x;
            const float dy = entity.origin.y - frame.localPosition.y;
            const float distanceSquared = dx * dx + dy * dy;
            if (!std::isfinite(distanceSquared) ||
                distanceSquared > rangeUnits * rangeUnits) continue;

            const float scale = static_cast<float>(innerRadius) / rangeUnits;

            // Same world-to-radar convention as the player arrow:
            // +X world is up and +Y world is left. This guarantees that an
            // enemy in front appears in the direction the arrow points.
            const int px = centerX - static_cast<int>(dy * scale);
            const int py = centerY - static_cast<int>(dx * scale);
            RendererCore::FillCircleFast(px, py, 3, Palette::Radar());
        }
    }
}
