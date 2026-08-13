#pragma once
#include "geometry.h"
#include "platform/native_canvas.h"
#include <vector>

namespace ltgui {

// Canvas wraps NativeCanvas with coordinate transform and clipping support.
// Widgets still receive NativeCanvas* for drawing; Canvas is used as a
// compositing helper — create one on the stack, apply transforms, then
// forward adjusted draw calls to the native canvas.
class Canvas {
public:
  explicit Canvas(NativeCanvas *native) : native_(native) { pushDefault(); }

  NativeCanvas *native() const { return native_; }

  // Coordinate transform stack
  void save() { stack_.push_back(stack_.back()); }

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
  void setClipRect(const Rect &rect) {
    stack_.back().clip = rect;
    stack_.back().hasClip = true;
  }

  void clearClip() { stack_.back().hasClip = false; }

  Rect clipRect() const {
    auto &s = stack_.back();
    if (s.hasClip)
      return s.clip;
    return {};
  }

  bool hasClip() const { return stack_.back().hasClip; }

  // Apply current transform to a Rect (for forwarding to native canvas)
  Rect mapRect(const Rect &r) const {
    return r.translated(stack_.back().tx, stack_.back().ty);
  }

  Point mapPoint(const Point &p) const {
    return {p.x + stack_.back().tx, p.y + stack_.back().ty};
  }

  // Apply current transform and clip to a rect (for forwarding to native
  // canvas)
  Rect clippedRect(const Rect &r) const {
    auto result = mapRect(r);
    auto &s = stack_.back();
    if (s.hasClip) {
      auto clipNative = s.clip.translated(s.tx, s.ty);
      result = result.intersected(clipNative);
    }
    return result;
  }

  // Transform-aware drawing helpers (with clipping)
  void fillRect(const Rect &rect) {
    auto r = clippedRect(rect);
    if (!r.isEmpty())
      native_->fillRect(r);
  }

  void strokeRect(const Rect &rect, int lineWidth = 1) {
    auto r = clippedRect(rect);
    if (!r.isEmpty())
      native_->strokeRect(r, lineWidth);
  }

  void fillRoundedRect(const Rect &rect, int radius) {
    auto r = clippedRect(rect);
    if (!r.isEmpty())
      native_->fillRoundedRect(r, radius);
  }

  void strokeRoundedRect(const Rect &rect, int radius, int lineWidth = 1) {
    auto r = clippedRect(rect);
    if (!r.isEmpty())
      native_->strokeRoundedRect(r, radius, lineWidth);
  }

  void drawText(const std::string &text, const Rect &rect, int flags = 0) {
    auto r = clippedRect(rect);
    if (!r.isEmpty())
      native_->drawText(text, r, flags);
  }

private:
  struct State {
    int tx = 0;
    int ty = 0;
    Rect clip;
    bool hasClip = false;
  };

  void pushDefault() { stack_.push_back(State{}); }

  NativeCanvas *native_;
  std::vector<State> stack_;
};

} // namespace ltgui
