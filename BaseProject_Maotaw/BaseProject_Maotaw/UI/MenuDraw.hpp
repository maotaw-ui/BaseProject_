#pragma once

#include <cstdint>
#include <cstring>

#include "Core/RendererCore.hpp"
#include "UI/MenuState.hpp"
#include "UI/Palette.hpp"
#include "UI/PixelFont.hpp"

namespace PixelMenu::Draw {
    inline void Icon(int iconX, int iconY, const char* label, uint32_t color) {
        if (!std::strcmp(label, "PLAYER") || !std::strcmp(label, "BOX")) {
            RendererCore::FillRoundedRect(iconX + 3, iconY, 3, 3, 1, color);
            RendererCore::FillRoundedRect(iconX + 1, iconY + 4, 7, 4, 1, color);
        } else if (!std::strcmp(label, "HEALTHBAR")) {
            RendererCore::FillRectFast(iconX + 3, iconY, 2, 8, color);
            RendererCore::FillRectFast(iconX, iconY + 3, 8, 2, color);
        } else if (!std::strcmp(label, "ARMOR")) {
            RendererCore::FillQuadFast(
                iconX + 1, iconY, iconX + 8, iconY,
                iconX + 7, iconY + 5, iconX + 2, iconY + 5, color);
            RendererCore::FillTriangleFast(
                iconX + 2, iconY + 4, iconX + 7, iconY + 4,
                iconX + 4, iconY + 9, color);
        } else if (!std::strcmp(label, "RADAR")) {
            RendererCore::DrawRoundedBox(iconX, iconY, 9, 9, color, 0x00000000, 1, 1);
            RendererCore::DrawLineFast(iconX + 4, iconY + 1, iconX + 4, iconY + 7, color);
            RendererCore::DrawLineFast(iconX + 1, iconY + 4, iconX + 7, iconY + 4, color);
            RendererCore::FillCircleFast(iconX + 6, iconY + 2, 1, color);
        } else if (!std::strcmp(label, "TRAIL")) {
            RendererCore::DrawLineFast(iconX, iconY + 2, iconX + 8, iconY, color);
            RendererCore::DrawLineFast(iconX + 1, iconY + 5, iconX + 7, iconY + 3, color);
            RendererCore::DrawLineFast(iconX + 3, iconY + 8, iconX + 8, iconY + 6, color);
        } else if (!std::strcmp(label, "MAX DISTANCE")) {
            RendererCore::DrawLineFast(iconX + 4, iconY, iconX + 8, iconY + 4, color);
            RendererCore::DrawLineFast(iconX + 8, iconY + 4, iconX + 4, iconY + 8, color);
            RendererCore::DrawLineFast(iconX + 4, iconY + 8, iconX, iconY + 4, color);
            RendererCore::DrawLineFast(iconX, iconY + 4, iconX + 4, iconY, color);
            RendererCore::DrawLineFast(iconX + 4, iconY + 2, iconX + 4, iconY + 6, color);
        } else if (!std::strcmp(label, "BACK")) {
            RendererCore::DrawLineFast(iconX, iconY + 4, iconX + 5, iconY, color);
            RendererCore::DrawLineFast(iconX, iconY + 4, iconX + 5, iconY + 8, color);
            RendererCore::DrawLineFast(iconX, iconY + 4, iconX + 9, iconY + 4, color);
        } else {
            RendererCore::FillRoundedRect(iconX + 2, iconY + 2, 6, 6, 2, color);
            RendererCore::FillRoundedRect(iconX + 4, iconY + 4, 2, 2, 1, CurrentTheme().panel);
        }
    }

    inline void Panel(int panelX, const char* title, int rows) {
        (void)title;
        const int panelHeight = HeaderHeight + rows * RowHeight + FooterHeight;
        RendererCore::FillRoundedRect(
            panelX - 4, y - 4, Width + 8, panelHeight + 8,
            Radius + 2, RendererCore::WithAlpha(CurrentTheme().accent, 35));
        RendererCore::FillRoundedRect(
            panelX - 2, y - 2, Width + 4, panelHeight + 4,
            Radius + 1, RendererCore::WithAlpha(CurrentTheme().accent, 75));
        RendererCore::DrawRoundedBox(
            panelX, y, Width, panelHeight,
            CurrentTheme().border, CurrentTheme().panel, 1, Radius);

        for (int i = 0; i < 8; ++i) {
            const int streakX = panelX + 18 + (i * 31) % (Width - 40);
            const int streakY = y + 10 + (i * 37) % (panelHeight - 20);
            RendererCore::DrawLineFast(
                streakX, streakY, streakX + 5 + i % 4, streakY - 2,
                RendererCore::WithAlpha(CurrentTheme().accent, 24));
        }
    }

    inline void Row(int panelX, int row, const char* label, const char* value,
                    bool active, uint32_t valueColor = 0) {
        if (valueColor == 0) valueColor = CurrentTheme().value;
        const int rowY = y + HeaderHeight + row * RowHeight;
        if (active) {
            RendererCore::FillRoundedRect(
                panelX + 7, rowY + 2, Width - 14, RowHeight - 4,
                2, CurrentTheme().selection);
        }

        const uint32_t itemColor = active ? CurrentTheme().itemActive : CurrentTheme().item;
        Icon(panelX + 13, rowY + 7, label, itemColor);
        PixelFont::Draw(panelX + 33, rowY + 8, label, itemColor);
        PixelFont::Draw(panelX + Width - PixelFont::Width(value) - 29,
                        rowY + 8, value, active ? CurrentTheme().itemActive : valueColor);
        PixelFont::Draw(panelX + Width - 15, rowY + 8, ">", itemColor);
    }

    [[nodiscard]] inline const char* Toggle(bool enabled) {
        return enabled ? "ON" : "OFF";
    }

    [[nodiscard]] inline uint32_t ToggleColor(bool enabled) {
        return enabled ? CurrentTheme().on : CurrentTheme().off;
    }
}

namespace PixelMenu {
    inline void DrawMainPanel() {
        Draw::Panel(x, Title, 4);
        Draw::Row(x, 0, "VISUALS", Draw::Toggle(settings.esp),
                  mainRow == 0, Draw::ToggleColor(settings.esp));
        Draw::Row(x, 1, "RADAR", Draw::Toggle(settings.radar),
                  mainRow == 1, Draw::ToggleColor(settings.radar));

        char distanceNumber[4]{}, distanceText[5]{};
        PixelFont::Number(settings.espDistance, distanceNumber);
        strcpy_s(distanceText, distanceNumber);
        strcat_s(distanceText, "M");
        Draw::Row(x, 2, "DISTANCE", distanceText, mainRow == 2);

        static constexpr const char* ThemeNames[]{ "VIOLET", "PEARL", "MINT", "ICE" };
        Draw::Row(x, 3, "THEME", ThemeNames[static_cast<int>(settings.menuTheme)],
                  mainRow == 3, CurrentTheme().value);
    }

    inline void DrawVisualPanel(int panelX) {
        static constexpr const char* BoxStyles[]{
            "BOX", "CORNER", "FILLED", "RAINBOW", "R+F"
        };
        static constexpr const char* HealthStyles[]{
            "NORMAL", "SEGMENT", "GRADIENT"
        };
        static constexpr const char* HealthPositions[]{
            "LEFT", "RIGHT", "TOP", "BOTTOM"
        };
        static constexpr const char* SnaplineStyles[]{
            "BOTTOM", "TOP", "HEAD"
        };

        const int rows = expanded == Expanded::Box ? 10
                       : expanded == Expanded::Health ? 13
                       : expanded == Expanded::Armor ? 13
                       : expanded == Expanded::Skeleton ? 4
                       : expanded == Expanded::Snapline ? 5
                       : expanded == Expanded::Trail ? 5 : 7;
        Draw::Panel(panelX, "ESP SETTINGS", rows);

        char boxWidth[4]{}, boxThickness[4]{}, fillOpacity[4]{};
        char boxColor[4]{}, fillColor[4]{}, boxOutlineThickness[4]{}, boxOutlineColor[4]{};
        char healthWidth[4]{}, healthRadius[4]{}, healthSpacing[4]{}, healthSegments[4]{};
        char healthColor[4]{}, healthBackground[4]{}, healthOutlineThickness[4]{}, healthOutlineColor[4]{};
        char armorWidth[4]{}, armorRadius[4]{}, armorSpacing[4]{}, armorSegments[4]{};
        char armorColor[4]{}, armorBackground[4]{}, armorOutlineThickness[4]{}, armorOutlineColor[4]{};
        char skeletonThickness[4]{}, skeletonColor[4]{};
        char snaplineThickness[4]{}, snaplineColor[4]{};
        char trailWidth[4]{}, trailLength[4]{}, trailColor[4]{};

        PixelFont::Number(settings.boxWidthPercent, boxWidth);
        PixelFont::Number(settings.boxThickness, boxThickness);
        PixelFont::Number(settings.fillOpacity, fillOpacity);
        PixelFont::Number(settings.boxColor + 1, boxColor);
        PixelFont::Number(settings.fillColor + 1, fillColor);
        PixelFont::Number(settings.boxOutlineThickness, boxOutlineThickness);
        PixelFont::Number(settings.boxOutlineColor + 1, boxOutlineColor);
        PixelFont::Number(settings.healthWidth, healthWidth);
        PixelFont::Number(settings.healthRadius, healthRadius);
        PixelFont::Number(settings.healthSpacing, healthSpacing);
        PixelFont::Number(settings.healthSegments, healthSegments);
        PixelFont::Number(settings.healthColor + 1, healthColor);
        PixelFont::Number(settings.healthBackgroundColor + 1, healthBackground);
        PixelFont::Number(settings.healthOutlineThickness, healthOutlineThickness);
        PixelFont::Number(settings.healthOutlineColor + 1, healthOutlineColor);
        PixelFont::Number(settings.armorWidth, armorWidth);
        PixelFont::Number(settings.armorRadius, armorRadius);
        PixelFont::Number(settings.armorSpacing, armorSpacing);
        PixelFont::Number(settings.armorSegments, armorSegments);
        PixelFont::Number(settings.armorColor + 1, armorColor);
        PixelFont::Number(settings.armorBackgroundColor + 1, armorBackground);
        PixelFont::Number(settings.armorOutlineThickness, armorOutlineThickness);
        PixelFont::Number(settings.armorOutlineColor + 1, armorOutlineColor);
        PixelFont::Number(settings.skeletonThickness, skeletonThickness);
        PixelFont::Number(settings.skeletonColor + 1, skeletonColor);
        PixelFont::Number(settings.snaplineThickness, snaplineThickness);
        PixelFont::Number(settings.snaplineColor + 1, snaplineColor);
        PixelFont::Number(settings.trailWidth, trailWidth);
        PixelFont::Number(settings.trailLength, trailLength);
        PixelFont::Number(settings.trailColor + 1, trailColor);

        int row = 0;
        auto add = [&](const char* label, const char* value,
                       uint32_t valueColor = 0) {
            Draw::Row(panelX, row, label, value, visualRow == row, valueColor);
            ++row;
        };

        if (expanded == Expanded::Box) {
            add("BOX", Draw::Toggle(settings.box), Draw::ToggleColor(settings.box));
            add("STYLE", BoxStyles[static_cast<int>(settings.boxStyle)]);
            add("WIDTH %", boxWidth);
            add("THICKNESS", boxThickness);
            add("FILL %", fillOpacity);
            add("BOX COLOR", boxColor, Palette::Box());
            add("FILL COLOR", fillColor, Palette::Fill());
            add("OUTLINE", boxOutlineThickness);
            add("OUTLINE COLOR", boxOutlineColor, Palette::BoxOutline());
            add("BACK", "");
            return;
        }

        if (expanded == Expanded::Health) {
            add("HEALTHBAR", Draw::Toggle(settings.health), Draw::ToggleColor(settings.health));
            add("HEALTH TEXT", Draw::Toggle(settings.healthText),
                Draw::ToggleColor(settings.healthText));
            add("RADIUS", healthRadius);
            add("STYLE", HealthStyles[static_cast<int>(settings.healthStyle)]);
            add("POSITION", HealthPositions[static_cast<int>(settings.healthPosition)]);
            add("WIDTH", healthWidth);
            add("SPACING", healthSpacing);
            add("SEGMENTS", healthSegments);
            add("COLOR", healthColor, Palette::Health());
            add("BG COLOR", healthBackground, Palette::HealthBackground());
            add("OUTLINE", healthOutlineThickness);
            add("OUT COLOR", healthOutlineColor, Palette::HealthOutline());
            add("BACK", "");
            return;
        }

        if (expanded == Expanded::Armor) {
            add("ARMOR", Draw::Toggle(settings.armor), Draw::ToggleColor(settings.armor));
            add("ARMOR TEXT", Draw::Toggle(settings.armorText),
                Draw::ToggleColor(settings.armorText));
            add("RADIUS", armorRadius);
            add("STYLE", HealthStyles[static_cast<int>(settings.armorStyle)]);
            add("POSITION", HealthPositions[static_cast<int>(settings.armorPosition)]);
            add("WIDTH", armorWidth);
            add("SPACING", armorSpacing);
            add("SEGMENTS", armorSegments);
            add("COLOR", armorColor, Palette::Armor());
            add("BG COLOR", armorBackground, Palette::ArmorBackground());
            add("OUTLINE", armorOutlineThickness);
            add("OUT COLOR", armorOutlineColor, Palette::ArmorOutline());
            add("BACK", "");
            return;
        }

        if (expanded == Expanded::Skeleton) {
            add("SKELETON", Draw::Toggle(settings.skeleton),
                Draw::ToggleColor(settings.skeleton));
            add("THICKNESS", skeletonThickness);
            add("COLOR", skeletonColor, Palette::Skeleton());
            add("BACK", "");
            return;
        }

        if (expanded == Expanded::Snapline) {
            add("SNAPLINE", Draw::Toggle(settings.snapline),
                Draw::ToggleColor(settings.snapline));
            add("ANCHOR", SnaplineStyles[static_cast<int>(settings.snaplineStyle)]);
            add("THICKNESS", snaplineThickness);
            add("COLOR", snaplineColor, Palette::Snapline());
            add("BACK", "");
            return;
        }

        if (expanded == Expanded::Trail) {
            add("TRAIL", Draw::Toggle(settings.trail), Draw::ToggleColor(settings.trail));
            add("WIDTH", trailWidth);
            add("LENGTH", trailLength);
            add("COLOR", trailColor, Palette::Trail());
            add("BACK", "");
            return;
        }

        add("BOX", Draw::Toggle(settings.box), Draw::ToggleColor(settings.box));
        add("HEALTHBAR", Draw::Toggle(settings.health), Draw::ToggleColor(settings.health));
        add("ARMOR", Draw::Toggle(settings.armor), Draw::ToggleColor(settings.armor));
        add("SKELETON", Draw::Toggle(settings.skeleton), Draw::ToggleColor(settings.skeleton));
        add("SNAPLINE", Draw::Toggle(settings.snapline), Draw::ToggleColor(settings.snapline));
        add("TRAIL", Draw::Toggle(settings.trail), Draw::ToggleColor(settings.trail));
        add("BACK", "");
    }
    inline void DrawRadarPanel(int panelX) {
        Draw::Panel(panelX, "RADAR SETTINGS", 6);

        char radarSize[4]{}, radarRangeNumber[4]{}, radarRange[5]{}, radarColor[4]{}, radarBackground[4]{};
        PixelFont::Number(settings.radarSize, radarSize);
        PixelFont::Number(settings.radarRange, radarRangeNumber);
        strcpy_s(radarRange, radarRangeNumber);
        strcat_s(radarRange, "M");
        PixelFont::Number(settings.radarColor + 1, radarColor);
        PixelFont::Number(settings.radarBackgroundColor + 1, radarBackground);

        int row = 0;
        auto add = [&](const char* label, const char* value,
                       uint32_t valueColor = 0) {
            Draw::Row(panelX, row, label, value, radarRow == row, valueColor);
            ++row;
        };

        add("RADAR", Draw::Toggle(settings.radar), Draw::ToggleColor(settings.radar));
        add("DISTANCE", radarRange);
        add("SIZE", radarSize);
        add("COLOR", radarColor, Palette::Radar());
        add("BG COLOR", radarBackground, Palette::RadarBackground());
        add("BACK", "");
    }

}
