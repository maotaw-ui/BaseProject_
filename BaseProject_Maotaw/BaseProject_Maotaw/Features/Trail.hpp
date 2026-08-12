#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <unordered_map>
#include <vector>

#include "App/Settings.hpp"
#include "Core/MathCore.hpp"
#include "Core/RendererCore.hpp"
#include "Game/EntityCache.hpp"
#include "UI/Palette.hpp"

namespace Trail {
    inline constexpr int MaxSamples = 32;
    inline constexpr auto SampleInterval = std::chrono::milliseconds(40);
    inline constexpr auto SampleLifetime = std::chrono::milliseconds(1100);

    struct Sample {
        vec3 position{};
        std::chrono::steady_clock::time_point created{};
    };

    struct Track {
        std::array<Sample, MaxSamples> samples{};
        int next = 0;
        int count = 0;
        std::chrono::steady_clock::time_point lastSample{};
        std::chrono::steady_clock::time_point lastSeen{};
    };

    inline std::unordered_map<uintptr_t, Track> tracks;

    [[nodiscard]] inline float DistanceSquared(const vec3& a, const vec3& b) {
        const float x = a.x - b.x;
        const float y = a.y - b.y;
        const float z = a.z - b.z;
        return x * x + y * y + z * z;
    }

    inline void AddSample(Track& track, const vec3& position,
                          std::chrono::steady_clock::time_point now) {
        if (track.count > 0) {
            const int latest = (track.next + MaxSamples - 1) % MaxSamples;
            if (DistanceSquared(track.samples[latest].position, position) < 4.0f)
                return;
        }

        track.samples[track.next] = { position, now };
        track.next = (track.next + 1) % MaxSamples;
        track.count = std::min(track.count + 1, MaxSamples);
        track.lastSample = now;
    }

    inline void PruneExpired(Track& track,
                             std::chrono::steady_clock::time_point now) {
        while (track.count > 0) {
            const int oldest = (track.next + MaxSamples - track.count) % MaxSamples;
            if (now - track.samples[oldest].created <= SampleLifetime) break;
            --track.count;
        }
    }

    inline void DrawThickLine(int x1, int y1, int x2, int y2,
                              int thickness, uint32_t color) {
        thickness = std::clamp(thickness, 1, 5);
        const int first = -(thickness / 2);
        const int last = first + thickness - 1;
        for (int offset = first; offset <= last; ++offset)
            RendererCore::DrawLineFast(x1 + offset, y1, x2 + offset, y2, color);
    }

    inline void RenderTrack(const Track& track, const Matrix4x4& matrix,
                            std::chrono::steady_clock::time_point now) {
        const int limit = std::min(track.count, AppSettings::values.trailLength);
        if (limit < 2) return;

        const int oldest = (track.next + MaxSamples - limit) % MaxSamples;
        int previousX = 0, previousY = 0;
        bool previousVisible = false;

        for (int i = 0; i < limit; ++i) {
            const int index = (oldest + i) % MaxSamples;
            int screenX = 0, screenY = 0;
            const bool visible = Math::WorldToScreen(
                track.samples[index].position, matrix,
                RendererCore::width, RendererCore::height, screenX, screenY);

            if (visible && previousVisible) {
                const auto age = std::chrono::duration_cast<std::chrono::milliseconds>(
                    now - track.samples[index].created);
                const int remaining = static_cast<int>(SampleLifetime.count() - age.count());
                const uint8_t alpha = static_cast<uint8_t>(std::clamp(
                    35 + 200 * remaining / static_cast<int>(SampleLifetime.count()),
                    35, 235));
                DrawThickLine(previousX, previousY, screenX, screenY,
                              AppSettings::values.trailWidth,
                              RendererCore::WithAlpha(Palette::Trail(), alpha));
            }

            previousX = screenX;
            previousY = screenY;
            previousVisible = visible;
        }
    }

    inline void UpdateAndRender(
        const EntityCache::FrameData& frame,
        const std::vector<EntityCache::CachedEntity>& entities) {
        if (!AppSettings::values.trail) {
            tracks.clear();
            return;
        }

        const auto now = std::chrono::steady_clock::now();
        for (const auto& entity : entities) {
            if (!entity.pawn || !entity.originValid ||
                entity.team == frame.localTeam ||
                entity.health <= 0 || entity.health > 100) continue;
            if (!Math::WithinMeters(entity.origin, frame.localPosition,
                                    AppSettings::values.espDistance)) continue;

            Track& track = tracks[entity.pawn];
            track.lastSeen = now;
            PruneExpired(track, now);
            if (track.count == 0 || now - track.lastSample >= SampleInterval)
                AddSample(track, entity.origin, now);
            RenderTrack(track, frame.viewMatrix, now);
        }

        for (auto it = tracks.begin(); it != tracks.end();) {
            if (now - it->second.lastSeen > std::chrono::seconds(1))
                it = tracks.erase(it);
            else
                ++it;
        }
    }
}
