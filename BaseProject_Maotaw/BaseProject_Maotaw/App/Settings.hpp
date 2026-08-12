#pragma once

#include <algorithm>

namespace AppSettings {
    enum class BoxStyle : int {
        Box, Corner, Filled, Rainbow, RainbowFilled
    };
    enum class HealthStyle : int { Normal, Segmented, Gradient };
    enum class HealthPosition : int { Left, Right, Top, Bottom };
    enum class SnaplineStyle : int { Bottom, Top, Head };
    enum class MenuTheme : int { Purple, Light, Emerald, Ocean };

    inline constexpr int ColorCount = 20;

    struct Values {
        // Same visual options as our current cheat.
        bool esp = true;              // master visuals
        bool box = true;
        bool skeleton = true;
        bool health = true;
        bool healthText = true;
        bool armor = true;
        bool armorText = true;
        bool snapline = true;
        bool radar = false;
        bool trail = false;

        // Separate top-level ESP draw distance. Radar keeps its own range.
        int espDistance = 500;         // 20..500 meters, 20m steps

        BoxStyle boxStyle = BoxStyle::Box;
        int boxWidthPercent = 50;      // 30..80 -> 0.30..0.80
        int boxThickness = 1;          // 1..5
        int fillOpacity = 20;          // 0..60 percent
        int boxColor = 4;
        int fillColor = 4;
        int boxOutlineThickness = 1; // 0..5
        int boxOutlineColor = 10;

        int skeletonThickness = 1;     // 1..5
        int skeletonColor = 12;

        HealthStyle healthStyle = HealthStyle::Normal;
        HealthPosition healthPosition = HealthPosition::Left;
        int healthWidth = 3;           // 1..12
        int healthRadius = 3;          // 0..6 actual corner radius in pixels
        int healthSpacing = 4;         // 0..10
        int healthSegments = 10;       // 2..20
        int healthColor = 1;
        int healthBackgroundColor = 10;
        int healthOutlineThickness = 1; // 0..5
        int healthOutlineColor = 10;

        HealthStyle armorStyle = HealthStyle::Normal;
        HealthPosition armorPosition = HealthPosition::Right;
        int armorWidth = 3;            // 1..12
        int armorRadius = 3;           // 0..6
        int armorSpacing = 4;          // 0..10
        int armorSegments = 10;        // 2..20
        int armorColor = 13;           // blue
        int armorBackgroundColor = 10;
        int armorOutlineThickness = 1; // 0..5
        int armorOutlineColor = 10;

        SnaplineStyle snaplineStyle = SnaplineStyle::Bottom;
        int snaplineThickness = 1;     // 1..5
        int snaplineColor = 6;

        int radarSize = 140;           // 80..260 pixels
        int radarRange = 60;           // 20..500 meters, adjusted in 20m steps
        int radarColor = 5;
        int radarBackgroundColor = 10;

        int trailWidth = 2;            // 1..5
        int trailLength = 20;          // 4..32 samples
        int trailColor = 3;

        MenuTheme menuTheme = MenuTheme::Purple;

        int menuX = 70;
        int menuY = 280;

        bool operator==(const Values&) const = default;
    };

    inline Values values{};

    inline void Sanitize() {
        values.espDistance = std::clamp(values.espDistance, 20, 500);
        values.espDistance = ((values.espDistance + 10) / 20) * 20;
        values.espDistance = std::clamp(values.espDistance, 20, 500);

        values.boxStyle = static_cast<BoxStyle>(std::clamp(
            static_cast<int>(values.boxStyle), 0, 4));
        values.boxWidthPercent = std::clamp(values.boxWidthPercent, 30, 80);
        values.boxThickness = std::clamp(values.boxThickness, 1, 5);
        values.fillOpacity = std::clamp(values.fillOpacity, 0, 60);
        values.boxColor = std::clamp(values.boxColor, 0, ColorCount - 1);
        values.fillColor = std::clamp(values.fillColor, 0, ColorCount - 1);
        values.boxOutlineThickness = std::clamp(values.boxOutlineThickness, 0, 5);
        values.boxOutlineColor = std::clamp(values.boxOutlineColor, 0, ColorCount - 1);

        values.skeletonThickness = std::clamp(values.skeletonThickness, 1, 5);
        values.skeletonColor = std::clamp(values.skeletonColor, 0, ColorCount - 1);

        values.healthStyle = static_cast<HealthStyle>(std::clamp(
            static_cast<int>(values.healthStyle), 0, 2));
        values.healthPosition = static_cast<HealthPosition>(std::clamp(
            static_cast<int>(values.healthPosition), 0, 3));
        values.healthWidth = std::clamp(values.healthWidth, 1, 12);
        values.healthRadius = std::clamp(values.healthRadius, 0, 6);
        values.healthSpacing = std::clamp(values.healthSpacing, 0, 10);
        values.healthSegments = std::clamp(values.healthSegments, 2, 20);
        values.healthColor = std::clamp(values.healthColor, 0, ColorCount - 1);
        values.healthBackgroundColor = std::clamp(
            values.healthBackgroundColor, 0, ColorCount - 1);
        values.healthOutlineThickness = std::clamp(values.healthOutlineThickness, 0, 5);
        values.healthOutlineColor = std::clamp(values.healthOutlineColor, 0, ColorCount - 1);

        values.armorStyle = static_cast<HealthStyle>(std::clamp(
            static_cast<int>(values.armorStyle), 0, 2));
        values.armorPosition = static_cast<HealthPosition>(std::clamp(
            static_cast<int>(values.armorPosition), 0, 3));
        values.armorWidth = std::clamp(values.armorWidth, 1, 12);
        values.armorRadius = std::clamp(values.armorRadius, 0, 6);
        values.armorSpacing = std::clamp(values.armorSpacing, 0, 10);
        values.armorSegments = std::clamp(values.armorSegments, 2, 20);
        values.armorColor = std::clamp(values.armorColor, 0, ColorCount - 1);
        values.armorBackgroundColor = std::clamp(values.armorBackgroundColor, 0, ColorCount - 1);
        values.armorOutlineThickness = std::clamp(values.armorOutlineThickness, 0, 5);
        values.armorOutlineColor = std::clamp(values.armorOutlineColor, 0, ColorCount - 1);

        values.snaplineStyle = static_cast<SnaplineStyle>(std::clamp(
            static_cast<int>(values.snaplineStyle), 0, 2));
        values.snaplineThickness = std::clamp(values.snaplineThickness, 1, 5);
        values.snaplineColor = std::clamp(values.snaplineColor, 0, ColorCount - 1);

        values.radarSize = std::clamp(values.radarSize, 80, 260);
        values.radarRange = std::clamp(values.radarRange, 20, 500);
        values.radarRange = ((values.radarRange + 10) / 20) * 20;
        values.radarRange = std::clamp(values.radarRange, 20, 500);
        values.radarColor = std::clamp(values.radarColor, 0, ColorCount - 1);
        values.radarBackgroundColor = std::clamp(
            values.radarBackgroundColor, 0, ColorCount - 1);

        values.trailWidth = std::clamp(values.trailWidth, 1, 5);
        values.trailLength = std::clamp(values.trailLength, 4, 32);
        values.trailColor = std::clamp(values.trailColor, 0, ColorCount - 1);

        values.menuTheme = static_cast<MenuTheme>(std::clamp(
            static_cast<int>(values.menuTheme), 0, 3));

        values.menuX = std::max(0, values.menuX);
        values.menuY = std::max(0, values.menuY);
    }
}
