#pragma once
#include "geometry.h"
#include "event.h"
#include "platform/native_window.h"
#include "platform/native_canvas.h"
#include "platform/gpu/gpu_canvas.h"
#include <string>
#include <memory>

namespace ltgui {

class Widget;

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

    void setCentralWidget(Widget* widget);
    Widget* centralWidget() const { return centralWidget_; }

    void update();
    void invalidate(const Rect& rect);
    void* nativeHandle() const;

    NativeCanvas* canvas() { return canvas_; }
    bool isGpuAccelerated() const { return useGpu_; }

protected:
    virtual void onPaint(NativeCanvas* canvas, const Rect& dirtyRect);

private:
    friend class Application;
    void handleEvent(Event& event);

    friend class Widget;
    void setFocusWidget(Widget* w);

    std::unique_ptr<NativeWindow> nativeWindow_;
    std::unique_ptr<gpu::GpuCanvas> gpuCanvas_;
    NativeCanvas* canvas_ = nullptr;
    Widget* centralWidget_ = nullptr;
    Widget* focusWidget_ = nullptr;
    Rect accumulatedDirty_;
    bool dirtyValid_ = false;
    bool useGpu_ = false;
};

} // namespace ltgui
