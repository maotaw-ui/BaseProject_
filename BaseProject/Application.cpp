#include "Application.hpp"

#include "User/Features.hpp"
#include "Game/Game.hpp"

#include <chrono>
#include <thread>

int Application::Run(HINSTANCE instance) {
    if (!Initialize(instance)) {
        return 1;
    }

    while (window_.ProcessMessages()) {
        if (GetAsyncKeyState(VK_DELETE) & 1) {
            break;
        }

        RenderFrame();
        std::this_thread::sleep_for(std::chrono::milliseconds(1));
    }

    return 0;
}

bool Application::Initialize(HINSTANCE instance) {
    SetProcessDpiAwarenessContext(
        DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

    if (!memory_.Attach(Target::Process)) {
        MessageBoxW(
            nullptr,
            L"Start the target process first.",
            L"BaseProject",
            MB_OK | MB_ICONERROR);
        return false;
    }

    const std::uintptr_t client = memory_.Module(Target::Module);
    if (!client) {
        MessageBoxW(
            nullptr,
            L"Target module was not found.",
            L"BaseProject",
            MB_OK | MB_ICONERROR);
        return false;
    }

    entities_.Initialize(memory_, client);

    return window_.Create(instance) &&
           renderer_.Initialize(window_.Handle());
}

void Application::RenderFrame() {
    entities_.Update();

    if (!renderer_.Begin()) {
        return;
    }

    Features::Run(renderer_, entities_);
    renderer_.End();
}
