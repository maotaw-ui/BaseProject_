#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <algorithm>
#include <cmath>
#include <cstdint>
#include <cstring>

namespace RendererCore {
    struct DirtyRect {
        int left = 0;
        int top = 0;
        int right = 0;
        int bottom = 0;

        [[nodiscard]] bool Empty() const { return left >= right || top >= bottom; }
    };

    inline uint32_t* bits = nullptr;
    inline int width = 0;
    inline int height = 0;
    inline DirtyRect previousDirty{};
    inline DirtyRect currentDirty{};

    [[nodiscard]] inline bool Ready() { return bits && width > 0 && height > 0; }

    inline void MarkDirty(int left, int top, int right, int bottom) {
        left = std::clamp(left, 0, width);
        top = std::clamp(top, 0, height);
        right = std::clamp(right, 0, width);
        bottom = std::clamp(bottom, 0, height);
        if (left >= right || top >= bottom) return;

        if (currentDirty.Empty()) currentDirty = { left, top, right, bottom };
        else {
            currentDirty.left = std::min(currentDirty.left, left);
            currentDirty.top = std::min(currentDirty.top, top);
            currentDirty.right = std::max(currentDirty.right, right);
            currentDirty.bottom = std::max(currentDirty.bottom, bottom);
        }
    }

    inline void Clear() {
        currentDirty = {};
        if (!Ready() || previousDirty.Empty()) return;

        const int dirtyWidth = previousDirty.right - previousDirty.left;
        const int dirtyHeight = previousDirty.bottom - previousDirty.top;
        const long long dirtyArea = static_cast<long long>(dirtyWidth) * dirtyHeight;
        const long long screenArea = static_cast<long long>(width) * height;

        // When ESP elements are spread across the screen, the old bounding
        // dirty rectangle is often almost full-screen. A single contiguous
        // memset is faster than hundreds/thousands of row memsets in that case.
        if (dirtyArea * 100 >= screenArea * 40) {
            std::memset(bits, 0,
                        static_cast<size_t>(width) * height * sizeof(uint32_t));
            return;
        }

        const size_t bytes = static_cast<size_t>(dirtyWidth) * sizeof(uint32_t);
        for (int row = previousDirty.top; row < previousDirty.bottom; ++row) {
            std::memset(bits + static_cast<size_t>(row) * width + previousDirty.left,
                        0, bytes);
        }
    }

    inline void DrawPixel(int x, int y, uint32_t color) {
        if (Ready() && x >= 0 && x < width && y >= 0 && y < height) {
            bits[static_cast<size_t>(y) * width + x] = color;
            MarkDirty(x, y, x + 1, y + 1);
        }
    }

    inline void FillRectFast(int x, int y, int w, int h, uint32_t color) {
        if (!Ready() || w <= 0 || h <= 0) return;

        const long long rawRight = static_cast<long long>(x) + w;
        const long long rawBottom = static_cast<long long>(y) + h;
        const int left = std::clamp(x, 0, width);
        const int top = std::clamp(y, 0, height);
        const int right = static_cast<int>(std::clamp(
            rawRight, 0LL, static_cast<long long>(width)));
        const int bottom = static_cast<int>(std::clamp(
            rawBottom, 0LL, static_cast<long long>(height)));
        if (left >= right || top >= bottom) return;

        MarkDirty(left, top, right, bottom);

        for (int py = top; py < bottom; ++py) {
            uint32_t* row = bits + static_cast<size_t>(py) * width + left;
            std::fill_n(row, right - left, color);
        }
    }

    [[nodiscard]] inline long long Edge(int ax, int ay, int bx, int by,
                                        int px, int py) {
        return static_cast<long long>(px - ax) * (by - ay) -
               static_cast<long long>(py - ay) * (bx - ax);
    }

    inline void FillTriangleFast(int x1, int y1, int x2, int y2,
                                 int x3, int y3, uint32_t color) {
        if (!Ready()) return;
        const long long area = Edge(x1, y1, x2, y2, x3, y3);
        if (area == 0) return;

        const int left = std::clamp(std::min({ x1, x2, x3 }), 0, width - 1);
        const int right = std::clamp(std::max({ x1, x2, x3 }), 0, width - 1);
        const int top = std::clamp(std::min({ y1, y2, y3 }), 0, height - 1);
        const int bottom = std::clamp(std::max({ y1, y2, y3 }), 0, height - 1);
        if (left > right || top > bottom) return;

        MarkDirty(left, top, right + 1, bottom + 1);
        for (int py = top; py <= bottom; ++py) {
            for (int px = left; px <= right; ++px) {
                const long long a = Edge(x1, y1, x2, y2, px, py);
                const long long b = Edge(x2, y2, x3, y3, px, py);
                const long long c = Edge(x3, y3, x1, y1, px, py);
                if ((a >= 0 && b >= 0 && c >= 0) ||
                    (a <= 0 && b <= 0 && c <= 0)) {
                    bits[static_cast<size_t>(py) * width + px] = color;
                }
            }
        }
    }

    inline void FillQuadFast(int x1, int y1, int x2, int y2,
                             int x3, int y3, int x4, int y4,
                             uint32_t color) {
        FillTriangleFast(x1, y1, x2, y2, x3, y3, color);
        FillTriangleFast(x1, y1, x3, y3, x4, y4, color);
    }

    [[nodiscard]] inline DirtyRect PresentDirty() {
        if (previousDirty.Empty()) return currentDirty;
        if (currentDirty.Empty()) return previousDirty;
        return {
            std::min(previousDirty.left, currentDirty.left),
            std::min(previousDirty.top, currentDirty.top),
            std::max(previousDirty.right, currentDirty.right),
            std::max(previousDirty.bottom, currentDirty.bottom)
        };
    }

    inline void CommitFrame() { previousDirty = currentDirty; }

    inline void FillRoundedRect(int x, int y, int w, int h,
                                int radius, uint32_t color) {
        if (w <= 0 || h <= 0) return;
        radius = std::clamp(radius, 0, std::min(w, h) / 2);
        if (!radius) {
            FillRectFast(x, y, w, h, color);
            return;
        }

        FillRectFast(x + radius, y, w - radius * 2, h, color);
        FillRectFast(x, y + radius, w, h - radius * 2, color);

        for (int row = 0; row < radius; ++row) {
            const float dy = static_cast<float>(radius - row) - 0.5f;
            const int inset = static_cast<int>(radius - std::sqrt(
                static_cast<float>(radius * radius) - dy * dy));
            FillRectFast(x + inset, y + row, w - inset * 2, 1, color);
            FillRectFast(x + inset, y + h - row - 1, w - inset * 2, 1, color);
        }
    }

    // Rounded rectangle with independent horizontal/vertical corner radii.
    // This is useful for very thin bars: the long-axis radius can remain
    // visible even when the short axis is only a couple of pixels wide.
    inline void FillRoundedRectElliptic(int x, int y, int w, int h,
                                        int radiusX, int radiusY,
                                        uint32_t color) {
        if (w <= 0 || h <= 0) return;
        radiusX = std::clamp(radiusX, 0, (w + 1) / 2);
        radiusY = std::clamp(radiusY, 0, (h + 1) / 2);
        if (!radiusX || !radiusY) {
            FillRectFast(x, y, w, h, color);
            return;
        }

        const int middleY = y + radiusY;
        const int middleH = h - radiusY * 2;
        if (middleH > 0)
            FillRectFast(x, middleY, w, middleH, color);

        for (int row = 0; row < radiusY; ++row) {
            const float dy = (static_cast<float>(radiusY - row) - 0.5f) /
                             static_cast<float>(radiusY);
            const float span = static_cast<float>(radiusX) *
                std::sqrt(std::max(0.0f, 1.0f - dy * dy));
            const int inset = std::clamp(
                static_cast<int>(std::ceil(static_cast<float>(radiusX) - span)),
                0, radiusX);
            const int lineW = std::max(1, w - inset * 2);
            const int lineX = x + (w - lineW) / 2;
            FillRectFast(lineX, y + row, lineW, 1, color);
            FillRectFast(lineX, y + h - row - 1, lineW, 1, color);
        }
    }

    inline void DrawLineFast(int x1, int y1, int x2, int y2, uint32_t color) {
        if (!Ready()) return;
        MarkDirty(std::min(x1, x2), std::min(y1, y2),
                  std::max(x1, x2) + 1, std::max(y1, y2) + 1);
        const int dx = std::abs(x2 - x1), dy = -std::abs(y2 - y1);
        const int sx = x1 < x2 ? 1 : -1, sy = y1 < y2 ? 1 : -1;
        int error = dx + dy;

        for (;;) {
            if (x1 >= 0 && x1 < width && y1 >= 0 && y1 < height)
                bits[static_cast<size_t>(y1) * width + x1] = color;
            if (x1 == x2 && y1 == y2) break;
            const int doubled = error * 2;
            if (doubled >= dy) { error += dy; x1 += sx; }
            if (doubled <= dx) { error += dx; y1 += sy; }
        }
    }

    inline void DrawLineThick(int x1, int y1, int x2, int y2,
                              uint32_t color, int thickness) {
        thickness = std::clamp(thickness, 1, 5);
        const int first = -(thickness / 2);
        const int last = first + thickness - 1;
        const bool mostlyHorizontal = std::abs(x2 - x1) >= std::abs(y2 - y1);
        for (int offset = first; offset <= last; ++offset) {
            if (mostlyHorizontal)
                DrawLineFast(x1, y1 + offset, x2, y2 + offset, color);
            else
                DrawLineFast(x1 + offset, y1, x2 + offset, y2, color);
        }
    }

    inline void FillCircleFast(int centerX, int centerY, int radius,
                               uint32_t color) {
        radius = std::clamp(radius, 0, 6);
        if (!radius) return;
        for (int y = -radius; y <= radius; ++y) {
            const int halfWidth = static_cast<int>(std::sqrt(
                static_cast<float>(radius * radius - y * y)));
            FillRectFast(centerX - halfWidth, centerY + y,
                         halfWidth * 2 + 1, 1, color);
        }
    }

    inline void DrawCircleFast(int centerX, int centerY, int radius,
                               uint32_t color, int thickness = 1) {
        if (!Ready()) return;
        radius = std::clamp(radius, 1, 100);
        thickness = std::clamp(thickness, 1, 5);
        MarkDirty(centerX - radius - thickness, centerY - radius - thickness,
                  centerX + radius + thickness + 1,
                  centerY + radius + thickness + 1);

        auto pixel = [&](int x, int y) {
            if (x >= 0 && x < width && y >= 0 && y < height)
                bits[static_cast<size_t>(y) * width + x] = color;
        };

        for (int ring = 0; ring < thickness; ++ring) {
            const int currentRadius = (std::max)(1, radius - ring);
            int x = currentRadius;
            int y = 0;
            int error = 1 - currentRadius;
            while (x >= y) {
                pixel(centerX + x, centerY + y);
                pixel(centerX + y, centerY + x);
                pixel(centerX - y, centerY + x);
                pixel(centerX - x, centerY + y);
                pixel(centerX - x, centerY - y);
                pixel(centerX - y, centerY - x);
                pixel(centerX + y, centerY - x);
                pixel(centerX + x, centerY - y);
                ++y;
                if (error < 0) error += 2 * y + 1;
                else { --x; error += 2 * (y - x) + 1; }
            }
        }
    }

    inline void DrawRect(int x, int y, int w, int h, uint32_t color,
                         int thickness = 1) {
        for (int i = 0; i < std::max(1, thickness); ++i) {
            if (w - i * 2 <= 0 || h - i * 2 <= 0) break;
            DrawLineFast(x + i, y + i, x + w - i, y + i, color);
            DrawLineFast(x + i, y + h - i, x + w - i, y + h - i, color);
            DrawLineFast(x + i, y + i, x + i, y + h - i, color);
            DrawLineFast(x + w - i, y + i, x + w - i, y + h - i, color);
        }
    }

    inline void DrawRoundedRect(int x, int y, int w, int h, int radius,
                                uint32_t color, int thickness = 1) {
        thickness = std::max(1, thickness);
        for (int i = 0; i < thickness; ++i) {
            const int innerW = w - i * 2;
            const int innerH = h - i * 2;
            if (innerW <= 0 || innerH <= 0) break;
            const int innerRadius = std::max(0, radius - i);
            FillRoundedRect(x + i, y + i, innerW, innerH, innerRadius, color);
            if (innerW > 2 && innerH > 2)
                FillRoundedRect(x + i + 1, y + i + 1, innerW - 2, innerH - 2,
                                std::max(0, innerRadius - 1), 0x00000000);
        }
    }

    inline void DrawCornerBox(int x, int y, int w, int h, uint32_t color,
                              int thickness = 1, int radius = 0) {
        const int cornerW = std::max(1, w / 4);
        const int cornerH = std::max(1, h / 4);
        radius = std::clamp(radius, 0, std::min(cornerW, cornerH));
        for (int i = 0; i < std::max(1, thickness); ++i) {
            const int r = std::max(0, radius - i);
            DrawLineFast(x + r, y + i, x + cornerW, y + i, color);
            DrawLineFast(x + i, y + r, x + i, y + cornerH, color);
            DrawLineFast(x + w - r, y + i, x + w - cornerW, y + i, color);
            DrawLineFast(x + w - i, y + r, x + w - i, y + cornerH, color);
            DrawLineFast(x + r, y + h - i, x + cornerW, y + h - i, color);
            DrawLineFast(x + i, y + h - r, x + i, y + h - cornerH, color);
            DrawLineFast(x + w - r, y + h - i, x + w - cornerW, y + h - i, color);
            DrawLineFast(x + w - i, y + h - r, x + w - i, y + h - cornerH, color);

            if (r > 0) {
                for (int dy = 0; dy <= r; ++dy) {
                    const int dx = static_cast<int>(std::sqrt(
                        static_cast<float>(r * r - dy * dy)) + 0.5f);
                    DrawPixel(x + r - dx, y + r - dy, color);
                    DrawPixel(x + w - r + dx, y + r - dy, color);
                    DrawPixel(x + r - dx, y + h - r + dy, color);
                    DrawPixel(x + w - r + dx, y + h - r + dy, color);
                }
            }
        }
    }

    [[nodiscard]] inline uint32_t WithAlpha(uint32_t color, uint8_t alpha) {
        const uint32_t r = ((color >> 16) & 0xFF) * alpha / 255;
        const uint32_t g = ((color >> 8) & 0xFF) * alpha / 255;
        const uint32_t b = (color & 0xFF) * alpha / 255;
        return (static_cast<uint32_t>(alpha) << 24) | (r << 16) | (g << 8) | b;
    }

    inline void DrawRoundedBox(int x, int y, int w, int h,
                               uint32_t outline, uint32_t fill,
                               int thickness, int radius) {
        thickness = std::clamp(thickness, 1, 5);
        radius = std::clamp(radius, 0, std::min(w, h) / 2);
        FillRoundedRect(x, y, w, h, radius, outline);

        const int innerW = w - thickness * 2;
        const int innerH = h - thickness * 2;
        if (innerW > 0 && innerH > 0) {
            FillRoundedRect(x + thickness, y + thickness, innerW, innerH,
                            std::max(0, radius - thickness), fill);
        }
    }

    inline void DrawHealthBar(int x, int y, int h, int health,
                              uint32_t color, int barWidth, int radius) {
        if (h <= 0 || barWidth <= 0) return;
        health = std::clamp(health, 0, 100);
        radius = std::clamp(radius, 0, std::min(barWidth, h) / 2);
        const int filled = h * health / 100;

        FillRoundedRect(x, y, barWidth, h, radius, 0xE6141414);
        if (filled > 0) {
            const int pad = barWidth >= 4 ? 1 : 0;
            FillRoundedRect(x + pad, y + h - filled,
                            std::max(1, barWidth - pad * 2), filled,
                            std::max(0, radius - pad), color);
        }
    }

    inline void DrawHealthBarHorizontal(int x, int y, int w, int health,
                                        uint32_t color, int barHeight, int radius) {
        if (w <= 0 || barHeight <= 0) return;
        health = std::clamp(health, 0, 100);
        radius = std::clamp(radius, 0, std::min(w, barHeight) / 2);
        FillRoundedRect(x, y, w, barHeight, radius, 0xE6141414);
        const int pad = barHeight >= 4 ? 1 : 0;
        const int filled = std::max(1, w - pad * 2) * health / 100;
        if (filled > 0) {
            FillRoundedRect(x + pad, y + pad, filled,
                            std::max(1, barHeight - pad * 2),
                            std::max(0, radius - pad), color);
        }
    }

    [[nodiscard]] inline uint32_t HealthGradient(float value, uint32_t topColor) {
        value = std::clamp(value, 0.0f, 1.0f);
        constexpr int startRed = 255, startGreen = 35, startBlue = 35;
        const int endRed = (topColor >> 16) & 0xFF;
        const int endGreen = (topColor >> 8) & 0xFF;
        const int endBlue = topColor & 0xFF;
        const int red = static_cast<int>(startRed + (endRed - startRed) * value);
        const int green = static_cast<int>(startGreen + (endGreen - startGreen) * value);
        const int blue = static_cast<int>(startBlue + (endBlue - startBlue) * value);
        return 0xFF000000u | (static_cast<uint32_t>(red) << 16) |
               (static_cast<uint32_t>(green) << 8) | static_cast<uint32_t>(blue);
    }

    inline void DrawSegmentedHealthBar(int x, int y, int h, int health,
                                       int barWidth, int radius, uint32_t topColor,
                                       int segments = 20) {
        if (h <= 0 || barWidth <= 0 || segments <= 0) return;
        health = std::clamp(health, 0, 100);
        radius = std::clamp(radius, 0, std::min(barWidth, h) / 2);

        FillRoundedRect(x, y, barWidth, h, radius, 0xE6141414);
        const int padding = barWidth >= 5 ? 2 : 1;
        const int innerWidth = std::max(1, barWidth - padding * 2);
        const int gap = 1;
        const int innerTop = y + 2;
        const int innerBottom = y + h - 2;
        const int innerHeight = std::max(1, innerBottom - innerTop);
        const int actualSegments = std::min(
            std::min(segments, 10), std::max(1, (innerHeight + gap) / 2));
        const int filledSegments = (health * actualSegments + 99) / 100;

        for (int i = 0; i < actualSegments; ++i) {
            const int lower = innerBottom - i * innerHeight / actualSegments;
            const int upper = innerBottom -
                              (i + 1) * innerHeight / actualSegments;
            const int segmentHeight = std::max(1, lower - upper - gap);
            const uint32_t segmentColor = i < filledSegments
                ? HealthGradient(static_cast<float>(i + 1) / actualSegments,
                                 topColor)
                : 0xFF252525;
            FillRoundedRect(x + padding, upper, innerWidth, segmentHeight,
                            std::min(1, innerWidth / 2), segmentColor);
        }
    }

    inline void DrawSegmentedHealthBarHorizontal(
        int x, int y, int w, int health, int barHeight, int radius,
        uint32_t topColor, int segments = 20) {
        if (w <= 0 || barHeight <= 0 || segments <= 0) return;
        health = std::clamp(health, 0, 100);
        radius = std::clamp(radius, 0, std::min(w, barHeight) / 2);

        FillRoundedRect(x, y, w, barHeight, radius, 0xE6141414);
        const int padding = barHeight >= 5 ? 2 : 1;
        const int innerHeight = std::max(1, barHeight - padding * 2);
        const int gap = 1;
        const int innerLeft = x + 2;
        const int innerRight = x + w - 2;
        const int innerWidth = std::max(1, innerRight - innerLeft);
        const int actualSegments = std::min(
            std::min(segments, 10), std::max(1, (innerWidth + gap) / 2));
        const int filledSegments = (health * actualSegments + 99) / 100;

        for (int i = 0; i < actualSegments; ++i) {
            const int segmentLeft = innerLeft +
                                    i * innerWidth / actualSegments;
            const int segmentRight = innerLeft +
                                     (i + 1) * innerWidth / actualSegments;
            const int segmentWidth = std::max(
                1, segmentRight - segmentLeft - gap);
            const uint32_t segmentColor = i < filledSegments
                ? HealthGradient(static_cast<float>(i + 1) / actualSegments, topColor)
                : 0xFF252525;
            FillRoundedRect(segmentLeft, y + padding, segmentWidth, innerHeight,
                            std::min(1, innerHeight / 2), segmentColor);
        }
    }
}
