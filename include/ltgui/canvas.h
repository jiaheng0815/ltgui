#pragma once
#include "platform/native_canvas.h"
#include "geometry.h"
#include <vector>

namespace ltgui {

// Canvas wraps NativeCanvas with coordinate transform and clipping support.
// Widgets still receive NativeCanvas* for drawing; Canvas is used as a
// compositing helper — create one on the stack, apply transforms, then
// forward adjusted draw calls to the native canvas.
class Canvas {
public:
    explicit Canvas(NativeCanvas* native) : native_(native) {
        pushDefault();
    }

    NativeCanvas* native() const { return native_; }

    // Coordinate transform stack
    void save() {
        stack_.push_back(stack_.back());
    }

    void restore() {
        if (stack_.size() > 1) {
            stack_.pop_back();
        }
    }

    void translate(int dx, int dy) {
        stack_.back().tx += dx;
        stack_.back().ty += dy;
    }

    int translateX() const { return stack_.back().tx; }
    int translateY() const { return stack_.back().ty; }

    // Clip region (in canvas-local coordinates, before translate)
    void setClipRect(const Rect& rect) {
        stack_.back().clip = rect;
        stack_.back().hasClip = true;
    }

    void clearClip() {
        stack_.back().hasClip = false;
    }

    Rect clipRect() const {
        auto& s = stack_.back();
        if (s.hasClip) return s.clip;
        return {};
    }

    bool hasClip() const { return stack_.back().hasClip; }

    // Apply current transform to a Rect (for forwarding to native canvas)
    Rect mapRect(const Rect& r) const {
        return r.translated(stack_.back().tx, stack_.back().ty);
    }

    Point mapPoint(const Point& p) const {
        return {p.x + stack_.back().tx, p.y + stack_.back().ty};
    }

    // Transform-aware drawing helpers
    void fillRect(const Rect& rect) {
        native_->fillRect(mapRect(rect));
    }

    void strokeRect(const Rect& rect, int lineWidth = 1) {
        native_->strokeRect(mapRect(rect), lineWidth);
    }

    void fillRoundedRect(const Rect& rect, int radius) {
        native_->fillRoundedRect(mapRect(rect), radius);
    }

    void strokeRoundedRect(const Rect& rect, int radius, int lineWidth = 1) {
        native_->strokeRoundedRect(mapRect(rect), radius, lineWidth);
    }

    void drawText(const std::string& text, const Rect& rect, int flags = 0) {
        native_->drawText(text, mapRect(rect), flags);
    }

private:
    struct State {
        int tx = 0;
        int ty = 0;
        Rect clip;
        bool hasClip = false;
    };

    void pushDefault() { stack_.push_back(State{}); }

    NativeCanvas* native_;
    std::vector<State> stack_;
};

} // namespace ltgui
