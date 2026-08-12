#pragma once

#include <windows.h>
#include <algorithm>
#include <cstdint>

#include "App/Settings.hpp"
#include "Core/RendererCore.hpp"

namespace PixelMenu {
    using AppSettings::BoxStyle;
    using AppSettings::HealthStyle;
    using AppSettings::HealthPosition;
    using AppSettings::SnaplineStyle;

    enum class Page { Main, Visuals, Radar };
    enum class Expanded { None, Box, Health, Armor, Skeleton, Snapline, Trail };

    inline auto& settings = AppSettings::values;
    inline int& x = settings.menuX;
    inline int& y = settings.menuY;

    // Keep the uploaded menu geometry/style exactly as supplied.
    inline constexpr char Title[] = "";
    inline constexpr int ToggleKey = VK_F11;
    inline constexpr int Width = 210;
    inline constexpr int HeaderHeight = 8;
    inline constexpr int RowHeight = 24;
    inline constexpr int FooterHeight = 8;
    inline constexpr int MaxPanelHeight = HeaderHeight + 14 * RowHeight + FooterHeight;
    inline constexpr int Gap = 0;
    inline constexpr int Radius = 4;

    struct ThemeColors {
        uint32_t accent;
        uint32_t panel;
        uint32_t header;
        uint32_t border;
        uint32_t selection;
        uint32_t separator;
        uint32_t item;
        uint32_t itemActive;
        uint32_t value;
        uint32_t on;
        uint32_t off;
    };

    [[nodiscard]] inline ThemeColors CurrentTheme() {
        switch (settings.menuTheme) {
        case AppSettings::MenuTheme::Light: // PEARL
            return { 0xFF7568FF, 0xF7F7F7FA, 0xFFF3F3F6, 0xFFD4D4DC,
                     0xFFE9E7FF, 0xFFE1E1E7, 0xFF2B2B32, 0xFF111116,
                     0xFF595963, 0xFF5E52E8, 0xFF92929B };
        case AppSettings::MenuTheme::Emerald: // MINT
            return { 0xFF63DDB7, 0xF40B100F, 0xFF0B100F, 0xFF285144,
                     0xFF17382F, 0xFF1A2B26, 0xFFC6DED6, 0xFFF4FFFB,
                     0xFF91B5AA, 0xFF70E1BD, 0xFF828A87 };
        case AppSettings::MenuTheme::Ocean: // ICE
            return { 0xFF63B9F7, 0xF4090D12, 0xFF090D12, 0xFF284861,
                     0xFF173247, 0xFF182A39, 0xFFC7DBEB, 0xFFF7FBFF,
                     0xFF92AFC5, 0xFF79C8FF, 0xFF858B91 };
        default: // VIOLET
            return { 0xFF9A7CFF, 0xF40B0B10, 0xFF0B0B10, 0xFF40345E,
                     0xFF241B3A, 0xFF211B2C, 0xFFD0C7E8, 0xFFFAF8FF,
                     0xFFA89CBC, 0xFFB79CFF, 0xFF89818F };
        }
    }

    inline bool visible = true;
    inline Page page = Page::Main;
    inline Expanded expanded = Expanded::None;
    inline int mainRow = 0;
    inline int visualRow = 0;
    inline int radarRow = 0;

    inline void ClampToScreen() {
        const int maxWidth = Width;
        x = std::clamp(x, 0, std::max(0, RendererCore::width - maxWidth));
        y = std::clamp(y, 0, std::max(0, RendererCore::height - MaxPanelHeight));
    }
}
