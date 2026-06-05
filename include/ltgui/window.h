#pragma once
#include "geometry.h"
#include "event.h"
#include "shortcut.h"
#include "platform/native_window.h"
#include "platform/native_canvas.h"
#include "platform/gpu/gpu_canvas.h"
#include <string>
#include <memory>
#include <vector>

namespace ltgui {

class Widget;
class ComboBox;

class Window {
public:
    Window();
    virtual ~Window();

    bool create(int width = 800, int height = 600, const std::string& title = "ltgui");
    void close();
    void show();
    void hide();

    void setTitle(const std::string& title);
    void setSize(int width, int height);
    Size getSize() const;

    void setCentralWidget(std::unique_ptr<Widget> widget);
    Widget* centralWidget() const { return centralWidget_.get(); }

    void update();
    void invalidate(const Rect& rect);
    void* nativeHandle() const;

    NativeCanvas* canvas() { return canvas_; }
    NativeWindow* nativeWindow() { return nativeWindow_.get(); }
    bool isGpuAccelerated() const { return useGpu_; }

    // DPI scale factor for this window
    float dpiScale() const {
        if (nativeWindow_) return nativeWindow_->dpiScale();
        return 1.0f;
    }

    void setCursor(CursorShape shape) {
        if (nativeWindow_) nativeWindow_->setCursor(shape);
    }

    // Focus management
    Widget* focusWidget() const { return focusWidget_; }

    // Keyboard shortcuts: registered shortcuts are checked before widget
    // event dispatch. If a shortcut matches, its callback fires and the
    // key event is consumed.
    void registerShortcut(const Shortcut& sc, Shortcut::Callback cb);
    void unregisterShortcut(const Shortcut& sc);

protected:
    virtual void onPaint(NativeCanvas* canvas, const Rect& dirtyRect);

private:
    friend class Application;
    void handleEvent(Event& event);

    friend class Widget;
    friend class ComboBox;
    void setFocusWidget(Widget* w);

    // Guard against dangling focusWidget_: if the focus widget is no longer in
    // this window's tree (e.g. it was destroyed or reparented), clear and
    // return false. Callers should bail out on false.
    bool validateFocusWidget();

    // IME composition cursor position in screen coordinates.
    // The focused widget reports its cursor position so the platform
    // can position the IME composition window correctly.
    Point imeCursorScreenPos() const;

    std::unique_ptr<NativeWindow> nativeWindow_;
    std::unique_ptr<gpu::GpuCanvas> gpuCanvas_;
    NativeCanvas* canvas_ = nullptr;
    std::unique_ptr<Widget> centralWidget_;
    Widget* focusWidget_ = nullptr;
    ComboBox* openCombo_ = nullptr;
    Rect accumulatedDirty_;
    bool dirtyValid_ = false;
    bool useGpu_ = false;

    struct ShortcutEntry {
        Shortcut shortcut;
        Shortcut::Callback callback;
    };
    std::vector<ShortcutEntry> shortcuts_;
};

} // namespace ltgui
