#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>

#include "App/Settings.hpp"
#include "Core/MathCore.hpp"
#include "Core/RendererCore.hpp"
#include "Game/EntityCache.hpp"
#include "Features/Radar.hpp"
#include "Features/Trail.hpp"
#include "UI/Palette.hpp"
#include "UI/PixelFont.hpp"

namespace UserRender {
    inline constexpr float PlayerHeight = 72.0f;
    inline constexpr float BoxWidthRatio = 0.50f;

    inline void DrawPlayerSkeleton(const EntityCache::CachedEntity& entity,
                                   const Matrix4x4& matrix, int,
                                   int playerLeft, int playerTop,
                                   int playerWidth, int playerHeight) {
        if (!entity.debugBonesValid) return;

        const auto& settings = AppSettings::values;
        struct ScreenBone { int x = 0; int y = 0; bool visible = false; };
        std::array<ScreenBone, EntityCache::DebugBoneCount> screen{};

        // Only project bones that are actually used by the skeleton links.
        // The old renderer projected the entire cached bone block for every
        // player every frame even though most points were never drawn.
        const vec3 boneFollowDelta{
            entity.origin.x - entity.boneOrigin.x,
            entity.origin.y - entity.boneOrigin.y,
            entity.origin.z - entity.boneOrigin.z
        };

        for (const std::size_t index : EntityCache::SkeletonRenderIndices) {
            if (index >= entity.debugBones.size()) continue;
            const vec3 renderBone{
                entity.debugBones[index].x + boneFollowDelta.x,
                entity.debugBones[index].y + boneFollowDelta.y,
                entity.debugBones[index].z + boneFollowDelta.z
            };
            auto& point = screen[index];
            point.visible = Math::WorldToScreen(
                renderBone, matrix, RendererCore::width,
                RendererCore::height, point.x, point.y);
        }

        using Connection = std::array<std::size_t, 2>;
        static constexpr std::array<Connection, 20> TerroristConnections{{
            {{1, 2}}, {{2, 3}}, {{3, 4}}, {{4, 12}},
            {{12, 5}}, {{5, 6}}, {{6, 7}},
            {{12, 13}}, {{13, 14}}, {{14, 71}},
            {{12, 9}}, {{9, 50}}, {{50, 10}}, {{10, 11}},
            {{1, 17}}, {{17, 18}}, {{18, 19}},
            {{1, 20}}, {{20, 21}}, {{21, 22}}
        }};
        static constexpr std::array<Connection, 20> CounterTerroristConnections{{
            {{1, 2}}, {{2, 3}}, {{3, 4}}, {{4, 12}},
            {{12, 5}}, {{5, 6}}, {{6, 7}},
            {{12, 13}}, {{13, 14}}, {{14, 73}},
            {{12, 9}}, {{9, 50}}, {{50, 10}}, {{10, 11}},
            {{1, 17}}, {{17, 18}}, {{18, 19}},
            {{1, 20}}, {{20, 21}}, {{21, 22}}
        }};

        const int playerCenterX = playerLeft + playerWidth / 2;
        const int horizontalAllowance = std::max(
            playerWidth, static_cast<int>(playerHeight * 0.68f));
        const int minSkeletonX = playerCenterX - horizontalAllowance;
        const int maxSkeletonX = playerCenterX + horizontalAllowance;
        const int minSkeletonY = playerTop - static_cast<int>(playerHeight * 0.18f);
        const int maxSkeletonY = playerTop + playerHeight +
                                 static_cast<int>(playerHeight * 0.12f);
        const float maxSegmentPixels = std::max(18.0f, playerHeight * 0.62f);
        const float maxSegmentPixelsSq = maxSegmentPixels * maxSegmentPixels;

        auto screenPointSane = [&](const ScreenBone& point) {
            return point.visible &&
                   point.x >= minSkeletonX && point.x <= maxSkeletonX &&
                   point.y >= minSkeletonY && point.y <= maxSkeletonY;
        };

        auto drawConnections = [&](const auto& connections) {
            for (const auto& connection : connections) {
                const auto& from = screen[connection[0]];
                const auto& to = screen[connection[1]];
                if (!screenPointSane(from) || !screenPointSane(to)) continue;
                if (Math::DistanceSquared(
                        entity.debugBones[connection[0]],
                        entity.debugBones[connection[1]]) > 55.0f * 55.0f)
                    continue;
                const float screenDx = static_cast<float>(to.x - from.x);
                const float screenDy = static_cast<float>(to.y - from.y);
                if (screenDx * screenDx + screenDy * screenDy > maxSegmentPixelsSq)
                    continue;
                RendererCore::DrawLineThick(
                    from.x, from.y, to.x, to.y,
                    Palette::Skeleton(), settings.skeletonThickness);
            }
        };

        if (entity.team == 2) drawConnections(TerroristConnections);
        else drawConnections(CounterTerroristConnections);
    }

    inline void DrawRainbowBox(int left, int top, int width, int height,
                               int thickness, int phase) {
        constexpr int ColorBand = 5;
        constexpr int PhaseSteps = 4096;
        const int perimeter = 2 * (width + height);
        const int movement = phase * perimeter / PhaseSteps;

        for (int x = 0; x < width; x += ColorBand) {
            const int length = (std::min)(ColorBand, width - x);
            RendererCore::FillRectFast(left + x, top, length, thickness,
                                       Palette::RainbowAt(x + movement, perimeter));
            RendererCore::FillRectFast(
                left + x, top + height - thickness, length, thickness,
                Palette::RainbowAt(
                    width + height + width - x + movement, perimeter));
        }

        for (int y = 0; y < height; y += ColorBand) {
            const int length = (std::min)(ColorBand, height - y);
            RendererCore::FillRectFast(
                left + width - thickness, top + y, thickness, length,
                Palette::RainbowAt(width + y + movement, perimeter));
            RendererCore::FillRectFast(
                left, top + y, thickness, length,
                Palette::RainbowAt(
                    2 * width + height + height - y + movement, perimeter));
        }
    }

    inline void DrawPlayerBox(int left, int top, int boxWidth, int boxHeight,
                              int rainbowMovement) {
        using AppSettings::BoxStyle;
        const auto& settings = AppSettings::values;
        const auto style = settings.boxStyle;
        const uint32_t color = Palette::Box();
        const uint32_t outline = Palette::BoxOutline();
        const uint32_t transparent = 0x00000000;
        const uint8_t fillAlpha = static_cast<uint8_t>(
            std::clamp(settings.fillOpacity, 0, 100) * 255 / 100);
        const uint32_t fill = RendererCore::WithAlpha(Palette::Fill(), fillAlpha);
        const int thickness = settings.boxThickness;
        constexpr int radius = 0;
        const int outlineWidth = std::clamp(settings.boxOutlineThickness, 0, 5);

        if (style == BoxStyle::Corner) {
            if (outlineWidth > 0) {
                RendererCore::DrawCornerBox(
                    left - outlineWidth, top - outlineWidth,
                    boxWidth + outlineWidth * 2, boxHeight + outlineWidth * 2,
                    outline, outlineWidth, 0);
            }
            RendererCore::DrawCornerBox(left, top, boxWidth, boxHeight,
                                        color, thickness, 0);
            return;
        }

        if (outlineWidth > 0) {
            RendererCore::DrawRoundedBox(
                left - outlineWidth, top - outlineWidth,
                boxWidth + outlineWidth * 2, boxHeight + outlineWidth * 2,
                outline, transparent, outlineWidth, 0);
        }

        if (style == BoxStyle::Rainbow || style == BoxStyle::RainbowFilled) {
            if (style == BoxStyle::RainbowFilled && fillAlpha > 0) {
                RendererCore::FillRoundedRect(
                    left + thickness, top + thickness,
                    std::max(1, boxWidth - thickness * 2),
                    std::max(1, boxHeight - thickness * 2), radius, fill);
            }
            DrawRainbowBox(left, top, boxWidth, boxHeight,
                           thickness, rainbowMovement);
            return;
        }

        if (style == BoxStyle::Filled && fillAlpha > 0) {
            RendererCore::DrawRoundedBox(left, top, boxWidth, boxHeight,
                                         color, fill, thickness, radius);
        } else {
            RendererCore::DrawRoundedBox(left, top, boxWidth, boxHeight,
                                         color, transparent, thickness, radius);
        }
    }

    [[nodiscard]] inline int TinyTextWidth(const char* text);
    inline void DrawTinyText(int x, int y, const char* text, uint32_t color);

    inline void DrawPlayerValueBar(
        int left, int top, int boxWidth, int boxHeight, int value,
        AppSettings::HealthStyle style,
        AppSettings::HealthPosition position,
        int configuredWidth, int requestedRadius, int configuredSpacing,
        int configuredSegments, uint32_t foreground, uint32_t backgroundColor,
        int configuredOutlineThickness, uint32_t outlineColor,
        bool showText) {
        const bool horizontal = position == AppSettings::HealthPosition::Top ||
                                position == AppSettings::HealthPosition::Bottom;
        configuredWidth = std::clamp(configuredWidth, 1, 12);
        configuredSpacing = std::clamp(configuredSpacing, 0, 10);
        requestedRadius = std::clamp(requestedRadius, 0, 6);
        configuredOutlineThickness = std::clamp(configuredOutlineThickness, 0, 5);
        value = std::clamp(value, 0, 100);

        // Scale every cross-axis measurement with the projected player height.
        // This prevents a distant 20px player from getting a visually huge bar.
        const float projectedScale = std::clamp(
            static_cast<float>(boxHeight) / 180.0f, 0.20f, 1.0f);
        int thickness = std::max(1, static_cast<int>(std::lround(
            static_cast<float>(configuredWidth) * projectedScale)));
        if (requestedRadius > 0 && configuredWidth > 1)
            thickness = std::max(2, thickness);
        const int gap = std::max(0, static_cast<int>(std::lround(
            static_cast<float>(configuredSpacing) * projectedScale)));
        const int outline = configuredOutlineThickness > 0
            ? std::max(1, static_cast<int>(std::lround(
                static_cast<float>(configuredOutlineThickness) * projectedScale)))
            : 0;

        // Radius remains visible on very thin bars by using an elliptical cap.
        const int crossRadius = requestedRadius > 0
            ? std::max(1, (thickness + 1) / 2) : 0;

        // Keep the OUTSIDE of the outline exactly `gap` pixels from the box.
        const int barX = position == AppSettings::HealthPosition::Left
                       ? left - gap - outline - thickness
                       : position == AppSettings::HealthPosition::Right
                       ? left + boxWidth + gap + outline : left;
        const int barY = position == AppSettings::HealthPosition::Top
                       ? top - gap - outline - thickness
                       : position == AppSettings::HealthPosition::Bottom
                       ? top + boxHeight + gap + outline : top;

        const uint32_t background = RendererCore::WithAlpha(backgroundColor, 230);

        auto fillVerticalRounded = [&](int x, int y, int w, int h,
                                       int radius, uint32_t color) {
            const int ry = std::min(radius, std::max(0, h / 2));
            RendererCore::FillRoundedRectElliptic(
                x, y, w, h, crossRadius, ry, color);
        };
        auto fillHorizontalRounded = [&](int x, int y, int w, int h,
                                         int radius, uint32_t color) {
            const int rx = std::min(radius, std::max(0, w / 2));
            RendererCore::FillRoundedRectElliptic(
                x, y, w, h, rx, crossRadius, color);
        };
        auto ellipseInset = [](int positionIndex, int length,
                               int alongRadius, int acrossRadius) {
            if (alongRadius <= 0 || acrossRadius <= 0 || length <= 0) return 0;
            alongRadius = std::min(alongRadius, std::max(0, length / 2));
            if (alongRadius <= 0) return 0;
            const int edge = std::min(positionIndex, length - 1 - positionIndex);
            if (edge >= alongRadius) return 0;
            const float d = (static_cast<float>(alongRadius - edge) - 0.5f) /
                            static_cast<float>(alongRadius);
            const float span = static_cast<float>(acrossRadius) *
                std::sqrt(std::max(0.0f, 1.0f - d * d));
            return std::clamp(
                static_cast<int>(std::ceil(static_cast<float>(acrossRadius) - span)),
                0, acrossRadius);
        };

        // Draw outline outside the configured bar width so outline thickness
        // never makes a thin health/armor bar disappear.
        if (outline > 0) {
            if (horizontal) {
                const int outerRx = std::min(requestedRadius + outline,
                                             (boxWidth + outline * 2) / 2);
                const int outerRy = crossRadius + outline;
                RendererCore::FillRoundedRectElliptic(
                    barX - outline, barY - outline,
                    boxWidth + outline * 2, thickness + outline * 2,
                    outerRx, outerRy, outlineColor);
            } else {
                const int outerRx = crossRadius + outline;
                const int outerRy = std::min(requestedRadius + outline,
                                             (boxHeight + outline * 2) / 2);
                RendererCore::FillRoundedRectElliptic(
                    barX - outline, barY - outline,
                    thickness + outline * 2, boxHeight + outline * 2,
                    outerRx, outerRy, outlineColor);
            }
        }

        auto drawNormalVertical = [&]() {
            fillVerticalRounded(barX, barY, thickness, boxHeight,
                                requestedRadius, background);
            const int filled = boxHeight * value / 100;
            if (filled > 0) {
                fillVerticalRounded(barX, barY + boxHeight - filled,
                                    thickness, filled, requestedRadius, foreground);
            }
        };
        auto drawNormalHorizontal = [&]() {
            fillHorizontalRounded(barX, barY, boxWidth, thickness,
                                  requestedRadius, background);
            const int filled = boxWidth * value / 100;
            if (filled > 0) {
                fillHorizontalRounded(barX, barY, filled, thickness,
                                      requestedRadius, foreground);
            }
        };
        auto drawGradientVertical = [&]() {
            fillVerticalRounded(barX, barY, thickness, boxHeight,
                                requestedRadius, background);
            const int filled = boxHeight * value / 100;
            const int fillRadius = std::min(requestedRadius, filled / 2);
            for (int i = 0; i < filled; ++i) {
                const float level = filled > 1 ? static_cast<float>(i) / (filled - 1) : 1.0f;
                const uint32_t c = RendererCore::HealthGradient(level, foreground);
                const int inset = ellipseInset(i, filled, fillRadius, crossRadius);
                const int lineWidth = std::max(1, thickness - inset * 2);
                const int lineX = barX + (thickness - lineWidth) / 2;
                RendererCore::FillRectFast(lineX,
                    barY + boxHeight - filled + i, lineWidth, 1, c);
            }
        };
        auto drawGradientHorizontal = [&]() {
            fillHorizontalRounded(barX, barY, boxWidth, thickness,
                                  requestedRadius, background);
            const int filled = boxWidth * value / 100;
            const int fillRadius = std::min(requestedRadius, filled / 2);
            for (int i = 0; i < filled; ++i) {
                const float level = filled > 1 ? static_cast<float>(i) / (filled - 1) : 1.0f;
                const uint32_t c = RendererCore::HealthGradient(level, foreground);
                const int inset = ellipseInset(i, filled, fillRadius, crossRadius);
                const int lineHeight = std::max(1, thickness - inset * 2);
                const int lineY = barY + (thickness - lineHeight) / 2;
                RendererCore::FillRectFast(barX + i, lineY, 1, lineHeight, c);
            }
        };
        auto drawSegmentedVertical = [&]() {
            fillVerticalRounded(barX, barY, thickness, boxHeight,
                                requestedRadius, background);
            const int segments = std::clamp(configuredSegments, 2, 20);
            const int active = (value * segments + 99) / 100;
            constexpr int segmentGap = 1;
            for (int i = 0; i < active; ++i) {
                const int lower = barY + boxHeight - i * boxHeight / segments;
                const int upper = barY + boxHeight - (i + 1) * boxHeight / segments;
                const int h = std::max(1, lower - upper - segmentGap);
                const uint32_t c = RendererCore::HealthGradient(
                    static_cast<float>(i + 1) / segments, foreground);
                fillVerticalRounded(barX, upper, thickness, h,
                                    requestedRadius, c);
            }
        };
        auto drawSegmentedHorizontal = [&]() {
            fillHorizontalRounded(barX, barY, boxWidth, thickness,
                                  requestedRadius, background);
            const int segments = std::clamp(configuredSegments, 2, 20);
            const int active = (value * segments + 99) / 100;
            constexpr int segmentGap = 1;
            for (int i = 0; i < active; ++i) {
                const int l = barX + i * boxWidth / segments;
                const int r = barX + (i + 1) * boxWidth / segments;
                const int w = std::max(1, r - l - segmentGap);
                const uint32_t c = RendererCore::HealthGradient(
                    static_cast<float>(i + 1) / segments, foreground);
                fillHorizontalRounded(l, barY, w, thickness,
                                      requestedRadius, c);
            }
        };

        if (style == AppSettings::HealthStyle::Segmented) {
            if (horizontal) drawSegmentedHorizontal();
            else drawSegmentedVertical();
        } else if (style == AppSettings::HealthStyle::Gradient) {
            if (horizontal) drawGradientHorizontal();
            else drawGradientVertical();
        } else {
            if (horizontal) drawNormalHorizontal();
            else drawNormalVertical();
        }

        // Same tiny value treatment for health and armor; only shown below 100.
        if (showText && value < 100) {
            char text[4]{};
            PixelFont::Number(value, text);
            const int textWidth = TinyTextWidth(text);
            int tx = 0;
            int ty = 0;
            if (horizontal) {
                tx = barX + (boxWidth - textWidth) / 2;
                ty = barY + (thickness - 5) / 2;
            } else {
                const int filled = boxHeight * value / 100;
                const int fillTop = barY + boxHeight - filled;
                tx = barX + (thickness - textWidth) / 2;
                ty = std::clamp(fillTop - 2, barY, barY + boxHeight - 5);
            }
            DrawTinyText(tx + 1, ty + 1, text, 0xE0000000);
            DrawTinyText(tx, ty, text, 0xFFFFFFFF);
        }
    }

    inline void DrawPlayerHealth(int left, int top, int boxWidth,
                                 int boxHeight, int health) {
        const auto& settings = AppSettings::values;
        DrawPlayerValueBar(
            left, top, boxWidth, boxHeight, health,
            settings.healthStyle, settings.healthPosition,
            settings.healthWidth, settings.healthRadius, settings.healthSpacing,
            settings.healthSegments, Palette::Health(), Palette::HealthBackground(),
            settings.healthOutlineThickness, Palette::HealthOutline(),
            settings.healthText);
    }

    inline void DrawPlayerArmor(int left, int top, int boxWidth,
                                int boxHeight, int armor) {
        const auto& settings = AppSettings::values;
        DrawPlayerValueBar(
            left, top, boxWidth, boxHeight, armor,
            settings.armorStyle, settings.armorPosition,
            settings.armorWidth, settings.armorRadius, settings.armorSpacing,
            settings.armorSegments, Palette::Armor(), Palette::ArmorBackground(),
            settings.armorOutlineThickness, Palette::ArmorOutline(),
            settings.armorText);
    }

    inline void DrawSnapline(int feetX, int feetY, int headX, int headY) {
        const auto& settings = AppSettings::values;
        int startX = RendererCore::width / 2;
        int startY = RendererCore::height;
        int endX = feetX;
        int endY = feetY;
        if (settings.snaplineStyle == AppSettings::SnaplineStyle::Top) {
            startY = 0;
        } else if (settings.snaplineStyle == AppSettings::SnaplineStyle::Head) {
            startY = 0;
            endX = headX;
            endY = headY;
        }
        RendererCore::DrawLineThick(startX, startY, endX, endY,
                                    Palette::Snapline(), settings.snaplineThickness);
    }

    using TinyGlyph = std::array<uint8_t, 5>;

    [[nodiscard]] inline TinyGlyph ArmorGlyph(char character) {
        switch (character) {
        case '0': return { 7, 5, 5, 5, 7 };
        case '1': return { 2, 6, 2, 2, 7 };
        case '2': return { 7, 1, 7, 4, 7 };
        case '3': return { 7, 1, 7, 1, 7 };
        case '4': return { 5, 5, 7, 1, 1 };
        case '5': return { 7, 4, 7, 1, 7 };
        case '6': return { 7, 4, 7, 5, 7 };
        case '7': return { 7, 1, 2, 2, 2 };
        case '8': return { 7, 5, 7, 5, 7 };
        case '9': return { 7, 5, 7, 1, 7 };
        case '%': return { 5, 1, 2, 4, 5 };
        default: return {};
        }
    }

    [[nodiscard]] inline int TinyTextWidth(const char* text) {
        int length = 0;
        while (text[length]) ++length;
        return length ? length * 4 - 1 : 0;
    }

    inline void DrawTinyText(int x, int y, const char* text, uint32_t color) {
        if (!RendererCore::Ready()) return;
        RendererCore::MarkDirty(x, y, x + TinyTextWidth(text), y + 5);
        for (int index = 0; text[index]; ++index) {
            const auto glyph = ArmorGlyph(text[index]);
            for (int row = 0; row < 5; ++row) {
                for (int column = 0; column < 3; ++column) {
                    if (!(glyph[row] & (1u << (2 - column)))) continue;
                    const int pixelX = x + index * 4 + column;
                    const int pixelY = y + row;
                    if (pixelX >= 0 && pixelX < RendererCore::width &&
                        pixelY >= 0 && pixelY < RendererCore::height) {
                        RendererCore::bits[static_cast<size_t>(pixelY) *
                                           RendererCore::width + pixelX] = color;
                    }
                }
            }
        }
    }

    inline void FillArmorShield(int x, int y, int width, int height,
                                int shoulderY, int firstRow,
                                uint32_t color) {
        const int center = width / 2;
        const int lastRow = height - 1;
        firstRow = std::clamp(firstRow, 1, lastRow);

        for (int row = firstRow; row < lastRow; ++row) {
            int left = 1;
            int right = width - 2;

            if (row > shoulderY) {
                const int lowerHeight = (std::max)(1, lastRow - shoulderY);
                const int halfWidth = (width - 5) * (lastRow - row) /
                                      (2 * lowerHeight);
                left = center - halfWidth;
                right = center + halfWidth;
            } else if (row == shoulderY) {
                left = 2;
                right = width - 3;
            }

            RendererCore::FillRectFast(
                x + left, y + row, (std::max)(1, right - left + 1), 1, color);
        }
    }

    inline void DrawArmorShield(int x, int y, int armor, int boxHeight) {
        const bool micro = boxHeight < 35;
        const bool compact = boxHeight < 90;
        const int shieldWidth = micro ? 9 : compact ? 15 : 19;
        const int shieldHeight = micro ? 8 : compact ? 12 : 15;
        const int shoulderY = micro ? 4 : compact ? 6 : 8;
        constexpr uint32_t ShieldBlue = 0xE51A73E8;
        constexpr uint32_t ShieldLight = 0xFF66B5FF;
        constexpr uint32_t ShieldDark = 0xFF073B7A;
        constexpr uint32_t ShieldEmpty = 0xE5081930;

        armor = std::clamp(armor, 0, 100);
        FillArmorShield(x, y, shieldWidth, shieldHeight,
                        shoulderY, 1, ShieldEmpty);

        const int interiorRows = shieldHeight - 2;
        const int filledRows = (interiorRows * armor + 99) / 100;
        if (filledRows > 0) {
            const int fillStart = shieldHeight - 1 - filledRows;
            FillArmorShield(x, y, shieldWidth, shieldHeight,
                            shoulderY, fillStart, ShieldBlue);
        }

        RendererCore::DrawLineFast(
            x + 1, y + 1, x + shieldWidth - 2, y + 1, ShieldLight);
        RendererCore::DrawLineFast(
            x + 1, y + 1, x + 2, y + shoulderY, ShieldDark);
        RendererCore::DrawLineFast(
            x + shieldWidth - 2, y + 1,
            x + shieldWidth - 3, y + shoulderY, ShieldDark);
        RendererCore::DrawLineFast(
            x + 2, y + shoulderY,
            x + shieldWidth / 2, y + shieldHeight - 1, ShieldDark);
        RendererCore::DrawLineFast(
            x + shieldWidth - 3, y + shoulderY,
            x + shieldWidth / 2, y + shieldHeight - 1, ShieldDark);

        if (micro) return;

        char text[5]{};
        if (armor == 100) {
            text[0] = '1'; text[1] = '0'; text[2] = '0';
            if (!compact) text[3] = '%';
        } else if (armor >= 10) {
            text[0] = static_cast<char>('0' + armor / 10);
            text[1] = static_cast<char>('0' + armor % 10);
            if (!compact) text[2] = '%';
        } else {
            text[0] = static_cast<char>('0' + armor);
            if (!compact) text[1] = '%';
        }

        DrawTinyText(x + (shieldWidth - TinyTextWidth(text)) / 2,
                     y + (compact ? 3 : 4), text, 0xFFFFFFFF);
    }

    inline void OnRender() {
        const auto& frame = EntityCache::GetFrame();
        const auto& settings = AppSettings::values;
        if (!frame.valid) return;

        // Radar is a separate top-level feature and must not depend on ESP.
        Radar::Render(frame, EntityCache::GetEntities());

        if (!settings.esp) return;

        int rainbowPhase = 0;
        if (settings.box &&
            (settings.boxStyle == AppSettings::BoxStyle::Rainbow ||
             settings.boxStyle == AppSettings::BoxStyle::RainbowFilled)) {
            const auto milliseconds = std::chrono::duration_cast<
                std::chrono::milliseconds>(
                    std::chrono::steady_clock::now().time_since_epoch()).count();
            constexpr long long CycleMilliseconds = 4000;
            constexpr int PhaseSteps = 4096;
            rainbowPhase = static_cast<int>(
                (milliseconds % CycleMilliseconds) * PhaseSteps /
                CycleMilliseconds);
        }

        Trail::UpdateAndRender(frame, EntityCache::GetEntities());

        for (const auto& entity : EntityCache::GetEntities()) {
            if (!entity.pawn || !entity.originValid ||
                entity.team == frame.localTeam ||
                entity.health <= 0 || entity.health > 100) continue;
            if (!Math::WithinMeters(entity.origin, frame.localPosition,
                                    settings.espDistance)) continue;

            vec3 head = entity.origin;
            head.z += PlayerHeight;

            int feetX = 0, feetY = 0, headX = 0, headY = 0;
            const bool feetVisible = Math::WorldToScreen(
                entity.origin, frame.viewMatrix,
                RendererCore::width, RendererCore::height, feetX, feetY);
            const bool headVisible = Math::WorldToScreen(
                head, frame.viewMatrix,
                RendererCore::width, RendererCore::height, headX, headY);
            if (!feetVisible || !headVisible) continue;

            const int boxHeight = feetY - headY;
            if (boxHeight <= 4 || boxHeight > RendererCore::height * 2) continue;

            const float widthRatio = std::clamp(settings.boxWidthPercent, 30, 80) / 100.0f;
            const int boxWidth = std::max(2, static_cast<int>(boxHeight * widthRatio));
            const int left = headX - boxWidth / 2;
            if (left >= RendererCore::width || left + boxWidth <= 0 ||
                headY >= RendererCore::height || feetY <= 0) continue;

            if (settings.box)
                DrawPlayerBox(left, headY, boxWidth, boxHeight, rainbowPhase);
            if (settings.health)
                DrawPlayerHealth(left, headY, boxWidth, boxHeight, entity.health);
            if (settings.armor)
                DrawPlayerArmor(left, headY, boxWidth, boxHeight, entity.armor);
            if (settings.snapline)
                DrawSnapline(feetX, feetY, headX, headY);
            if (settings.skeleton)
                DrawPlayerSkeleton(entity, frame.viewMatrix, rainbowPhase,
                                   left, headY, boxWidth, boxHeight);
        }
    }

}
