#pragma once
#include "platform/platform.h"

#ifdef LTGUI_PLATFORM_MACOS

#include "platform/native_window.h"
#include <string>

#ifdef __OBJC__
@class NSWindow;
@class CocoaWindowDelegate;
@class CocoaView;
#else
typedef struct objc_object NSWindow;
typedef struct objc_object CocoaWindowDelegate;
typedef struct objc_object CocoaView;
#endif

namespace ltgui {

class CocoaWindow : public NativeWindow {
public:
    CocoaWindow();
    ~CocoaWindow() override;

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

    // Clipboard
    bool setClipboardText(const std::string& text) override;
    std::string getClipboardText() override;

    NativeCanvas* getCanvas() override;

    // Called from Objective-C delegate
    void onPaint();
    void onResize(int width, int height);
    void onClose();
    void onMouseEvent(Event& ev);
    void onKeyEvent(Event& ev);

    // Map macOS keyCode to ltgui Key enum
    Key mapCocoaKey(int keyCode) const;

private:
    NSWindow* nsWindow_ = nullptr;
    CocoaWindowDelegate* delegate_ = nullptr;
    CocoaView* view_ = nullptr;
    NativeCanvas* canvas_ = nullptr;
    Size size_;
};

} // namespace ltgui

#endif // LTGUI_PLATFORM_MACOS
