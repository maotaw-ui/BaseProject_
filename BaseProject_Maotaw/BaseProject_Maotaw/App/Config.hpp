#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <cstring>

#include "App/Settings.hpp"

namespace Config {
    inline constexpr auto SaveDelay = std::chrono::milliseconds(350);
    inline bool savePending = false;
    inline std::chrono::steady_clock::time_point saveAt{};

    [[nodiscard]] inline bool GetTextPaths(
        wchar_t (&filePath)[MAX_PATH], wchar_t (&tempPath)[MAX_PATH],
        wchar_t (&backupPath)[MAX_PATH]) {
        wchar_t localAppData[MAX_PATH]{};
        const DWORD length = GetEnvironmentVariableW(
            L"LOCALAPPDATA", localAppData, MAX_PATH);
        if (!length || length >= MAX_PATH) return false;

        wchar_t folder[MAX_PATH]{};
        if (swprintf_s(folder, L"%ls\\BaseProject_Maotaw", localAppData) < 0) return false;
        if (!CreateDirectoryW(folder, nullptr) &&
            GetLastError() != ERROR_ALREADY_EXISTS) return false;

        return swprintf_s(filePath, L"%ls\\settings.cfg", folder) >= 0 &&
               swprintf_s(tempPath, L"%ls\\settings.tmp", folder) >= 0 &&
               swprintf_s(backupPath, L"%ls\\settings.bak", folder) >= 0;
    }

    inline bool Save() {
        wchar_t filePath[MAX_PATH]{}, tempPath[MAX_PATH]{}, backupPath[MAX_PATH]{};
        if (!GetTextPaths(filePath, tempPath, backupPath)) return false;

        const auto& s = AppSettings::values;
        char text[4096]{};
        const int length = sprintf_s(text, sizeof(text),
            "# BaseProject_Maotaw visuals - PixelMenu UI\r\n"
            "esp=%d\r\nbox=%d\r\nskeleton=%d\r\nhealth=%d\r\narmor=%d\r\n"
            "health_text=%d\r\narmor_text=%d\r\nhealth_radius=%d\r\narmor_radius=%d\r\n"
            "snapline=%d\r\nradar=%d\r\ntrail=%d\r\n"
            "esp_distance=%d\r\n"
            "box_style=%d\r\nbox_width_percent=%d\r\nbox_thickness=%d\r\n"
            "fill_opacity=%d\r\nbox_color=%d\r\nfill_color=%d\r\n"
            "box_outline_thickness=%d\r\nbox_outline_color=%d\r\n"
            "skeleton_thickness=%d\r\nskeleton_color=%d\r\n"
            "health_style=%d\r\nhealth_position=%d\r\nhealth_width=%d\r\n"
            "health_spacing=%d\r\nhealth_segments=%d\r\nhealth_color=%d\r\n"
            "health_background_color=%d\r\nhealth_outline_thickness=%d\r\nhealth_outline_color=%d\r\n"
            "armor_style=%d\r\narmor_position=%d\r\narmor_width=%d\r\n"
            "armor_spacing=%d\r\narmor_segments=%d\r\narmor_color=%d\r\n"
            "armor_background_color=%d\r\narmor_outline_thickness=%d\r\narmor_outline_color=%d\r\n"
            "snapline_style=%d\r\nsnapline_thickness=%d\r\nsnapline_color=%d\r\n"
            "radar_size=%d\r\nradar_range=%d\r\nradar_color=%d\r\nradar_background_color=%d\r\n"
            "trail_width=%d\r\ntrail_length=%d\r\ntrail_color=%d\r\n"
            "menu_theme=%d\r\n"
            "menu_x=%d\r\nmenu_y=%d\r\n",
            s.esp, s.box, s.skeleton, s.health, s.armor,
            s.healthText, s.armorText, s.healthRadius, s.armorRadius,
            s.snapline, s.radar, s.trail,
            s.espDistance,
            static_cast<int>(s.boxStyle), s.boxWidthPercent, s.boxThickness,
            s.fillOpacity, s.boxColor, s.fillColor,
            s.boxOutlineThickness, s.boxOutlineColor,
            s.skeletonThickness, s.skeletonColor,
            static_cast<int>(s.healthStyle),
            static_cast<int>(s.healthPosition), s.healthWidth,
            s.healthSpacing, s.healthSegments, s.healthColor,
            s.healthBackgroundColor, s.healthOutlineThickness, s.healthOutlineColor,
            static_cast<int>(s.armorStyle), static_cast<int>(s.armorPosition),
            s.armorWidth, s.armorSpacing, s.armorSegments, s.armorColor,
            s.armorBackgroundColor, s.armorOutlineThickness, s.armorOutlineColor,
            static_cast<int>(s.snaplineStyle), s.snaplineThickness,
            s.snaplineColor,
            s.radarSize, s.radarRange, s.radarColor, s.radarBackgroundColor,
            s.trailWidth, s.trailLength, s.trailColor,
            static_cast<int>(s.menuTheme),
            s.menuX, s.menuY);
        if (length <= 0 || length >= static_cast<int>(sizeof(text))) return false;

        const HANDLE file = CreateFileW(tempPath, GENERIC_WRITE, 0, nullptr,
            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return false;

        DWORD written = 0;
        const bool ok = WriteFile(file, text, static_cast<DWORD>(length),
                                  &written, nullptr) &&
                        written == static_cast<DWORD>(length) &&
                        FlushFileBuffers(file);
        CloseHandle(file);

        if (!ok) {
            DeleteFileW(tempPath);
            return false;
        }

        CopyFileW(filePath, backupPath, FALSE);
        if (!MoveFileExW(tempPath, filePath,
                MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH)) {
            DeleteFileW(tempPath);
            return false;
        }
        return true;
    }

    inline void RequestSave() {
        savePending = true;
        saveAt = std::chrono::steady_clock::now() + SaveDelay;
    }

    inline void Update() {
        if (savePending && std::chrono::steady_clock::now() >= saveAt) {
            savePending = false;
            Save();
        }
    }

    inline void Flush() {
        savePending = false;
        Save();
    }

    inline bool ApplyTextValue(const char* key, int value) {
        auto& s = AppSettings::values;
        if (!std::strcmp(key, "esp")) s.esp = value != 0;
        else if (!std::strcmp(key, "box")) s.box = value != 0;
        else if (!std::strcmp(key, "skeleton")) s.skeleton = value != 0;
        else if (!std::strcmp(key, "health")) s.health = value != 0;
        else if (!std::strcmp(key, "armor")) s.armor = value != 0;
        else if (!std::strcmp(key, "health_text")) s.healthText = value != 0;
        else if (!std::strcmp(key, "armor_text")) s.armorText = value != 0;
        else if (!std::strcmp(key, "health_radius")) s.healthRadius = value;
        else if (!std::strcmp(key, "armor_radius")) s.armorRadius = value;
        else if (!std::strcmp(key, "health_rounded")) s.healthRadius = value != 0 ? 3 : 0;
        else if (!std::strcmp(key, "snapline")) s.snapline = value != 0;
        else if (!std::strcmp(key, "radar")) s.radar = value != 0;
        else if (!std::strcmp(key, "trail")) s.trail = value != 0;
        else if (!std::strcmp(key, "esp_distance")) s.espDistance = value;
        else if (!std::strcmp(key, "box_style"))
            s.boxStyle = static_cast<AppSettings::BoxStyle>(value);
        else if (!std::strcmp(key, "box_width_percent")) s.boxWidthPercent = value;
        else if (!std::strcmp(key, "box_thickness")) s.boxThickness = value;
        else if (!std::strcmp(key, "fill_opacity")) s.fillOpacity = value;
        else if (!std::strcmp(key, "box_color")) s.boxColor = value;
        else if (!std::strcmp(key, "fill_color")) s.fillColor = value;
        else if (!std::strcmp(key, "box_outline_thickness")) s.boxOutlineThickness = value;
        else if (!std::strcmp(key, "box_outline_color")) s.boxOutlineColor = value;
        else if (!std::strcmp(key, "skeleton_thickness")) s.skeletonThickness = value;
        else if (!std::strcmp(key, "skeleton_color")) s.skeletonColor = value;
        else if (!std::strcmp(key, "health_style"))
            s.healthStyle = static_cast<AppSettings::HealthStyle>(value);
        else if (!std::strcmp(key, "health_position"))
            s.healthPosition = static_cast<AppSettings::HealthPosition>(value);
        else if (!std::strcmp(key, "health_width")) s.healthWidth = value;
        else if (!std::strcmp(key, "health_spacing")) s.healthSpacing = value;
        else if (!std::strcmp(key, "health_segments")) s.healthSegments = value;
        else if (!std::strcmp(key, "health_color")) s.healthColor = value;
        else if (!std::strcmp(key, "health_background_color"))
            s.healthBackgroundColor = value;
        else if (!std::strcmp(key, "health_outline_thickness")) s.healthOutlineThickness = value;
        else if (!std::strcmp(key, "health_outline_color")) s.healthOutlineColor = value;
        else if (!std::strcmp(key, "armor_style"))
            s.armorStyle = static_cast<AppSettings::HealthStyle>(value);
        else if (!std::strcmp(key, "armor_position"))
            s.armorPosition = static_cast<AppSettings::HealthPosition>(value);
        else if (!std::strcmp(key, "armor_width")) s.armorWidth = value;
        else if (!std::strcmp(key, "armor_spacing")) s.armorSpacing = value;
        else if (!std::strcmp(key, "armor_segments")) s.armorSegments = value;
        else if (!std::strcmp(key, "armor_color")) s.armorColor = value;
        else if (!std::strcmp(key, "armor_background_color")) s.armorBackgroundColor = value;
        else if (!std::strcmp(key, "armor_outline_thickness")) s.armorOutlineThickness = value;
        else if (!std::strcmp(key, "armor_outline_color")) s.armorOutlineColor = value;
        else if (!std::strcmp(key, "snapline_style"))
            s.snaplineStyle = static_cast<AppSettings::SnaplineStyle>(value);
        else if (!std::strcmp(key, "snapline_thickness")) s.snaplineThickness = value;
        else if (!std::strcmp(key, "snapline_color")) s.snaplineColor = value;
        else if (!std::strcmp(key, "radar_size")) s.radarSize = value;
        else if (!std::strcmp(key, "radar_range")) s.radarRange = value;
        else if (!std::strcmp(key, "radar_color")) s.radarColor = value;
        else if (!std::strcmp(key, "radar_background_color")) s.radarBackgroundColor = value;
        else if (!std::strcmp(key, "trail_width")) s.trailWidth = value;
        else if (!std::strcmp(key, "trail_length")) s.trailLength = value;
        else if (!std::strcmp(key, "trail_color")) s.trailColor = value;
        else if (!std::strcmp(key, "menu_theme"))
            s.menuTheme = static_cast<AppSettings::MenuTheme>(value);
        else if (!std::strcmp(key, "menu_x")) s.menuX = value;
        else if (!std::strcmp(key, "menu_y")) s.menuY = value;
        else return false;
        return true;
    }

    inline bool LoadTextFile(const wchar_t* path) {
        const HANDLE file = CreateFileW(path, GENERIC_READ, FILE_SHARE_READ,
            nullptr, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, nullptr);
        if (file == INVALID_HANDLE_VALUE) return false;

        const DWORD size = GetFileSize(file, nullptr);
        if (!size || size >= 4096) {
            CloseHandle(file);
            return false;
        }

        char text[4096]{};
        DWORD bytesRead = 0;
        const bool readOk = ReadFile(file, text, size, &bytesRead, nullptr) &&
                            bytesRead == size;
        CloseHandle(file);
        if (!readOk) return false;
        text[size] = '\0';

        int recognized = 0;
        char* context = nullptr;
        for (char* line = strtok_s(text, "\r\n", &context);
             line; line = strtok_s(nullptr, "\r\n", &context)) {
            if (*line == '#' || *line == '\0') continue;
            char* equals = std::strchr(line, '=');
            if (!equals) continue;
            *equals = '\0';
            if (ApplyTextValue(line, std::atoi(equals + 1))) ++recognized;
        }

        if (recognized < 8) return false;
        AppSettings::Sanitize();
        return true;
    }

    inline bool Load() {
        wchar_t filePath[MAX_PATH]{}, tempPath[MAX_PATH]{}, backupPath[MAX_PATH]{};
        if (!GetTextPaths(filePath, tempPath, backupPath)) return false;

        const AppSettings::Values defaults{};
        AppSettings::values = defaults;
        if (LoadTextFile(filePath)) return true;
        AppSettings::values = defaults;
        if (LoadTextFile(backupPath)) {
            Save();
            return true;
        }

        AppSettings::values = defaults;
        return Save();
    }
}
