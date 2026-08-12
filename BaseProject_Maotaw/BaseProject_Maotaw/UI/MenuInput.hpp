#pragma once

#include <windows.h>
#include <algorithm>
#include <array>

#include "App/Config.hpp"
#include "Core/Overlay.hpp"
#include "Core/RendererCore.hpp"
#include "UI/MenuState.hpp"

namespace PixelMenu::Input {
    enum class Entry {
        Theme, Box, Health, HealthText, HealthRadius, Armor, ArmorText, ArmorRadius,
        Skeleton, Snapline, Radar, Trail, Back,
        BoxStyle, BoxWidth, BoxThickness, FillOpacity, BoxColor, FillColor,
        BoxOutlineThickness, BoxOutlineColor,
        HealthStyle, HealthPosition, HealthWidth, HealthSpacing,
        HealthSegments, HealthColor, HealthBackgroundColor,
        HealthOutlineThickness, HealthOutlineColor,
        ArmorStyle, ArmorPosition, ArmorWidth, ArmorSpacing,
        ArmorSegments, ArmorColor, ArmorBackgroundColor,
        ArmorOutlineThickness, ArmorOutlineColor,
        SkeletonThickness, SkeletonColor,
        SnaplineStyle, SnaplineThickness, SnaplineColor,
        RadarSize, RadarRange, RadarColor, RadarBackgroundColor,
        TrailWidth, TrailLength, TrailColor
    };

    [[nodiscard]] inline bool Pressed(int key) {
        static std::array<bool, 256> previous{};
        const bool down = (GetAsyncKeyState(key) & 0x8000) != 0;
        const bool pressed = down && !previous[key];
        previous[key] = down;
        return pressed;
    }

    [[nodiscard]] inline int& CurrentRow() {
        if (page == Page::Visuals) return visualRow;
        if (page == Page::Radar) return radarRow;
        return mainRow;
    }

    [[nodiscard]] inline int CurrentRowCount() {
        if (page == Page::Main) return 4;
        if (page == Page::Radar) return 6;
        if (expanded == Expanded::Box) return 10;
        if (expanded == Expanded::Health) return 13;
        if (expanded == Expanded::Armor) return 13;
        if (expanded == Expanded::Skeleton) return 4;
        if (expanded == Expanded::Snapline) return 5;
        if (expanded == Expanded::Trail) return 5;
        return 7;
    }

    [[nodiscard]] inline Entry CurrentEntry() {
        if (page == Page::Radar) {
            static constexpr Entry rows[]{
                Entry::Radar, Entry::RadarRange, Entry::RadarSize,
                Entry::RadarColor, Entry::RadarBackgroundColor, Entry::Back
            };
            return rows[radarRow];
        }
        if (expanded == Expanded::Box) {
            static constexpr Entry rows[]{
                Entry::Box, Entry::BoxStyle, Entry::BoxWidth,
                Entry::BoxThickness, Entry::FillOpacity,
                Entry::BoxColor, Entry::FillColor,
                Entry::BoxOutlineThickness, Entry::BoxOutlineColor, Entry::Back
            };
            return rows[visualRow];
        }
        if (expanded == Expanded::Health) {
            static constexpr Entry rows[]{
                Entry::Health, Entry::HealthText, Entry::HealthRadius,
                Entry::HealthStyle, Entry::HealthPosition, Entry::HealthWidth,
                Entry::HealthSpacing, Entry::HealthSegments, Entry::HealthColor,
                Entry::HealthBackgroundColor, Entry::HealthOutlineThickness,
                Entry::HealthOutlineColor, Entry::Back
            };
            return rows[visualRow];
        }
        if (expanded == Expanded::Armor) {
            static constexpr Entry rows[]{
                Entry::Armor, Entry::ArmorText, Entry::ArmorRadius,
                Entry::ArmorStyle, Entry::ArmorPosition, Entry::ArmorWidth,
                Entry::ArmorSpacing, Entry::ArmorSegments, Entry::ArmorColor,
                Entry::ArmorBackgroundColor, Entry::ArmorOutlineThickness,
                Entry::ArmorOutlineColor, Entry::Back
            };
            return rows[visualRow];
        }
        if (expanded == Expanded::Skeleton) {
            static constexpr Entry rows[]{
                Entry::Skeleton, Entry::SkeletonThickness,
                Entry::SkeletonColor, Entry::Back
            };
            return rows[visualRow];
        }
        if (expanded == Expanded::Snapline) {
            static constexpr Entry rows[]{
                Entry::Snapline, Entry::SnaplineStyle,
                Entry::SnaplineThickness, Entry::SnaplineColor, Entry::Back
            };
            return rows[visualRow];
        }
        if (expanded == Expanded::Trail) {
            static constexpr Entry rows[]{
                Entry::Trail, Entry::TrailWidth,
                Entry::TrailLength, Entry::TrailColor, Entry::Back
            };
            return rows[visualRow];
        }
        static constexpr Entry rows[]{
            Entry::Box, Entry::Health, Entry::Armor, Entry::Skeleton,
            Entry::Snapline, Entry::Trail, Entry::Back
        };
        return rows[visualRow];
    }

    inline void Cycle(int& value, int minimum, int maximum, int direction) {
        value += direction;
        if (value > maximum) value = minimum;
        if (value < minimum) value = maximum;
    }

    inline void Adjust(int direction) {
        if (page == Page::Main) {
            if (mainRow == 0) settings.esp = direction > 0;
            else if (mainRow == 1) settings.radar = direction > 0;
            else if (mainRow == 2) {
                settings.espDistance = std::clamp(
                    settings.espDistance + direction * 20, 20, 500);
            } else if (mainRow == 3) {
                int theme = static_cast<int>(settings.menuTheme);
                Cycle(theme, 0, 3, direction);
                settings.menuTheme = static_cast<AppSettings::MenuTheme>(theme);
            }
            return;
        }

        const Entry entry = CurrentEntry();
        if (entry == Entry::Box) settings.box = direction > 0;
        else if (entry == Entry::Health) settings.health = direction > 0;
        else if (entry == Entry::HealthText) settings.healthText = direction > 0;
        else if (entry == Entry::Armor) settings.armor = direction > 0;
        else if (entry == Entry::ArmorText) settings.armorText = direction > 0;
        else if (entry == Entry::Skeleton) settings.skeleton = direction > 0;
        else if (entry == Entry::Snapline) settings.snapline = direction > 0;
        else if (entry == Entry::Radar) settings.radar = direction > 0;
        else if (entry == Entry::Trail) settings.trail = direction > 0;
        else if (entry == Entry::BoxStyle) {
            int style = static_cast<int>(settings.boxStyle);
            Cycle(style, 0, 4, direction);
            settings.boxStyle = static_cast<BoxStyle>(style);
        } else if (entry == Entry::BoxWidth) {
            settings.boxWidthPercent += direction * 5;
            if (settings.boxWidthPercent > 80) settings.boxWidthPercent = 30;
            if (settings.boxWidthPercent < 30) settings.boxWidthPercent = 80;
        } else if (entry == Entry::BoxThickness) {
            Cycle(settings.boxThickness, 1, 5, direction);
        } else if (entry == Entry::FillOpacity) {
            settings.fillOpacity += direction * 5;
            if (settings.fillOpacity > 60) settings.fillOpacity = 0;
            if (settings.fillOpacity < 0) settings.fillOpacity = 60;
        } else if (entry == Entry::BoxColor) {
            Cycle(settings.boxColor, 0, AppSettings::ColorCount - 1, direction);
        } else if (entry == Entry::FillColor) {
            Cycle(settings.fillColor, 0, AppSettings::ColorCount - 1, direction);
        } else if (entry == Entry::BoxOutlineThickness) {
            Cycle(settings.boxOutlineThickness, 0, 5, direction);
        } else if (entry == Entry::BoxOutlineColor) {
            Cycle(settings.boxOutlineColor, 0, AppSettings::ColorCount - 1, direction);
        } else if (entry == Entry::HealthStyle) {
            int style = static_cast<int>(settings.healthStyle);
            Cycle(style, 0, 2, direction);
            settings.healthStyle = static_cast<HealthStyle>(style);
        } else if (entry == Entry::HealthPosition) {
            int position = static_cast<int>(settings.healthPosition);
            Cycle(position, 0, 3, direction);
            settings.healthPosition = static_cast<HealthPosition>(position);
        } else if (entry == Entry::HealthRadius) {
            settings.healthRadius = std::clamp(settings.healthRadius + direction, 0, 6);
        } else if (entry == Entry::HealthWidth) {
            Cycle(settings.healthWidth, 1, 12, direction);
        } else if (entry == Entry::HealthSpacing) {
            Cycle(settings.healthSpacing, 0, 10, direction);
        } else if (entry == Entry::HealthSegments) {
            Cycle(settings.healthSegments, 2, 20, direction);
        } else if (entry == Entry::HealthColor) {
            Cycle(settings.healthColor, 0, AppSettings::ColorCount - 1, direction);
        } else if (entry == Entry::HealthBackgroundColor) {
            Cycle(settings.healthBackgroundColor, 0,
                  AppSettings::ColorCount - 1, direction);
        } else if (entry == Entry::HealthOutlineThickness) {
            Cycle(settings.healthOutlineThickness, 0, 5, direction);
        } else if (entry == Entry::HealthOutlineColor) {
            Cycle(settings.healthOutlineColor, 0, AppSettings::ColorCount - 1, direction);
        } else if (entry == Entry::ArmorStyle) {
            int style = static_cast<int>(settings.armorStyle);
            Cycle(style, 0, 2, direction);
            settings.armorStyle = static_cast<HealthStyle>(style);
        } else if (entry == Entry::ArmorPosition) {
            int position = static_cast<int>(settings.armorPosition);
            Cycle(position, 0, 3, direction);
            settings.armorPosition = static_cast<HealthPosition>(position);
        } else if (entry == Entry::ArmorRadius) {
            settings.armorRadius = std::clamp(settings.armorRadius + direction, 0, 6);
        } else if (entry == Entry::ArmorWidth) {
            Cycle(settings.armorWidth, 1, 12, direction);
        } else if (entry == Entry::ArmorSpacing) {
            Cycle(settings.armorSpacing, 0, 10, direction);
        } else if (entry == Entry::ArmorSegments) {
            Cycle(settings.armorSegments, 2, 20, direction);
        } else if (entry == Entry::ArmorColor) {
            Cycle(settings.armorColor, 0, AppSettings::ColorCount - 1, direction);
        } else if (entry == Entry::ArmorBackgroundColor) {
            Cycle(settings.armorBackgroundColor, 0, AppSettings::ColorCount - 1, direction);
        } else if (entry == Entry::ArmorOutlineThickness) {
            Cycle(settings.armorOutlineThickness, 0, 5, direction);
        } else if (entry == Entry::ArmorOutlineColor) {
            Cycle(settings.armorOutlineColor, 0, AppSettings::ColorCount - 1, direction);
        } else if (entry == Entry::SkeletonThickness) {
            Cycle(settings.skeletonThickness, 1, 5, direction);
        } else if (entry == Entry::SkeletonColor) {
            Cycle(settings.skeletonColor, 0, AppSettings::ColorCount - 1, direction);
        } else if (entry == Entry::SnaplineStyle) {
            int style = static_cast<int>(settings.snaplineStyle);
            Cycle(style, 0, 2, direction);
            settings.snaplineStyle = static_cast<SnaplineStyle>(style);
        } else if (entry == Entry::SnaplineThickness) {
            Cycle(settings.snaplineThickness, 1, 5, direction);
        } else if (entry == Entry::SnaplineColor) {
            Cycle(settings.snaplineColor, 0, AppSettings::ColorCount - 1, direction);
        } else if (entry == Entry::RadarSize) {
            settings.radarSize += direction * 10;
            if (settings.radarSize > 260) settings.radarSize = 80;
            if (settings.radarSize < 80) settings.radarSize = 260;
        } else if (entry == Entry::RadarRange) {
            settings.radarRange = std::clamp(
                settings.radarRange + direction * 20, 20, 500);
        } else if (entry == Entry::RadarColor) {
            Cycle(settings.radarColor, 0, AppSettings::ColorCount - 1, direction);
        } else if (entry == Entry::RadarBackgroundColor) {
            Cycle(settings.radarBackgroundColor, 0, AppSettings::ColorCount - 1, direction);
        } else if (entry == Entry::TrailWidth) {
            Cycle(settings.trailWidth, 1, 5, direction);
        } else if (entry == Entry::TrailLength) {
            Cycle(settings.trailLength, 4, 32, direction);
        } else if (entry == Entry::TrailColor) {
            Cycle(settings.trailColor, 0, AppSettings::ColorCount - 1, direction);
        }
    }

    inline void OpenSelected() {
        if (page == Page::Main) {
            if (mainRow == 0) {
                page = Page::Visuals;
                expanded = Expanded::None;
                visualRow = 0;
            } else if (mainRow == 1) {
                page = Page::Radar;
                radarRow = 0;
            }
            // DISTANCE is adjusted directly with left/right on the main page.
            return;
        }

        if (page == Page::Radar) {
            if (CurrentEntry() == Entry::Back) {
                radarRow = 0;
                mainRow = 1;
                page = Page::Main;
            }
            return;
        }

        const Entry entry = CurrentEntry();
        if (entry == Entry::Box) {
            expanded = expanded == Expanded::Box ? Expanded::None : Expanded::Box;
            visualRow = expanded == Expanded::None ? 0 : 0;
        } else if (entry == Entry::Health) {
            expanded = expanded == Expanded::Health ? Expanded::None : Expanded::Health;
            visualRow = expanded == Expanded::None ? 1 : 0;
        } else if (entry == Entry::Armor) {
            expanded = expanded == Expanded::Armor ? Expanded::None : Expanded::Armor;
            visualRow = expanded == Expanded::None ? 2 : 0;
        } else if (entry == Entry::Skeleton) {
            expanded = expanded == Expanded::Skeleton ? Expanded::None : Expanded::Skeleton;
            visualRow = expanded == Expanded::None ? 3 : 0;
        } else if (entry == Entry::Snapline) {
            expanded = expanded == Expanded::Snapline ? Expanded::None : Expanded::Snapline;
            visualRow = expanded == Expanded::None ? 4 : 0;
        } else if (entry == Entry::Trail) {
            expanded = expanded == Expanded::Trail ? Expanded::None : Expanded::Trail;
            visualRow = expanded == Expanded::None ? 5 : 0;
        } else if (entry == Entry::Back) {
            if (expanded != Expanded::None) {
                const Expanded previous = expanded;
                expanded = Expanded::None;
                visualRow = previous == Expanded::Box ? 0
                          : previous == Expanded::Health ? 1
                          : previous == Expanded::Armor ? 2
                          : previous == Expanded::Skeleton ? 3
                          : previous == Expanded::Snapline ? 4 : 5;
            } else {
                visualRow = 0;
                mainRow = 0;
                page = Page::Main;
            }
        }
    }

    inline void Update() {
        if (Pressed(ToggleKey)) visible = !visible;
        if (!visible) return;

        int& row = CurrentRow();
        const int count = CurrentRowCount();
        if (Pressed(VK_UP)) row = (row + count - 1) % count;
        if (Pressed(VK_DOWN)) row = (row + 1) % count;

        const bool left = Pressed(VK_LEFT);
        const bool right = Pressed(VK_RIGHT);
        if (left || right) {
            const auto previous = settings;
            Adjust(left ? -1 : 1);
            if (!(settings == previous)) Config::RequestSave();
        }
        if (Pressed(VK_RETURN)) OpenSelected();
    }

    inline void UpdateDragging() {
        static bool dragging = false;
        static POINT offset{};
        const bool down = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0;
        if (!down && !dragging) return;

        POINT cursor{};
        GetCursorPos(&cursor);
        ScreenToClient(Overlay::hwnd, &cursor);
        const bool overHeader = cursor.x >= x && cursor.x <= x + Width &&
                                cursor.y >= y && cursor.y <= y + HeaderHeight;
        if (down && !dragging && overHeader) {
            dragging = true;
            offset = { cursor.x - x, cursor.y - y };
        }
        if (!down && dragging) {
            dragging = false;
            Config::RequestSave();
        }

        if (dragging) {
            const int totalWidth = Width;
            const int totalHeight = HeaderHeight +
                                    CurrentRowCount() * RowHeight + FooterHeight;
            const int newX = std::clamp(static_cast<int>(cursor.x - offset.x), 0,
                           std::max(0, RendererCore::width - totalWidth));
            const int newY = std::clamp(static_cast<int>(cursor.y - offset.y), 0,
                           std::max(0, RendererCore::height - totalHeight));
            if (newX != x || newY != y) {
                x = newX;
                y = newY;
                Config::RequestSave();
            }
        }
    }
}
