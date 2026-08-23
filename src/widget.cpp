#include "widget.h"
#include "layout.h"
#include "platform/native_canvas.h"
#include "theme.h"
#include "window.h"
#include <algorithm>

namespace ltgui {

Widget::Widget(Widget *parent) : parent_(parent) {
  style_ = Style::defaultStyle();
  // Repaint when the theme changes so resolvedStyle() reflects the new
  // theme on the next paint pass. ScopedConnection keeps this safe if the
  // widget outlives or dies before the ThemeManager singleton.
  themeConn_ = ScopedConnection(&ThemeManager::instance().onThemeChanged,
                                [this](const Theme &) { update(); });
}

ResolvedStyle Widget::resolvedStyle() const {
  WidgetState state = WidgetState::Normal;
  if (!isEnabled()) {
    state = WidgetState::Disabled;
  } else if (pressed_) {
    state = WidgetState::Pressed;
  } else if (hovered_) {
    state = WidgetState::Hovered;
  } else if (hasFocus()) {
    state = WidgetState::Focused;
  }
  return style_.resolve(state, currentTheme());
}

Widget::~Widget() {
  // Clear focus before destruction — must happen while window_ is still valid
  if (window_ && window_->focusWidget_ == this) {
    window_->setFocusWidget(nullptr);
  }
  // layout_ and children_ destroyed automatically via unique_ptr
}

Widget *Widget::addChild(std::unique_ptr<Widget> child) {
  if (!child)
    return nullptr;
  if (child->parent_ && child->parent_ != this) {
    // Capture the returned unique_ptr to prevent the temporary from deleting
    // the child (which would make `child` a dangling pointer).
    child = child->parent_->removeChild(child.get());
    if (!child)
      return nullptr;
  }
  child->parent_ = this;
  Widget *raw = child.get();
  raw->propagateWindow(window_);
  children_.push_back(std::move(child));
  invalidateSizeHint();

  // If we have no layout, give this child a default size based on its
  // sizeHint, so widgets like Label aren't stuck at {0,0,0,0} rendering
  // nothing.  The caller can still override with setGeometry() later.
  if (!layout_ && raw->geometry().isEmpty()) {
    Size hint = raw->sizeHint();
    if (!hint.isEmpty())
      raw->setGeometry(Rect(0, 0, hint.width, hint.height));
  }

  // The widget tree changed — repaint so the new child appears even if no
  // geometry change triggered an invalidation.
  update();
  return raw;
}

std::unique_ptr<Widget> Widget::removeChild(Widget *child) {
  auto it = std::find_if(children_.begin(), children_.end(),
                         [child](const auto &p) { return p.get() == child; });
  if (it != children_.end()) {
    (*it)->parent_ = nullptr;
    (*it)->propagateWindow(nullptr);
    auto result = std::move(*it);
    children_.erase(it);
    invalidateSizeHint();
    // Re-run the layout immediately: the remaining children still occupy
    // the removed child's slot otherwise until the next explicit relayout.
    if (layout_)
      layout_->layout(this);
    // Repaint both the vacated area and the remaining children.
    update();
    return result;
  }
  return nullptr;
}

Widget *Widget::childAt(int index) const {
  if (index >= 0 && index < static_cast<int>(children_.size())) {
    return children_[index].get();
  }
  return nullptr;
}

Rect Widget::absoluteRect() const {
  int ax = geometry_.x;
  int ay = geometry_.y;
  for (const Widget *p = parent_; p; p = p->parent_) {
    ax += p->geometry_.x;
    ay += p->geometry_.y;
  }
  return {ax, ay, geometry_.width, geometry_.height};
}

void Widget::setGeometry(const Rect &rect) {
  if (geometry_ != rect) {
    // Invalidate the OLD absolute rect too — the area the widget is
    // moving away from still shows stale pixels. absoluteRect() must be
    // captured before geometry_ changes.
    Rect oldAbs = absoluteRect();
    geometry_ = rect;
    if (layout_) {
      layout_->layout(this);
    }
    if (window_) {
      window_->invalidate(oldAbs);
      window_->invalidate(absoluteRect());
    }
  }
}

void Widget::scheduleRelayout() {
  // Walk up to the nearest ancestor that has a layout, and re-lay it out
  // so children get resized after content changes (e.g. setText).
  bool foundLayout = false;
  Widget *ancestor = parent_;
  while (ancestor) {
    if (ancestor->layout() && !ancestor->geometry().isEmpty()) {
      ancestor->layout()->layout(ancestor);
      foundLayout = true;
      // Continue walking up in case outer containers also need relayout
      ancestor = ancestor->parent();
      continue;
    }
    ancestor = ancestor->parent();
  }
  // Fallback: only if no ancestor has a Layout, resize this widget based on
  // its new size hint so content changes like setText() are visible.
  // Without this, widgets in layout-less trees would stay at zero size.
  if (!foundLayout) {
    Size hint = sizeHint();
    if (!hint.isEmpty() &&
        (geometry_.width != hint.width || geometry_.height != hint.height)) {
      Rect newGeo(geometry_.x, geometry_.y, hint.width, hint.height);
      setGeometry(newGeo);
    }
  }
}

Size Widget::sizeHint() const {
  if (!sizeHintCache_.dirty)
    return sizeHintCache_.value;
  if (layout_) {
    sizeHintCache_.value = layout_->preferredSize(this);
  } else {
    sizeHintCache_.value = dpiScaleSize(100, 24);
  }
  sizeHintCache_.dirty = false;
  return sizeHintCache_.value;
}

Size Widget::dpiScaleSize(int w, int h) const {
  float dpi = window_ ? window_->dpiScale() : 1.0f;
  return {static_cast<int>(w * dpi), static_cast<int>(h * dpi)};
}

void Widget::setLayout(std::unique_ptr<Layout> layout) {
  layout_ = std::move(layout);
  // Size hints are computed by the layout engine — invalidate the cached
  // value so the next sizeHint()/getGeometry pass reflects the new layout.
  invalidateSizeHint();
}

void Widget::setEnabled(bool enabled) {
  bool cur = (flags_ & kFlagEnabled) != 0;
  if (cur != enabled) {
    if (enabled)
      flags_ |= kFlagEnabled;
    else
      flags_ &= ~kFlagEnabled;
    // Clear focus if this widget is being disabled while focused
    if (!enabled && window_ && window_->focusWidget() == this) {
      window_->setFocusWidget(nullptr);
    }
    update();
  }
}

void Widget::setVisible(bool visible) {
  bool cur = (flags_ & kFlagVisible) != 0;
  if (cur != visible) {
    if (visible)
      flags_ |= kFlagVisible;
    else
      flags_ &= ~kFlagVisible;
    // Clear focus if this widget is being hidden while focused
    if (!visible && window_ && window_->focusWidget() == this) {
      window_->setFocusWidget(nullptr);
    }
    update();
  }
}

void Widget::setWindow(Window *window) { propagateWindow(window); }

Window *Widget::findWindow() const {
  if (window_)
    return window_;
  // Detached from a plain tree — walk the parent chain so a freshly cut
  // widget can still find the window of its former ancestors.
  for (const Widget *p = parent_; p; p = p->parent_) {
    if (p->window_)
      return p->window_;
  }
  return nullptr;
}

void Widget::claimFocus() {
  if (window_) {
    window_->setFocusWidget(this);
  }
}

void Widget::raiseToTop() {
  if (!parent_)
    return;
  auto &siblings = parent_->children_;
  auto it = std::find_if(siblings.begin(), siblings.end(),
                         [this](const auto &p) { return p.get() == this; });
  if (it != siblings.end() && it != siblings.end() - 1) {
    auto self = std::move(*it);
    siblings.erase(it);
    siblings.push_back(std::move(self));
    // Z-order changed — repaint to reflect the new ordering
    update();
  }
}

void Widget::restoreChildOrder(int index) {
  if (index < 0 || !parent_)
    return;
  auto &siblings = parent_->children_;
  int cur = -1;
  for (int i = 0; i < static_cast<int>(siblings.size()); i++) {
    if (siblings[i].get() == this) {
      cur = i;
      break;
    }
  }
  if (cur < 0 || cur == index)
    return;

  auto self = std::move(siblings[cur]);
  siblings.erase(siblings.begin() + cur);
  // Clamp the target position: after the erase the valid insert range is
  // [0, size] (size == append). Without the clamp an out-of-range index
  // would be UB.
  index = std::min(index, static_cast<int>(siblings.size()));
  siblings.insert(siblings.begin() + index, std::move(self));

  // The child order changed — re-lay out the parent so this widget and
  // its siblings return to their original slots.
  if (parent_->layout_)
    parent_->layout_->layout(parent_);
  update();
}

void Widget::propagateWindow(Window *window) {
  // Detaching a widget that still holds the window's focus would leave a
  // dangling focusWidget_ behind once this widget leaves the tree (or is
  // destroyed). Clear the focus first — the widget is still alive here, so
  // the FocusOut notification is safe. (Window::~Window nulls focusWidget_
  // before tearing down its tree, so that path doesn't re-enter.)
  if (window == nullptr && window_ != nullptr && window_->focusWidget_ == this) {
    window_->setFocusWidget(nullptr);
  }
  window_ = window;
  sizeHintCache_.dirty =
      true; // canvas availability changed — recompute next time
  for (auto &child : children_) {
    child->propagateWindow(window);
  }
}

void Widget::paint(NativeCanvas *canvas, const Rect &dirtyRect) {
  if (!isVisible())
    return;

  Rect abs = absoluteRect();
  if (!abs.intersects(dirtyRect))
    return;

  // Only fill the dirty portion — the backbuffer retains the rest from the
  // previous frame. Painting the full widget rect would erase sibling widgets
  // that don't intersect the dirty region.
  ResolvedStyle st = resolvedStyle();
  if (st.gradient) {
    // Gradient interpolates across the whole widget rect (abs) even when
    // only a dirty sub-rect is painted, so partial repaints keep the
    // exact same color ramp.
    canvas->fillLinearGradient(abs.intersected(dirtyRect), st.gradient->from,
                               st.gradient->to, st.gradient->vertical, abs);
  } else {
    canvas->setColor(st.bgColor);
    canvas->fillRect(abs.intersected(dirtyRect));
  }

  paintSelf(canvas);
  paintBorder(canvas);
  paintChildren(canvas, dirtyRect);
}

void Widget::paintSelf(NativeCanvas * /*canvas*/) {}

void Widget::paintChildren(NativeCanvas *canvas, const Rect &dirtyRect) {
  for (auto &child : children_) {
    if (child->isVisible()) {
      child->paint(canvas, dirtyRect);
    }
  }
}

void Widget::paintBorder(NativeCanvas *canvas) {
  if (style_.borderWidth > 0) {
    canvas->setColor(style_.borderColor);
    Rect abs = absoluteRect();
    for (int i = 0; i < style_.borderWidth; i++) {
      Rect r = abs.adjusted(i, i, -i, -i);
      canvas->strokeRect(r);
    }
  }
}

void Widget::paintBackground(NativeCanvas *canvas) {
  Rect r = absoluteRect();
  ResolvedStyle st = resolvedStyle();
  if (st.gradient) {
    canvas->fillLinearGradient(r, st.gradient->from, st.gradient->to,
                               st.gradient->vertical, r);
  } else {
    canvas->setColor(st.bgColor);
    if (st.borderRadius > 0) {
      canvas->fillRoundedRect(r, st.borderRadius);
    } else {
      canvas->fillRect(r);
    }
  }
  if (st.borderWidth > 0) {
    canvas->setColor(st.borderColor);
    int bw = st.borderWidth;
    int br = st.borderRadius;
    if (br > 0) {
      canvas->strokeRoundedRect(r.adjusted(0, 0, -1, -1), br, bw);
    } else {
      for (int i = 0; i < bw; i++) {
        canvas->strokeRect(r.adjusted(i, i, -i, -i));
      }
    }
  }
}

void Widget::update() {
  if (window_) {
    window_->invalidate(absoluteRect());
  }
}

void Widget::update(const Rect &dirtyLocalRect) {
  if (window_) {
    // Clip the dirty rect to the widget's local bounds so invalidation
    // doesn't spill into sibling/parent geometry.
    Rect localBounds(0, 0, geometry_.width, geometry_.height);
    Rect clipped = localBounds.intersected(dirtyLocalRect);
    if (clipped.isEmpty())
      return;

    Rect absDirty = absoluteRect();
    absDirty.x += clipped.x;
    absDirty.y += clipped.y;
    absDirty.width = clipped.width;
    absDirty.height = clipped.height;
    window_->invalidate(absDirty);
  }
}

Widget *Widget::nextFocusWidget() {
  // Depth-first pre-order traversal: try children first, then siblings.
  if (!children_.empty()) {
    for (auto &child : children_) {
      if (child->isEnabled() && child->isVisible()) {
        Widget *found = child->nextFocusWidget();
        if (found)
          return found;
      }
    }
  }
  // If this widget can accept focus, return it
  if (canAcceptFocus()) {
    return this;
  }
  return nullptr;
}

Widget *Widget::previousFocusWidget() {
  // Find the last focusable widget in the tree above/left of this one.
  if (!parent_)
    return nullptr;

  auto &siblings = parent_->children_;
  // Find our index
  int myIdx = -1;
  for (int i = 0; i < static_cast<int>(siblings.size()); i++) {
    if (siblings[i].get() == this) {
      myIdx = i;
      break;
    }
  }
  if (myIdx < 0)
    return nullptr;

  // Check siblings before us (in reverse order)
  for (int i = myIdx - 1; i >= 0; i--) {
    Widget *sib = siblings[i].get();
    if (sib->isEnabled() && sib->isVisible()) {
      Widget *last = sib->lastFocusableDescendant();
      if (last)
        return last;
    }
  }

  // Move up to parent — only if the parent itself can accept focus;
  // otherwise keep walking so a non-interactive container (e.g. a plain
  // panel) isn't handed the focus.
  if (parent_->canAcceptFocus() && parent_->isEnabled() &&
      parent_->isVisible()) {
    return parent_;
  }
  return parent_->previousFocusWidget();
}

Widget *Widget::lastFocusableDescendant() {
  // Find the rightmost/deepest focusable widget in this subtree
  if (children_.empty()) {
    if (canAcceptFocus())
      return this;
    return nullptr;
  }
  for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
    Widget *child = it->get();
    if (child->isEnabled() && child->isVisible()) {
      Widget *found = child->lastFocusableDescendant();
      if (found)
        return found;
    }
  }
  if (canAcceptFocus())
    return this;
  return nullptr;
}

bool Widget::dispatchToChildren(Event &event, bool targeted) {
  // Snapshot raw pointers — a child handler may mutate children_ via
  // addChild/removeChild, invalidating iterators into the live vector.
  // A local copy instead of a shared member: a modal dialog pumping its
  // own event loop can re-enter dispatchToChildren recursively, and a
  // single member buffer would be clobbered mid-iteration.
  std::vector<Widget *> snapshot;
  snapshot.reserve(children_.size());
  for (auto &c : children_)
    snapshot.push_back(c.get());

  Point localPos = {event.pos.x - geometry_.x, event.pos.y - geometry_.y};
  bool handledAny = false;

  for (auto it = snapshot.rbegin(); it != snapshot.rend(); ++it) {
    Widget *child = *it;
    // Verify child still in tree (may have been removed by prior handler)
    if (std::none_of(children_.begin(), children_.end(),
                     [child](const auto &ptr) { return ptr.get() == child; }))
      continue;
    if (!child->isVisible() || !child->isEnabled())
      continue;

    if (targeted) {
      // Only the child under cursor gets the event (MouseDown, MouseWheel)
      Rect childEff = child->effectiveGeometry().translated(child->geometry_.x,
                                                            child->geometry_.y);
      if (!childEff.contains(localPos))
        continue;
    }

    Point savedPos = event.pos;
    event.pos = localPos;
    bool handled = child->handleEvent(event);
    event.pos = savedPos;

    if (targeted && handled) {
      event.accepted = true;
      return true;
    }
    if (handled)
      handledAny = true;
  }

  if (!targeted && handledAny) {
    event.accepted = true;
    return true;
  }
  return false;
}

bool Widget::handleEvent(Event &event) {
  if (!isEnabled() || !isVisible())
    return false;

  switch (event.type) {
  case EventType::MouseDown:
  case EventType::MouseWheel:
    return dispatchToChildren(event, /*targeted=*/true);
  case EventType::MouseUp:
  case EventType::MouseMove:
    return dispatchToChildren(event, /*targeted=*/false);
  default:
    break;
  }

  return event.accepted;
}

Widget *Widget::hitTest(const Point &pos) {
  if (!isVisible())
    return nullptr;

  // pos is in parent space — convert to this widget's local coordinates
  // before comparing against effectiveGeometry() (which is local-origin).
  Point local = {pos.x - geometry_.x, pos.y - geometry_.y};

  // Use effectiveGeometry() so widgets with extended hit areas
  // (context menus, tooltips, shadows) properly receive events.
  Rect eff = effectiveGeometry();
  if (!eff.contains(local))
    return nullptr;

  for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
    Widget *child = it->get();
    if (child->isVisible()) {
      Widget *hit = child->hitTest(local);
      if (hit)
        return hit;
    }
  }

  return this;
}

} // namespace ltgui
