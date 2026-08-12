#pragma once

#include "UI/MenuDraw.hpp"
#include "UI/MenuInput.hpp"
#include "UI/MenuState.hpp"

namespace PixelMenu {
    inline void Render() {
        Input::Update();
        if (!visible) return;

        Input::UpdateDragging();
        if (page == Page::Main) DrawMainPanel();
        else if (page == Page::Visuals) DrawVisualPanel(x);
        else DrawRadarPanel(x);
    }

    inline void Shutdown() {}
}
