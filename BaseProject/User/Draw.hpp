#pragma once

#include "../Core/Overlay.hpp"

#include <algorithm>
#include <cmath>
#include <string>

namespace Draw {

    struct Target {
        int health{};
        int armor{};
        float feetX{};
        float feetY{};
        float headX{};
        float headY{};
        float left{};
        float top{};
        float width{};
        float height{};
    };

    inline void Box(Renderer& renderer, const Target& target,
        D2D1_COLOR_F color, D2D1_COLOR_F outlineColor,
        float thickness = 1.0f) {
        const float right = target.left + target.width;
        const float bottom = target.top + target.height;

        renderer.Rectangle(target.left - 1.0f, target.top - 1.0f,
            right + 1.0f, bottom + 1.0f,
            outlineColor, thickness + 2.0f);

        renderer.Rectangle(target.left, target.top, right, bottom,
            color, thickness);
    }

    inline void CornerBox(Renderer& renderer, const Target& target,
        D2D1_COLOR_F color, D2D1_COLOR_F outlineColor,
        float thickness = 1.0f) {
        const float left = target.left;
        const float top = target.top;
        const float right = left + target.width;
        const float bottom = top + target.height;
        const float cornerWidth = target.width * 0.28f;
        const float cornerHeight = target.height * 0.22f;

        const auto corners = [&](D2D1_COLOR_F drawColor,
            float drawThickness,
            float offset) {
                const float x1 = left - offset;
                const float y1 = top - offset;
                const float x2 = right + offset;
                const float y2 = bottom + offset;
                const float width = cornerWidth + offset;
                const float height = cornerHeight + offset;

                renderer.Line(x1, y1, x1 + width, y1, drawColor, drawThickness);
                renderer.Line(x1, y1, x1, y1 + height, drawColor, drawThickness);
                renderer.Line(x2 - width, y1, x2, y1, drawColor, drawThickness);
                renderer.Line(x2, y1, x2, y1 + height, drawColor, drawThickness);
                renderer.Line(x1, y2 - height, x1, y2, drawColor, drawThickness);
                renderer.Line(x1, y2, x1 + width, y2, drawColor, drawThickness);
                renderer.Line(x2, y2 - height, x2, y2, drawColor, drawThickness);
                renderer.Line(x2 - width, y2, x2, y2, drawColor, drawThickness);
            };

        corners(outlineColor, thickness + 2.0f, 1.0f);
        corners(color, thickness, 0.0f);
    }

    inline void HealthBar(Renderer& renderer, const Target& target,
        D2D1_COLOR_F color,
        D2D1_COLOR_F backgroundColor,
        D2D1_COLOR_F textColor,
        float width = 4.0f) {
        const int health = std::clamp(target.health, 0, 100);
        const float percent = static_cast<float>(health) / 100.0f;
        const float left = target.left - width - 4.0f;
        const float right = left + width;
        const float top = target.top;
        const float bottom = top + target.height;
        const float filledTop = bottom - target.height * percent;

        renderer.FilledRectangle(left - 1.0f, top - 1.0f,
            right + 1.0f, bottom + 1.0f,
            backgroundColor);

        if (health > 0) {
            renderer.FilledRectangle(left, filledTop, right, bottom, color);
        }

        const std::wstring text = std::to_wstring(health);
        constexpr float fontSize = 7.0f;
        const float textWidth = static_cast<float>(text.length()) * 3.6f;
        const float textX = left + width * 0.5f - textWidth * 0.5f;
        const float textY = std::clamp(filledTop - fontSize,
            top - fontSize,
            bottom - fontSize);

        renderer.Text(textX, textY, text, textColor, fontSize);
    }

    inline void GradientHealthBar(Renderer& renderer, const Target& target,
        D2D1_COLOR_F highColor,
        D2D1_COLOR_F lowColor,
        D2D1_COLOR_F backgroundColor,
        D2D1_COLOR_F textColor,
        float width = 4.0f) {
        const int health = std::clamp(target.health, 0, 100);
        const float percent = static_cast<float>(health) / 100.0f;
        const float left = target.left - width - 4.0f;
        const float right = left + width;
        const float top = target.top;
        const float bottom = top + target.height;
        const float filledTop = bottom - target.height * percent;

        renderer.FilledRectangle(left - 1.0f, top - 1.0f,
            right + 1.0f, bottom + 1.0f,
            backgroundColor);

        const int rows = std::max(0, static_cast<int>(std::ceil(bottom - filledTop)));

        for (int row = 0; row < rows; ++row) {
            const float y = bottom - static_cast<float>(row) - 1.0f;
            if (y < filledTop) break;

            const float amount = (bottom - y) / std::max(1.0f, target.height);
            const D2D1_COLOR_F color = D2D1::ColorF(
                lowColor.r + (highColor.r - lowColor.r) * amount,
                lowColor.g + (highColor.g - lowColor.g) * amount,
                lowColor.b + (highColor.b - lowColor.b) * amount,
                1.0f);

            renderer.FilledRectangle(left, y, right,
                std::min(y + 1.0f, bottom), color);
        }

        const std::wstring text = std::to_wstring(health);
        constexpr float fontSize = 7.0f;
        const float textWidth = static_cast<float>(text.length()) * 3.6f;
        const float textX = left + width * 0.5f - textWidth * 0.5f;
        const float textY = std::clamp(filledTop - fontSize,
            top - fontSize,
            bottom - fontSize);

        renderer.Text(textX, textY, text, textColor, fontSize);
    }

    inline void Snapline(Renderer& renderer, const Target& target,
        D2D1_COLOR_F color, float thickness = 1.0f) {
        renderer.Line(renderer.Width() * 0.5f, renderer.Height(),
            target.feetX, target.feetY, color, thickness);

    }

} // namespace Draw
