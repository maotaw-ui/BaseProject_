#include "Game.hpp"

void EntityCache::Initialize(
    const Memory& memory,
    std::uintptr_t client) {

    memory_ = &memory;
    client_ = client;
    entities_.reserve(MaxPlayers);
    RebuildCache();
}

void EntityCache::Update() {
    if (!memory_ || !client_) {
        return;
    }

    const auto now = std::chrono::steady_clock::now();
    if (now >= nextRebuild_) {
        RebuildCache();
    }

    RefreshLiveData();
}

const LocalPlayer& EntityCache::Local() const noexcept {
    return local_;
}

const std::vector<GameEntity>& EntityCache::Entities() const noexcept {
    return entities_;
}

void EntityCache::RebuildCache() {
    nextRebuild_ = std::chrono::steady_clock::now() + RebuildInterval;

    local_.pawn = memory_->Read<std::uintptr_t>(
        client_ + Offsets::LocalPlayerPawn);

    entityList_ = memory_->Read<std::uintptr_t>(
        client_ + Offsets::EntityList);

    entities_.clear();

    if (!local_.pawn || !entityList_) {
        local_.valid = false;
        return;
    }

    local_.team = memory_->Read<int>(
        local_.pawn + Offsets::Team);

    for (int index = 1; index <= MaxPlayers; ++index) {
        const auto entry = memory_->Read<std::uintptr_t>(
            entityList_ + 8 * ((index & 0x7FFF) >> 9) + 16);

        if (!entry) {
            continue;
        }

        const auto controller = memory_->Read<std::uintptr_t>(
            entry + 112 * (index & 0x1FF));

        if (!controller) {
            continue;
        }

        const auto handle = memory_->Read<std::uint32_t>(
            controller + Offsets::PawnHandle);

        const auto pawn = ResolvePawn(handle);
        if (!pawn || pawn == local_.pawn) {
            continue;
        }

        entities_.push_back({
            index,
            controller,
            pawn
        });
    }
}

void EntityCache::RefreshLiveData() {
    if (!local_.pawn) {
        local_.valid = false;
        return;
    }

    local_.team = memory_->Read<int>(
        local_.pawn + Offsets::Team);

    local_.valid = memory_->Read(
        client_ + Offsets::ViewMatrix,
        local_.viewMatrix);

    for (GameEntity& entity : entities_) {
        (void)memory_->Read(
            entity.pawn + Offsets::Team,
            entity.team);

        (void)memory_->Read(
            entity.pawn + Offsets::Health,
            entity.health);

        (void)memory_->Read(
            entity.pawn + Offsets::Armor,
            entity.armor);

        (void)memory_->Read(
            entity.pawn + Offsets::Origin,
            entity.origin);
    }
}

std::uintptr_t EntityCache::ResolvePawn(
    std::uint32_t handle) const {

    if (!handle || handle == 0xFFFFFFFFu) {
        return 0;
    }

    const auto entry = memory_->Read<std::uintptr_t>(
        entityList_ + 8 * ((handle & 0x7FFFu) >> 9) + 16);

    if (!entry) {
        return 0;
    }

    return memory_->Read<std::uintptr_t>(
        entry + 112 * (handle & 0x1FFu));
}
