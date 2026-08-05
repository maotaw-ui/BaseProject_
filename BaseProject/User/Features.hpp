#pragma once

class Renderer;
class EntityCache;

namespace Features {

// Universal feature entry point. Called once every frame.
// The cached entity list is looped only once in Features.cpp.
void Run(Renderer& renderer, const EntityCache& cache);

} // namespace Features
