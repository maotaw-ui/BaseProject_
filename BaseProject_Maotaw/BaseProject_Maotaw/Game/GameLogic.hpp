#pragma once

#include "Game/EntityCache.hpp"
#include "User/UserLogic.hpp"
#include "User/UserRender.hpp"
#include "UI/PixelMenu.hpp"

namespace GameLogic {
    inline void RenderFrame() {
        UserLogic::OnUpdate();
        UserRender::OnRender();
        PixelMenu::Render();
    }
}
