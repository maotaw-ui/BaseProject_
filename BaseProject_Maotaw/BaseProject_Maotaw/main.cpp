#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include <chrono>
#include <thread>

#include "App/AppConfig.hpp"
#include "App/Config.hpp"
#include "Core/Memory.hpp"
#include "Core/Overlay.hpp"
#include "Game/GameLogic.hpp"

int WINAPI wWinMain(HINSTANCE, HINSTANCE, PWSTR, int) {
    Config::Load();

    if (!Overlay::Initialize()) {
        MessageBoxW(nullptr, L"Failed to initialize overlay.",
                    L"BaseProject_Maotaw", MB_OK | MB_ICONERROR);
        return 1;
    }

    PixelMenu::ClampToScreen();

    if (!MEMORY::Attach(AppConfig::ProcessName, AppConfig::ModuleName)) {
        MessageBoxW(nullptr, L"Could not attach to the target process.",
                    L"BaseProject_Maotaw", MB_OK | MB_ICONERROR);
        Overlay::Shutdown();
        return 1;
    }

    MSG message{};
    auto nextFrame = std::chrono::steady_clock::now();

    while (Overlay::isRunning) {
        while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
            if (message.message == WM_QUIT) Overlay::isRunning = false;
            TranslateMessage(&message);
            DispatchMessageW(&message);
        }

        if (!Overlay::isRunning || (GetAsyncKeyState(VK_DELETE) & 0x8000))
            break;

        RendererCore::Clear();
        GameLogic::RenderFrame();
        Overlay::Present();
        Config::Update();

        nextFrame += AppConfig::FrameTime;
        std::this_thread::sleep_until(nextFrame);

        const auto now = std::chrono::steady_clock::now();
        if (now > nextFrame + AppConfig::FrameTime) nextFrame = now;
    }

    Config::Flush();
    PixelMenu::Shutdown();
    MEMORY::Detach();
    Overlay::Shutdown();
    return 0;
}
