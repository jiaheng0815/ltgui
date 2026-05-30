#pragma once
#include "platform/platform.h"

#ifdef LTGUI_PLATFORM_WINDOWS

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

private:
    friend LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);
    LRESULT handleMessage(UINT msg, WPARAM wParam, LPARAM lParam);

    static void registerClass();

    HWND hwnd_ = nullptr;
    NativeCanvas* canvas_ = nullptr;
    Size size_;
    bool trackingMouse_ = false;
    static bool classRegistered_;
};

} // namespace ltgui

#endif // LTGUI_PLATFORM_WINDOWS
