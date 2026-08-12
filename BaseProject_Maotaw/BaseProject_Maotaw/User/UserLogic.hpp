#pragma once

#include "Game/EntityCache.hpp"

namespace UserLogic {
    inline void OnUpdate() {
        // Addresses refresh incrementally; positions, health and the view
        // matrix remain current every frame.
        EntityCache::Update();

        // Add feature logic here. Cached data is available through:
        // EntityCache::GetFrame() and EntityCache::GetEntities().
    }
}
