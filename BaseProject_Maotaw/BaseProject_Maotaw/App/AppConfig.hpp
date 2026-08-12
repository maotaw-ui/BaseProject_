#pragma once

#include <chrono>

namespace AppConfig {
    inline constexpr wchar_t ProcessName[] = L"cs2.exe";
    inline constexpr wchar_t ModuleName[] = L"client.dll";
    inline constexpr int TargetFps = 144;
    inline constexpr auto FrameTime =
        std::chrono::microseconds(1'000'000 / TargetFps);
}
