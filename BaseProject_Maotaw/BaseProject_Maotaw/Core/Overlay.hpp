#pragma once
#define WIN32_LEAN_AND_MEAN
#define NOMINMAX

#include <windows.h>
#include "Core/RendererCore.hpp"

namespace Overlay {
    inline constexpr wchar_t ClassName[] = L"BaseProject_MaotawOverlay";

    inline HWND hwnd = nullptr;
    inline HDC hdcMem = nullptr;
    inline HBITMAP bitmap = nullptr;
    inline HGDIOBJ oldBitmap = nullptr;
    inline HINSTANCE instance = nullptr;
    inline bool isRunning = true;

    inline LRESULT CALLBACK WndProc(HWND window, UINT message,
                                    WPARAM wParam, LPARAM lParam) {
        if (message == WM_DESTROY) {
            isRunning = false;
            PostQuitMessage(0);
            return 0;
        }
        return DefWindowProcW(window, message, wParam, lParam);
    }

    inline void Shutdown() {
        if (hdcMem && oldBitmap) SelectObject(hdcMem, oldBitmap);
        if (bitmap) DeleteObject(bitmap);
        if (hdcMem) DeleteDC(hdcMem);
        if (hwnd) DestroyWindow(hwnd);
        if (instance) UnregisterClassW(ClassName, instance);

        oldBitmap = nullptr;
        bitmap = nullptr;
        hdcMem = nullptr;
        hwnd = nullptr;
        instance = nullptr;
        RendererCore::bits = nullptr;
        RendererCore::width = 0;
        RendererCore::height = 0;
        RendererCore::previousDirty = {};
        RendererCore::currentDirty = {};
    }

    inline bool Initialize() {
        SetProcessDpiAwarenessContext(DPI_AWARENESS_CONTEXT_PER_MONITOR_AWARE_V2);

        isRunning = true;
        instance = GetModuleHandleW(nullptr);
        RendererCore::width = GetSystemMetrics(SM_CXSCREEN);
        RendererCore::height = GetSystemMetrics(SM_CYSCREEN);
        RendererCore::previousDirty = {};
        RendererCore::currentDirty = {};

        WNDCLASSEXW windowClass{};
        windowClass.cbSize = sizeof(windowClass);
        windowClass.lpfnWndProc = WndProc;
        windowClass.hInstance = instance;
        windowClass.lpszClassName = ClassName;
        windowClass.hCursor = LoadCursorW(nullptr, IDC_ARROW);

        if (!RegisterClassExW(&windowClass) && GetLastError() != ERROR_CLASS_ALREADY_EXISTS)
            return false;

        hwnd = CreateWindowExW(
            WS_EX_TOPMOST | WS_EX_TRANSPARENT | WS_EX_LAYERED | WS_EX_TOOLWINDOW |
            WS_EX_NOACTIVATE,
            ClassName, L"", WS_POPUP,
            0, 0, RendererCore::width, RendererCore::height,
            nullptr, nullptr, instance, nullptr);
        if (!hwnd) {
            Shutdown();
            return false;
        }

        BITMAPINFO bitmapInfo{};
        bitmapInfo.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
        bitmapInfo.bmiHeader.biWidth = RendererCore::width;
        bitmapInfo.bmiHeader.biHeight = -RendererCore::height;
        bitmapInfo.bmiHeader.biPlanes = 1;
        bitmapInfo.bmiHeader.biBitCount = 32;
        bitmapInfo.bmiHeader.biCompression = BI_RGB;

        HDC screenDc = GetDC(nullptr);
        if (!screenDc) {
            Shutdown();
            return false;
        }

        hdcMem = CreateCompatibleDC(screenDc);
        bitmap = CreateDIBSection(
            screenDc, &bitmapInfo, DIB_RGB_COLORS,
            reinterpret_cast<void**>(&RendererCore::bits), nullptr, 0);
        ReleaseDC(nullptr, screenDc);

        if (!hdcMem || !bitmap || !RendererCore::bits) {
            Shutdown();
            return false;
        }

        oldBitmap = SelectObject(hdcMem, bitmap);
        if (!oldBitmap || oldBitmap == HGDI_ERROR) {
            Shutdown();
            return false;
        }

        ShowWindow(hwnd, SW_SHOWNOACTIVATE);
        return true;
    }

    inline bool Present() {
        if (!hwnd || !hdcMem || !RendererCore::Ready()) return false;

        const auto dirty = RendererCore::PresentDirty();
        if (dirty.Empty()) return true;

        POINT destination{};
        POINT source{};
        SIZE size{ RendererCore::width, RendererCore::height };
        BLENDFUNCTION blend{ AC_SRC_OVER, 0, 255, AC_SRC_ALPHA };

        const long long dirtyArea =
            static_cast<long long>(dirty.right - dirty.left) *
            static_cast<long long>(dirty.bottom - dirty.top);
        const long long screenArea =
            static_cast<long long>(RendererCore::width) * RendererCore::height;

        bool ok = false;
        if (dirtyArea > 0 && dirtyArea * 100 < screenArea * 70) {
            RECT dirtyRect{ dirty.left, dirty.top, dirty.right, dirty.bottom };
            UPDATELAYEREDWINDOWINFO info{};
            info.cbSize = sizeof(info);
            info.pptDst = &destination;
            info.psize = &size;
            info.hdcSrc = hdcMem;
            info.pptSrc = &source;
            info.pblend = &blend;
            info.dwFlags = ULW_ALPHA;
            info.prcDirty = &dirtyRect;
            ok = UpdateLayeredWindowIndirect(hwnd, &info) != FALSE;
        }

        // Fallback for drivers/Windows configurations that reject a dirty
        // layered-window update, and for frames that already cover most of the
        // screen where a full atomic upload is just as efficient.
        if (!ok) {
            ok = UpdateLayeredWindow(
                hwnd, nullptr, &destination, &size,
                hdcMem, &source, 0, &blend, ULW_ALPHA) != FALSE;
        }

        if (ok) RendererCore::CommitFrame();
        return ok;
    }
}
