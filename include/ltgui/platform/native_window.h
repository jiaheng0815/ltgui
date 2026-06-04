#pragma once
#include "geometry.h"
#include "event.h"
#include <string>
#include <functional>

namespace ltgui {

class NativeCanvas;

enum class CursorShape {
    Arrow,
    IBeam,
    Wait,
    Crosshair,
    SizeWE,
    SizeNS,
    SizeAll,
    Hand,
    Denied,
};

class NativeWindow {
public:
    using EventCallback = std::function<void(Event&)>;

    virtual ~NativeWindow() = default;

    virtual bool create(int width, int height, const std::string& title) = 0;
    virtual void destroy() = 0;
    virtual void show() = 0;
    virtual void hide() = 0;
    virtual void close() = 0;
    virtual void setTitle(const std::string& title) = 0;
    virtual void setSize(int width, int height) = 0;
    virtual Size getSize() const = 0;
    virtual void invalidate(const Rect& rect) = 0;
    virtual void* nativeHandle() const = 0;
    virtual float dpiScale() const { return 1.0f; }
    virtual void setCursor(CursorShape) {} // default: no-op

    // Clipboard — platform-specific implementation.
    // Returns true on success, false if unavailable or unsupported.
    virtual bool setClipboardText(const std::string& /*text*/) { return false; }
    virtual std::string getClipboardText() { return {}; }

    virtual NativeCanvas* getCanvas() = 0;

    void setEventCallback(EventCallback cb) { eventCallback_ = std::move(cb); }

protected:
    EventCallback eventCallback_;
};

} // namespace ltgui
