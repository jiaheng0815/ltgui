#pragma once
#include "platform/platform.h"

#ifdef LTGUI_PLATFORM_WINDOWS

#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include "platform/native_window.h"

namespace ltgui {

class Win32Window : public NativeWindow {
public:
    Win32Window();
    ~Win32Window() override;

    bool create(int width, int height, const std::string& title) override;
    void destroy() override;
    void show() override;
    void hide() override;
    void close() override;
    void setTitle(const std::string& title) override;
    void setSize(int width, int height) override;
    Size getSize() const override;
    void invalidate(const Rect& rect) override;
    void* nativeHandle() const override;

    NativeCanvas* getCanvas() override;
    float dpiScale() const override { return dpiScale_; }
    void setCursor(CursorShape shape) override;
    bool setClipboardText(const std::string& text) override;
    std::string getClipboardText() override;

    // Set IME composition window position (client coordinates)
    void setImeCursorPos(int x, int y) { imeCompX_ = x; imeCompY_ = y; }

private:
    friend LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT handleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    static void registerClass();

    HWND hwnd_ = nullptr;
    NativeCanvas* canvas_ = nullptr;
    Size size_;
    float dpiScale_ = 1.0f;
    bool trackingMouse_ = false;
    int imeCompX_ = 10;
    int imeCompY_ = 10;
    static bool classRegistered_;
};

} // namespace ltgui

#endif // LTGUI_PLATFORM_WINDOWS
