#include "Overlay.hpp"

#include <dwmapi.h>
#include <algorithm>
#include <limits>

#pragma comment(lib, "D2d1.lib")
#pragma comment(lib, "Dwrite.lib")
#pragma comment(lib, "Dwmapi.lib")

D2D1_COLOR_F Colors::FromArgb(std::uint32_t value, float alpha) {
    constexpr float scale = 1.0f / 255.0f;

    return D2D1::ColorF(
        static_cast<float>((value >> 16) & 0xFF) * scale,
        static_cast<float>((value >> 8) & 0xFF) * scale,
        static_cast<float>(value & 0xFF) * scale,
        static_cast<float>((value >> 24) & 0xFF) * scale * alpha);
}

bool Renderer::Initialize(HWND window) {
    window_ = window;

    const auto options = D2D1_FACTORY_OPTIONS{};

    if (FAILED(D2D1CreateFactory(
            D2D1_FACTORY_TYPE_SINGLE_THREADED,
            __uuidof(ID2D1Factory),
            &options,
            reinterpret_cast<void**>(factory_.GetAddressOf())))) {
        return false;
    }

    return CreateTarget() && CreateTextResources();
}

bool Renderer::CreateTarget() {
    RECT area{};
    GetClientRect(window_, &area);

    const auto targetProperties = D2D1::RenderTargetProperties(
        D2D1_RENDER_TARGET_TYPE_HARDWARE,
        D2D1::PixelFormat(
            DXGI_FORMAT_B8G8R8A8_UNORM,
            D2D1_ALPHA_MODE_PREMULTIPLIED));

    const auto windowProperties = D2D1::HwndRenderTargetProperties(
        window_,
        D2D1::SizeU(area.right, area.bottom),
        D2D1_PRESENT_OPTIONS_IMMEDIATELY);

    if (FAILED(factory_->CreateHwndRenderTarget(
            targetProperties,
            windowProperties,
            target_.GetAddressOf()))) {
        return false;
    }

    size_ = target_->GetSize();
    target_->SetAntialiasMode(D2D1_ANTIALIAS_MODE_ALIASED);

    return SUCCEEDED(target_->CreateSolidColorBrush(
        D2D1::ColorF(1.0f, 1.0f, 1.0f, 1.0f),
        brush_.GetAddressOf()));
}


bool Renderer::CreateTextResources() {
    return SUCCEEDED(DWriteCreateFactory(
        DWRITE_FACTORY_TYPE_SHARED,
        __uuidof(IDWriteFactory),
        reinterpret_cast<IUnknown**>(writeFactory_.GetAddressOf())));
}

bool Renderer::Begin() {
    if (!target_ && !CreateTarget()) {
        return false;
    }

    target_->BeginDraw();
    target_->SetTransform(D2D1::Matrix3x2F::Identity());
    target_->Clear(D2D1::ColorF(0.0f, 0.0f, 0.0f, 0.0f));
    return true;
}

void Renderer::End() {
    if (target_->EndDraw() == D2DERR_RECREATE_TARGET) {
        brush_.Reset();
        target_.Reset();
    }
}

void Renderer::Line(
    float x1,
    float y1,
    float x2,
    float y2,
    D2D1_COLOR_F color,
    float thickness) {

    brush_->SetColor(color);
    target_->DrawLine(
        D2D1::Point2F(x1, y1),
        D2D1::Point2F(x2, y2),
        brush_.Get(),
        thickness);
}

void Renderer::Rectangle(
    float left,
    float top,
    float right,
    float bottom,
    D2D1_COLOR_F color,
    float thickness) {

    brush_->SetColor(color);
    target_->DrawRectangle(
        D2D1::RectF(left, top, right, bottom),
        brush_.Get(),
        thickness);
}

void Renderer::FilledRectangle(
    float left,
    float top,
    float right,
    float bottom,
    D2D1_COLOR_F color) {

    brush_->SetColor(color);
    target_->FillRectangle(
        D2D1::RectF(left, top, right, bottom),
        brush_.Get());
}


void Renderer::Text(float x, float y, std::wstring_view text,
                    D2D1_COLOR_F color, float size) {
    if (!target_ || !brush_ || !writeFactory_ || text.empty()) {
        return;
    }

    Microsoft::WRL::ComPtr<IDWriteTextFormat> format;

    if (FAILED(writeFactory_->CreateTextFormat(
            L"Segoe UI",
            nullptr,
            DWRITE_FONT_WEIGHT_SEMI_BOLD,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            size,
            L"",
            format.GetAddressOf()))) {
        return;
    }

    format->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
    format->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    format->SetWordWrapping(DWRITE_WORD_WRAPPING_NO_WRAP);

    brush_->SetColor(color);

    const auto length = static_cast<UINT32>(
        std::min<std::size_t>(text.size(), std::numeric_limits<UINT32>::max()));

    target_->DrawTextW(
        text.data(),
        length,
        format.Get(),
        D2D1::RectF(x, y, x + 200.0f, y + size + 8.0f),
        brush_.Get(),
        D2D1_DRAW_TEXT_OPTIONS_NO_SNAP,
        DWRITE_MEASURING_MODE_NATURAL);
}

float Renderer::Width() const noexcept {
    return size_.width;
}

float Renderer::Height() const noexcept {
    return size_.height;
}

OverlayWindow::~OverlayWindow() {
    if (window_) {
        DestroyWindow(window_);
    }

    if (instance_) {
        UnregisterClassW(className_, instance_);
    }
}

bool OverlayWindow::Create(HINSTANCE instance) {
    instance_ = instance;

    WNDCLASSEXW windowClass{};
    windowClass.cbSize = sizeof(windowClass);
    windowClass.hInstance = instance_;
    windowClass.lpfnWndProc = WindowProc;
    windowClass.lpszClassName = className_;
    windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);

    if (!RegisterClassExW(&windowClass)) {
        return false;
    }

    const int x = GetSystemMetrics(SM_XVIRTUALSCREEN);
    const int y = GetSystemMetrics(SM_YVIRTUALSCREEN);
    const int width = GetSystemMetrics(SM_CXVIRTUALSCREEN);
    const int height = GetSystemMetrics(SM_CYVIRTUALSCREEN);

    window_ = CreateWindowExW(
        WS_EX_TOPMOST |
            WS_EX_LAYERED |
            WS_EX_TRANSPARENT |
            WS_EX_TOOLWINDOW |
            WS_EX_NOACTIVATE,
        className_,
        L"BaseProject Overlay",
        WS_POPUP,
        x,
        y,
        width,
        height,
        nullptr,
        nullptr,
        instance_,
        nullptr);

    if (!window_) {
        return false;
    }

    SetLayeredWindowAttributes(window_, 0, 255, LWA_ALPHA);

    const MARGINS margins{ -1 };
    DwmExtendFrameIntoClientArea(window_, &margins);

    ShowWindow(window_, SW_SHOWNA);
    UpdateWindow(window_);
    return true;
}

bool OverlayWindow::ProcessMessages() const {
    MSG message{};

    while (PeekMessageW(&message, nullptr, 0, 0, PM_REMOVE)) {
        if (message.message == WM_QUIT) {
            return false;
        }

        TranslateMessage(&message);
        DispatchMessageW(&message);
    }

    return true;
}

HWND OverlayWindow::Handle() const noexcept {
    return window_;
}

LRESULT CALLBACK OverlayWindow::WindowProc(
    HWND window,
    UINT message,
    WPARAM wParam,
    LPARAM lParam) {

    if (message == WM_DESTROY) {
        PostQuitMessage(0);
        return 0;
    }

    if (message == WM_NCHITTEST) {
        return HTTRANSPARENT;
    }

    if (message == WM_ERASEBKGND) {
        return 1;
    }

    return DefWindowProcW(window, message, wParam, lParam);
}
