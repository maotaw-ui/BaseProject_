#pragma once

#include <Windows.h>

#include "Core/Memory.hpp"
#include "Core/Overlay.hpp"
#include "Game/Game.hpp"

class Application final {
public:
    [[nodiscard]] int Run(HINSTANCE instance);

private:
    [[nodiscard]] bool Initialize(HINSTANCE instance);
    void RenderFrame();

    Memory memory_;
    OverlayWindow window_;
    Renderer renderer_;
    EntityCache entities_;
};
