#include "widget.h"
#include "window.h"
#include "layout.h"
#include "platform/native_canvas.h"
#include <algorithm>

namespace ltgui {

Widget::Widget(Widget* parent) : parent_(parent) {
    style_ = Style::defaultStyle();
}

Widget::~Widget() {
    // Clear focus before destruction — must happen while window_ is still valid
    if (window_ && window_->focusWidget_ == this) {
        window_->setFocusWidget(nullptr);
    }
    // layout_ and children_ destroyed automatically via unique_ptr
}

Widget* Widget::addChild(std::unique_ptr<Widget> child) {
    if (!child) return nullptr;
    if (child->parent_ && child->parent_ != this) {
        child->parent_->removeChild(child.get());
    }
    child->parent_ = this;
    Widget* raw = child.get();
    raw->propagateWindow(window_);
    raw->flags_ |= kFlagNeedsLayout;
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

    return raw;
}

std::unique_ptr<Widget> Widget::removeChild(Widget* child) {
    auto it = std::find_if(children_.begin(), children_.end(),
        [child](const auto& p) { return p.get() == child; });
    if (it != children_.end()) {
        (*it)->parent_ = nullptr;
        (*it)->propagateWindow(nullptr);
        auto result = std::move(*it);
        children_.erase(it);
        flags_ |= kFlagNeedsLayout;
        invalidateSizeHint();
        return result;
    }
    return nullptr;
}

Widget* Widget::childAt(int index) const {
    if (index >= 0 && index < static_cast<int>(children_.size())) {
        return children_[index].get();
    }
    return nullptr;
}

Rect Widget::absoluteRect() const {
    int ax = geometry_.x;
    int ay = geometry_.y;
    for (const Widget* p = parent_; p; p = p->parent_) {
        ax += p->geometry_.x;
        ay += p->geometry_.y;
    }
    return {ax, ay, geometry_.width, geometry_.height};
}

void Widget::setGeometry(const Rect& rect) {
    if (geometry_ != rect) {
        geometry_ = rect;
        if (layout_) {
            layout_->layout(this);
        }
    }
}

void Widget::scheduleRelayout() {
    // Walk up to the nearest ancestor that has a layout, and re-lay it out
    // so children get resized after content changes (e.g. setText).
    // Guard against re-entrancy: if any ancestor is already inside a layout
    // pass (detected by needsLayout_ being cleared mid-layout), bail out.
    Widget* ancestor = parent_;
    while (ancestor) {
        if (ancestor->layout() && !ancestor->geometry().isEmpty()) {
            ancestor->layout()->layout(ancestor);
            ancestor->flags_ &= ~kFlagNeedsLayout;
            // Continue walking up in case outer containers also need relayout
            ancestor = ancestor->parent();
            continue;
        }
        ancestor = ancestor->parent();
    }
    // Fallback: if no ancestor has a Layout, at minimum resize this widget
    // based on its new size hint so content changes like setText() are visible.
    // Without this, widgets in layout-less trees would stay at zero size.
    Size hint = sizeHint();
    if (!hint.isEmpty() && (geometry_.width != hint.width || geometry_.height != hint.height)) {
        Rect newGeo(geometry_.x, geometry_.y, hint.width, hint.height);
        setGeometry(newGeo);
    }
    flags_ &= ~kFlagNeedsLayout;
}

Size Widget::sizeHint() const {
    if (!(flags_ & kFlagSizeHintDirty)) return cachedSizeHint_;
    if (layout_) {
        cachedSizeHint_ = layout_->preferredSize(this);
    } else {
        float dpi = window_ ? window_->dpiScale() : 1.0f;
        cachedSizeHint_ = {static_cast<int>(100 * dpi), static_cast<int>(24 * dpi)};
    }
    flags_ &= ~kFlagSizeHintDirty;
    return cachedSizeHint_;
}

void Widget::setLayout(std::unique_ptr<Layout> layout) {
    layout_ = std::move(layout);
    flags_ |= kFlagNeedsLayout;
}

void Widget::setEnabled(bool enabled) {
    bool cur = (flags_ & kFlagEnabled) != 0;
    if (cur != enabled) {
        if (enabled) flags_ |= kFlagEnabled; else flags_ &= ~kFlagEnabled;
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
        if (visible) flags_ |= kFlagVisible; else flags_ &= ~kFlagVisible;
        // Clear focus if this widget is being hidden while focused
        if (!visible && window_ && window_->focusWidget() == this) {
            window_->setFocusWidget(nullptr);
        }
        update();
    }
}

void Widget::setWindow(Window* window) {
    propagateWindow(window);
}

void Widget::claimFocus() {
    if (window_) {
        window_->setFocusWidget(this);
    }
}

void Widget::raiseToTop() {
    if (!parent_) return;
    auto& siblings = parent_->children_;
    auto it = std::find_if(siblings.begin(), siblings.end(),
        [this](const auto& p) { return p.get() == this; });
    if (it != siblings.end() && it != siblings.end() - 1) {
        auto self = std::move(*it);
        siblings.erase(it);
        siblings.push_back(std::move(self));
    }
}

void Widget::propagateWindow(Window* window) {
    window_ = window;
    flags_ |= kFlagSizeHintDirty;  // canvas availability changed — recompute next time
    for (auto& child : children_) {
        child->propagateWindow(window);
    }
}

void Widget::paint(NativeCanvas* canvas, const Rect& dirtyRect) {
    if (!isVisible()) return;

    Rect abs = absoluteRect();
    if (!abs.intersects(dirtyRect)) return;

    // Only fill the dirty portion — the backbuffer retains the rest from the
    // previous frame. Painting the full widget rect would erase sibling widgets
    // that don't intersect the dirty region.
    canvas->setColor(style_.bgColor);
    canvas->fillRect(abs.intersected(dirtyRect));

    paintSelf(canvas);
    paintBorder(canvas);
    paintChildren(canvas, dirtyRect);
}

void Widget::paintSelf(NativeCanvas* /*canvas*/) {
}

void Widget::paintChildren(NativeCanvas* canvas, const Rect& dirtyRect) {
    for (auto& child : children_) {
        if (child->isVisible()) {
            child->paint(canvas, dirtyRect);
        }
    }
}

void Widget::paintBorder(NativeCanvas* canvas) {
    if (style_.borderWidth > 0) {
        canvas->setColor(style_.borderColor);
        Rect abs = absoluteRect();
        for (int i = 0; i < style_.borderWidth; i++) {
            Rect r = abs.adjusted(i, i, -i, -i);
            canvas->strokeRect(r);
        }
    }
}

void Widget::update() {
    if (window_) {
        window_->invalidate(absoluteRect());
    }
}

void Widget::update(const Rect& dirtyLocalRect) {
    if (window_) {
        Rect absDirty = absoluteRect();
        absDirty.x += dirtyLocalRect.x;
        absDirty.y += dirtyLocalRect.y;
        absDirty.width = dirtyLocalRect.width;
        absDirty.height = dirtyLocalRect.height;
        window_->invalidate(absDirty);
    }
}

Widget* Widget::nextFocusWidget() {
    // Depth-first pre-order traversal: try children first, then siblings.
    if (!children_.empty()) {
        for (auto& child : children_) {
            if (child->isEnabled() && child->isVisible()) {
                Widget* found = child->nextFocusWidget();
                if (found) return found;
            }
        }
    }
    // If this widget can accept focus, return it
    if (canAcceptFocus()) {
        return this;
    }
    return nullptr;
}

Widget* Widget::previousFocusWidget() {
    // Find the last focusable widget in the tree above/left of this one.
    if (!parent_) return nullptr;

    auto& siblings = parent_->children_;
    // Find our index
    int myIdx = -1;
    for (int i = 0; i < static_cast<int>(siblings.size()); i++) {
        if (siblings[i].get() == this) { myIdx = i; break; }
    }
    if (myIdx < 0) return nullptr;

    // Check siblings before us (in reverse order)
    for (int i = myIdx - 1; i >= 0; i--) {
        Widget* sib = siblings[i].get();
        if (sib->isEnabled() && sib->isVisible()) {
            Widget* last = sib->lastFocusableDescendant();
            if (last) return last;
        }
    }

    // Move up to parent
    if (parent_->isEnabled() && parent_->isVisible()) return parent_;
    return parent_->previousFocusWidget();
}

Widget* Widget::lastFocusableDescendant() {
    // Find the rightmost/deepest focusable widget in this subtree
    if (children_.empty()) {
        if (canAcceptFocus())
            return this;
        return nullptr;
    }
    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        Widget* child = it->get();
        if (child->isEnabled() && child->isVisible()) {
            Widget* found = child->lastFocusableDescendant();
            if (found) return found;
        }
    }
    if (canAcceptFocus())
        return this;
    return nullptr;
}

bool Widget::handleEvent(Event& event) {
    if (!isEnabled() || !isVisible()) return false;

    switch (event.type) {
    case EventType::MouseDown:
    case EventType::MouseWheel: {
        // Targeted dispatch — only the child under the cursor gets the event.
        Point localPos = {event.pos.x - geometry_.x, event.pos.y - geometry_.y};
        for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
            Widget* child = it->get();
            if (child->isVisible() && child->isEnabled()) {
                Rect childEff = child->effectiveGeometry().translated(
                    child->geometry_.x, child->geometry_.y);
                if (childEff.contains(localPos)) {
                    Point savedPos = event.pos;
                    event.pos = localPos;
                    bool handled = child->handleEvent(event);
                    event.pos = savedPos;
                    if (handled) {
                        event.accepted = true;
                        return true;
                    }
                }
            }
        }
        break;
    }
    case EventType::MouseUp:
    case EventType::MouseMove: {
        // Broadcast — every child gets the event so hover/pressed state
        // can be cleared when the cursor leaves the widget bounds.
        Point localPos = {event.pos.x - geometry_.x, event.pos.y - geometry_.y};
        bool handledAny = false;
        for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
            Widget* child = it->get();
            if (child->isVisible() && child->isEnabled()) {
                Point savedPos = event.pos;
                event.pos = localPos;
                if (child->handleEvent(event))
                    handledAny = true;
                event.pos = savedPos;
            }
        }
        if (handledAny) {
            event.accepted = true;
            return true;
        }
        break;
    }
    default:
        break;
    }

    return event.accepted;
}

Widget* Widget::hitTest(const Point& pos) {
    if (!isVisible()) return nullptr;

    // Use effectiveGeometry() so widgets with extended hit areas
    // (context menus, tooltips, shadows) properly receive events.
    Rect eff = effectiveGeometry();
    if (!eff.contains(pos)) return nullptr;

    Point local = {pos.x - geometry_.x, pos.y - geometry_.y};

    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        Widget* child = it->get();
        if (child->isVisible()) {
            Widget* hit = child->hitTest(local);
            if (hit) return hit;
        }
    }

    return this;
}

} // namespace ltgui
