#pragma once

#include <algorithm>
#include <array>
#include <chrono>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <utility>
#include <vector>

#include "App/Settings.hpp"
#include "Core/MathCore.hpp"
#include "Core/Memory.hpp"
#include "Game/Offsets.hpp"

namespace EntityCache {
    enum class BoneId : std::size_t {
        Waist = 0, Neck = 5, Head = 6,
        ShoulderLeft = 8, ForeLeft = 9, HandLeft = 11,
        ShoulderRight = 13, ForeRight = 14, HandRight = 16,
        KneeLeft = 23, FeetLeft = 24,
        KneeRight = 26, FeetRight = 27
    };

    inline constexpr std::array<BoneId, 13> BoneIds{
        BoneId::Waist, BoneId::Neck, BoneId::Head,
        BoneId::ShoulderLeft, BoneId::ForeLeft, BoneId::HandLeft,
        BoneId::ShoulderRight, BoneId::ForeRight, BoneId::HandRight,
        BoneId::KneeLeft, BoneId::FeetLeft, BoneId::KneeRight, BoneId::FeetRight
    };

    using BoneArray = std::array<vec3, BoneIds.size()>;

    // The skeleton renderer only references bones up to index 73. Reading 74
    // bones keeps one contiguous RPM read, but avoids fetching the old 96/128
    // bone blocks every update.
    inline constexpr std::size_t SkeletonBoneCount = 74;
    inline constexpr std::size_t DebugBoneCount = SkeletonBoneCount;
    using DebugBoneArray = std::array<vec3, DebugBoneCount>;

    inline void ExtractSelectedBones(const DebugBoneArray& source,
                                     BoneArray& destination) {
        for (std::size_t i = 0; i < BoneIds.size(); ++i) {
            const auto index = static_cast<std::size_t>(BoneIds[i]);
            if (index < source.size()) destination[i] = source[index];
        }
    }

    template <std::size_t Count>
    [[nodiscard]] inline bool ReadBoneRange(uintptr_t boneAddress,
                                            DebugBoneArray& bones) {
        static_assert(Count > 0 && Count <= DebugBoneCount);
        constexpr std::size_t BoneStride = 32;
        constexpr std::size_t BufferSize =
            (Count - 1) * BoneStride + sizeof(vec3);
        std::array<std::byte, BufferSize> bytes{};
        if (!MEMORY::TryRead(boneAddress, bytes)) return false;

        for (std::size_t i = 0; i < Count; ++i)
            std::memcpy(&bones[i], bytes.data() + i * BoneStride, sizeof(vec3));
        return true;
    }

    [[nodiscard]] inline bool ReadSkeletonBones(uintptr_t boneAddress,
                                                DebugBoneArray& bones) {
        return ReadBoneRange<SkeletonBoneCount>(boneAddress, bones);
    }

    [[nodiscard]] inline bool ReadDebugBones(uintptr_t boneAddress,
                                             DebugBoneArray& bones) {
        return ReadSkeletonBones(boneAddress, bones);
    }

    [[nodiscard]] inline bool ReadBones(uintptr_t boneAddress,
                                        BoneArray& bones) {
        DebugBoneArray source{};
        if (!ReadSkeletonBones(boneAddress, source)) return false;
        ExtractSelectedBones(source, bones);
        return true;
    }

    using Clock = std::chrono::steady_clock;
    using TimePoint = Clock::time_point;

    struct CachedEntity {
        int slot = 0;
        uintptr_t controller = 0;
        uintptr_t pawn = 0;
        uintptr_t sceneNode = 0;
        uintptr_t boneArray = 0;
        int team = 0;
        int health = 0;
        int armor = 0;
        vec3 origin{};
        bool originValid = false;
        vec3 boneOrigin{};
        BoneArray bones{};
        bool bonesValid = false;
        DebugBoneArray debugBones{};
        bool debugBonesValid = false;
        TimePoint lastOriginUpdate{};
        TimePoint lastHealthUpdate{};
        TimePoint lastBoneUpdate{};
        int boneReadFailures = 0;
    };

    inline constexpr std::array<std::size_t, 23> SkeletonRenderIndices{
        1, 2, 3, 4, 5, 6, 7, 9, 10, 11, 12,
        13, 14, 17, 18, 19, 20, 21, 22, 50, 71, 73, 0
    };

    [[nodiscard]] inline bool IsFinitePoint(const vec3& point) {
        return std::isfinite(point.x) && std::isfinite(point.y) &&
               std::isfinite(point.z);
    }

    [[nodiscard]] inline bool ValidateSkeletonSnapshot(
        const CachedEntity& entity, const DebugBoneArray& candidate) {
        if (!entity.originValid || !IsFinitePoint(entity.origin)) return false;

        // A valid player skeleton must stay close to the pawn origin. This
        // rejects transient bad bone-array pointers before they ever replace
        // the last good snapshot.
        constexpr float MaxOriginDistance = 150.0f;
        constexpr float MaxOriginDistanceSq =
            MaxOriginDistance * MaxOriginDistance;

        for (const std::size_t index : SkeletonRenderIndices) {
            if (index >= candidate.size()) return false;
            const vec3& bone = candidate[index];
            if (!IsFinitePoint(bone)) return false;
            if (Math::DistanceSquared(bone, entity.origin) > MaxOriginDistanceSq)
                return false;
        }

        // If we already have a good frame, compare bone positions relative to
        // the player origin. Real animation can move limbs quickly, but a
        // one-frame pointer glitch produces a much larger relative teleport.
        if (entity.debugBonesValid) {
            constexpr float MaxRelativeJump = 48.0f;
            constexpr float MaxRelativeJumpSq =
                MaxRelativeJump * MaxRelativeJump;

            for (const std::size_t index : SkeletonRenderIndices) {
                const vec3 oldRelative{
                    entity.debugBones[index].x - entity.boneOrigin.x,
                    entity.debugBones[index].y - entity.boneOrigin.y,
                    entity.debugBones[index].z - entity.boneOrigin.z
                };
                const vec3 newRelative{
                    candidate[index].x - entity.origin.x,
                    candidate[index].y - entity.origin.y,
                    candidate[index].z - entity.origin.z
                };
                if (Math::DistanceSquared(oldRelative, newRelative) >
                    MaxRelativeJumpSq)
                    return false;
            }
        }

        return true;
    }

    struct FrameData {
        uintptr_t localPawn = 0;
        vec3 localPosition{};
        int localTeam = 0;
        Matrix4x4 viewMatrix{};
        vec3 viewAngles{};
        bool viewAnglesValid = false;
        bool valid = false;
        TimePoint lastPositionUpdate{};
        TimePoint lastMatrixUpdate{};
        TimePoint lastViewAnglesUpdate{};
    };

    // Static addresses are relatively cheap to refresh and are rebuilt in
    // small chunks. Live positions/matrix stay independent from this rebuild.
    inline constexpr auto RefreshInterval = std::chrono::milliseconds(250);
    inline constexpr auto OriginFreshness = std::chrono::milliseconds(120);
    inline constexpr auto FrameFreshness = std::chrono::milliseconds(80);
    inline constexpr auto BoneFreshness = std::chrono::milliseconds(90);
    inline constexpr auto BoneRefreshInterval = std::chrono::milliseconds(10);
    inline constexpr int MaxSlots = 64;
    inline constexpr int SlotsPerFrame = 8;
    inline constexpr int BoneEntitiesPerFrame = 12;

    inline std::vector<CachedEntity> entities;
    inline std::vector<CachedEntity> pendingEntities;
    inline FrameData frame;
    inline uintptr_t entityList = 0;
    inline int nextSlot = MaxSlots + 1;
    inline std::size_t boneCursor = 0;
    inline TimePoint nextRefresh{};

    [[nodiscard]] inline bool IsFresh(TimePoint stamp, TimePoint now,
                                      Clock::duration maximumAge) {
        return stamp != TimePoint{} && now >= stamp && now - stamp <= maximumAge;
    }

    [[nodiscard]] inline uintptr_t ResolvePawn(uintptr_t list, uint32_t handle) {
        if (!list || !handle || handle == 0xFFFFFFFFu) return 0;

        const uintptr_t entry = MEMORY::Read<uintptr_t>(
            list + 8 * ((handle & 0x7FFFu) >> 9) + 16);
        return entry ? MEMORY::Read<uintptr_t>(entry + 112 * (handle & 0x1FFu)) : 0;
    }

    inline bool ResolveBoneArray(CachedEntity& entity) {
        uintptr_t sceneNode = 0;
        if (!MEMORY::TryRead(entity.pawn + m_pGameSceneNode, sceneNode) ||
            !sceneNode) {
            entity.sceneNode = 0;
            entity.boneArray = 0;
            return false;
        }

        uintptr_t boneArray = 0;
        if (!MEMORY::TryRead(sceneNode + m_modelState + 0x80, boneArray) ||
            !boneArray) {
            entity.sceneNode = sceneNode;
            entity.boneArray = 0;
            return false;
        }

        entity.sceneNode = sceneNode;
        entity.boneArray = boneArray;
        return true;
    }

    inline void BeginRefresh() {
        const auto now = Clock::now();
        uintptr_t localPawn = 0;
        uintptr_t list = 0;
        MEMORY::TryRead(MEMORY::baseAddress + dwLocalPlayerPawn, localPawn);
        MEMORY::TryRead(MEMORY::baseAddress + dwEntityList, list);

        if (!localPawn || !list) {
            frame.localPawn = 0;
            frame.valid = false;
            frame.viewAnglesValid = false;
            entityList = 0;
            entities.clear();
            pendingEntities.clear();
            nextSlot = MaxSlots + 1;
            nextRefresh = now + RefreshInterval;
            return;
        }

        frame.localPawn = localPawn;
        entityList = list;
        MEMORY::TryRead(frame.localPawn + m_iTeamNum, frame.localTeam);

        pendingEntities.clear();
        pendingEntities.reserve(MaxSlots);
        nextSlot = 1;
    }

    inline void ScanSlots() {
        if (nextSlot > MaxSlots || !entityList) return;

        const int finalSlot = (std::min)(nextSlot + SlotsPerFrame - 1, MaxSlots);
        for (; nextSlot <= finalSlot; ++nextSlot) {
            const uintptr_t listEntry = MEMORY::Read<uintptr_t>(
                entityList + 8 * ((nextSlot & 0x7FFF) >> 9) + 16);
            if (!listEntry) continue;

            const uintptr_t controller = MEMORY::Read<uintptr_t>(
                listEntry + 112 * (nextSlot & 0x1FF));
            if (!controller) continue;

            const uint32_t handle = MEMORY::Read<uint32_t>(controller + m_hPlayerPawn);
            const uintptr_t pawn = ResolvePawn(entityList, handle);
            if (!pawn || pawn == frame.localPawn) continue;

            CachedEntity cached{};
            cached.slot = nextSlot;
            cached.controller = controller;
            cached.pawn = pawn;
            MEMORY::TryRead(pawn + m_iTeamNum, cached.team);

            // Reuse every live/cache value when the pawn did not change. This
            // prevents periodic entity-list rebuilds from producing a visual
            // hitch or forcing bone pointers to be resolved again.
            const auto previous = std::find_if(
                entities.begin(), entities.end(),
                [pawn](const CachedEntity& entity) { return entity.pawn == pawn; });
            if (previous != entities.end()) {
                cached.sceneNode = previous->sceneNode;
                cached.boneArray = previous->boneArray;
                cached.health = previous->health;
                cached.armor = previous->armor;
                cached.origin = previous->origin;
                cached.originValid = previous->originValid;
                cached.boneOrigin = previous->boneOrigin;
                cached.bones = previous->bones;
                cached.bonesValid = previous->bonesValid;
                cached.debugBones = previous->debugBones;
                cached.debugBonesValid = previous->debugBonesValid;
                cached.lastOriginUpdate = previous->lastOriginUpdate;
                cached.lastHealthUpdate = previous->lastHealthUpdate;
                cached.lastBoneUpdate = previous->lastBoneUpdate;
                cached.boneReadFailures = previous->boneReadFailures;
            }

            pendingEntities.push_back(cached);
        }

        if (nextSlot > MaxSlots) {
            entities.swap(pendingEntities);
            if (boneCursor >= entities.size()) boneCursor = 0;
            nextRefresh = Clock::now() + RefreshInterval;
        }
    }

    inline void UpdateFrameData(TimePoint now) {
        if (!frame.localPawn) {
            frame.valid = false;
            frame.viewAnglesValid = false;
            return;
        }

        vec3 localPosition{};
        if (MEMORY::TryRead(frame.localPawn + m_vOldOrigin, localPosition)) {
            frame.localPosition = localPosition;
            frame.lastPositionUpdate = now;
        }

        Matrix4x4 viewMatrix{};
        if (MEMORY::TryRead(MEMORY::baseAddress + dwViewMatrix, viewMatrix)) {
            frame.viewMatrix = viewMatrix;
            frame.lastMatrixUpdate = now;
        }

        vec3 viewAngles{};
        if (MEMORY::TryRead(MEMORY::baseAddress + dwViewAngles, viewAngles) &&
            IsFinitePoint(viewAngles)) {
            frame.viewAngles = viewAngles;
            frame.lastViewAnglesUpdate = now;
        }
        frame.viewAnglesValid =
            IsFresh(frame.lastViewAnglesUpdate, now, FrameFreshness);

        // Never render an old camera matrix indefinitely. One failed RPM call
        // can be tolerated, but stale data is dropped instead of freezing ESP.
        frame.valid = IsFresh(frame.lastPositionUpdate, now, FrameFreshness) &&
                      IsFresh(frame.lastMatrixUpdate, now, FrameFreshness);
        if (!frame.valid && nextSlot > MaxSlots)
            nextRefresh = now;
    }

    inline void UpdateEntityLiveData(TimePoint now) {
        bool staleEntityDetected = false;
        for (auto& entity : entities) {
            if (!entity.pawn) {
                entity.originValid = false;
                continue;
            }

            vec3 origin{};
            if (MEMORY::TryRead(entity.pawn + m_vOldOrigin, origin)) {
                entity.origin = origin;
                entity.lastOriginUpdate = now;
                entity.originValid = true;
            } else if (!IsFresh(entity.lastOriginUpdate, now, OriginFreshness)) {
                entity.originValid = false;
                staleEntityDetected = true;
            }

            // Health changes are important for filtering and are inexpensive
            // compared with bone reads, so refresh it every frame.
            int health = 0;
            if (MEMORY::TryRead(entity.pawn + m_iHealth, health)) {
                entity.health = health;
                entity.lastHealthUpdate = now;
            }

            int armor = 0;
            if (MEMORY::TryRead(entity.pawn + m_ArmorValue, armor)) {
                entity.armor = std::clamp(armor, 0, 100);
            }

        }

        // If a pawn pointer became stale (respawn/map transition), start a
        // new incremental address scan immediately instead of waiting for the
        // normal refresh deadline.
        if (staleEntityDetected && nextSlot > MaxSlots)
            nextRefresh = now;
    }

    inline void UpdateBoneCache(TimePoint now) {
        if (!AppSettings::values.skeleton || entities.empty()) return;

        const std::size_t count = entities.size();
        const std::size_t attempts = (std::min)(
            count, static_cast<std::size_t>(BoneEntitiesPerFrame));

        for (std::size_t attempt = 0; attempt < attempts; ++attempt) {
            if (boneCursor >= count) boneCursor = 0;
            auto& entity = entities[boneCursor++];

            if (!entity.pawn || !entity.originValid ||
                entity.team == frame.localTeam ||
                entity.health <= 0 || entity.health > 100) {
                entity.debugBonesValid = false;
                entity.bonesValid = false;
                continue;
            }

            if (IsFresh(entity.lastBoneUpdate, now, BoneRefreshInterval))
                continue;

            if (!entity.boneArray && !ResolveBoneArray(entity)) {
                if (!IsFresh(entity.lastBoneUpdate, now, BoneFreshness)) {
                    entity.debugBonesValid = false;
                    entity.bonesValid = false;
                }
                continue;
            }

            DebugBoneArray freshBones{};
            if (ReadSkeletonBones(entity.boneArray, freshBones) &&
                ValidateSkeletonSnapshot(entity, freshBones)) {
                entity.debugBones = freshBones;
                entity.boneOrigin = entity.origin;
                ExtractSelectedBones(entity.debugBones, entity.bones);
                entity.lastBoneUpdate = now;
                entity.debugBonesValid = true;
                entity.bonesValid = true;
                entity.boneReadFailures = 0;
            } else {
                ++entity.boneReadFailures;
                // Bone arrays can move when a pawn/model changes. Drop only
                // the cached address so the next update re-resolves it.
                if (entity.boneReadFailures >= 2) {
                    entity.sceneNode = 0;
                    entity.boneArray = 0;
                    entity.boneReadFailures = 0;
                }

                if (!IsFresh(entity.lastBoneUpdate, now, BoneFreshness)) {
                    entity.debugBonesValid = false;
                    entity.bonesValid = false;
                }
            }
        }

        // Entities not selected by the per-frame budget can keep their most
        // recent bones for a very short grace period, then stop rendering them
        // rather than showing a frozen skeleton.
        for (auto& entity : entities) {
            if (entity.debugBonesValid &&
                !IsFresh(entity.lastBoneUpdate, now, BoneFreshness)) {
                entity.debugBonesValid = false;
                entity.bonesValid = false;
            }
        }
    }

    inline void UpdateLiveData() {
        const auto now = Clock::now();
        UpdateFrameData(now);
        UpdateEntityLiveData(now);
        UpdateBoneCache(now);
    }

    inline void Update() {
        const auto now = Clock::now();
        if (nextSlot > MaxSlots && now >= nextRefresh)
            BeginRefresh();

        ScanSlots();
        UpdateLiveData();
    }

    [[nodiscard]] inline const std::vector<CachedEntity>& GetEntities() { return entities; }
    [[nodiscard]] inline const FrameData& GetFrame() { return frame; }
}
