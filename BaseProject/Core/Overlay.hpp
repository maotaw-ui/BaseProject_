#pragma once

#include <Windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <wrl/client.h>
#include <cstdint>
#include <string_view>

namespace Colors {
[[nodiscard]] D2D1_COLOR_F FromArgb(std::uint32_t value, float alpha = 1.0f);
}

class Renderer final {
public:
    [[nodiscard]] bool Initialize(HWND window);
    [[nodiscard]] bool Begin();
    void End();

    void Line(float x1, float y1, float x2, float y2,
              D2D1_COLOR_F color, float thickness = 1.0f);

    void Rectangle(float left, float top, float right, float bottom,
                   D2D1_COLOR_F color, float thickness = 1.0f);

    void FilledRectangle(float left, float top, float right, float bottom,
                         D2D1_COLOR_F color);

    void Text(float x, float y, std::wstring_view text,
              D2D1_COLOR_F color, float size = 11.0f);

    [[nodiscard]] float Width() const noexcept;
    [[nodiscard]] float Height() const noexcept;

private:
    [[nodiscard]] bool CreateTarget();
    [[nodiscard]] bool CreateTextResources();

    HWND window_{};

    Microsoft::WRL::ComPtr<ID2D1Factory> factory_;
    Microsoft::WRL::ComPtr<ID2D1HwndRenderTarget> target_;
    Microsoft::WRL::ComPtr<ID2D1SolidColorBrush> brush_;

    Microsoft::WRL::ComPtr<IDWriteFactory> writeFactory_;

    D2D1_SIZE_F size_{};
};

class OverlayWindow final {
public:
    ~OverlayWindow();

    [[nodiscard]] bool Create(HINSTANCE instance);
    [[nodiscard]] bool ProcessMessages() const;
    [[nodiscard]] HWND Handle() const noexcept;

private:
    static LRESULT CALLBACK WindowProc(
        HWND window,
        UINT message,
        WPARAM wParam,
        LPARAM lParam);

    HWND window_{};
    HINSTANCE instance_{};
    const wchar_t* className_{ L"BaseProjectOverlayWindow" };
};
