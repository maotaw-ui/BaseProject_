#include "Features.hpp"
#include "Draw.hpp"

#include "../Core/Overlay.hpp"
#include "../Game/Game.hpp"

#include <algorithm>

namespace Features {

void Run(Renderer& renderer, const EntityCache& cache) {
    const LocalPlayer& local = cache.Local();
    if (!local.valid) return;

    // This is the only feature loop.
    // Add future frame logic above or below this loop.
    for (const GameEntity& entity : cache.Entities()) {
        if (!entity.pawn || entity.team == local.team ||
            entity.health <= 0 || entity.health > 100) continue;

        Vec3 head = entity.origin;
        head.z += 72.0f;

        Draw::Target target{};
        target.health = entity.health;
        target.armor = entity.armor;

        if (!Math::WorldToScreen(entity.origin, local.viewMatrix,
                                 renderer.Width(), renderer.Height(),
                                 target.feetX, target.feetY)) continue;

        if (!Math::WorldToScreen(head, local.viewMatrix,
                                 renderer.Width(), renderer.Height(),
                                 target.headX, target.headY)) continue;

        target.height = target.feetY - target.headY;
        if (target.height <= 4.0f ||
            target.height > renderer.Height() * 2.0f) continue;

        target.width = std::max(2.0f, target.height * 0.50f);
        target.left = target.headX - target.width * 0.5f;
        target.top = target.headY;

        // Add or remove rendered features here.
        Draw::Box(renderer, target, Colors::FromArgb(0xFFFFFFFF), Colors::FromArgb(0xFF000000), 1.0f);
        Draw::HealthBar(renderer, target, Colors::FromArgb(0xFF32E85A), Colors::FromArgb(0xD9000000), Colors::FromArgb(0xFFFFFFFF), 2.5f);
       // Draw::Snapline(renderer, target, Colors::FromArgb(0xFFFFFFFF), 1.0f);

        // Add future per-entity logic here.
    }
}

} // namespace Features
