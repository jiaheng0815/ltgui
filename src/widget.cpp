#include "widget.h"
#include "window.h"
#include "layout.h"
#include "platform/native_canvas.h"

namespace ltgui {

Widget::Widget(Widget* parent) : parent_(parent) {
    style_ = Style::defaultStyle();
    if (parent_) {
        parent_->addChild(this);
    }
}

Widget::~Widget() {
    delete layout_;

    // Remove from parent
    if (parent_) {
        parent_->removeChild(this);
    }

    // Delete children (copy list since removeChild modifies it)
    auto kids = children_;
    for (auto* child : kids) {
        delete child;
    }
}

void Widget::addChild(Widget* child) {
    if (child && child->parent_ != this) {
        if (child->parent_) {
            child->parent_->removeChild(child);
        }
        child->parent_ = this;
        children_.push_back(child);
        child->propagateWindow(window_);
        child->needsLayout_ = true;
    }
}

void Widget::removeChild(Widget* child) {
    auto it = std::find(children_.begin(), children_.end(), child);
    if (it != children_.end()) {
        child->parent_ = nullptr;
        child->propagateWindow(nullptr);
        children_.erase(it);
        needsLayout_ = true;
    }
}

Widget* Widget::childAt(int index) const {
    if (index >= 0 && index < static_cast<int>(children_.size())) {
        return children_[index];
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
        // Re-layout children if we have a layout
        if (layout_) {
            layout_->layout(this);
        }
    }
}

Size Widget::sizeHint() const {
    if (layout_) {
        return layout_->preferredSize(const_cast<Widget*>(this));
    }
    return {100, 24}; // Default widget size
}

Size Widget::minimumSize() const {
    return {0, 0};
}

void Widget::setLayout(Layout* layout) {
    delete layout_;
    layout_ = layout;
    needsLayout_ = true;
}

void Widget::setEnabled(bool enabled) {
    if (enabled_ != enabled) {
        enabled_ = enabled;
        update();
    }
}

void Widget::setVisible(bool visible) {
    if (visible_ != visible) {
        visible_ = visible;
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

void Widget::propagateWindow(Window* window) {
    window_ = window;
    for (auto* child : children_) {
        child->propagateWindow(window);
    }
}

void Widget::paint(NativeCanvas* canvas) {
    if (!visible_) return;

    Rect abs = absoluteRect();
    canvas->setColor(style_.bgColor);
    canvas->fillRect(abs);

    paintSelf(canvas);
    paintBorder(canvas);
    paintChildren(canvas);
}

void Widget::paintSelf(NativeCanvas* /*canvas*/) {
    // Override in subclasses
}

void Widget::paintChildren(NativeCanvas* canvas) {
    for (auto* child : children_) {
        if (child->visible_) {
            child->paint(canvas);
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
        window_->update();
    }
}

bool Widget::handleEvent(Event& event) {
    if (!enabled_ || !visible_) return false;

    // Handle mouse events: hit test children first
    switch (event.type) {
    case EventType::MouseDown:
    case EventType::MouseUp:
    case EventType::MouseMove:
    case EventType::MouseWheel: {
        // Translate event position to local coordinates
        Point localPos = {event.pos.x - geometry_.x, event.pos.y - geometry_.y};

        // Try children in reverse z-order
        for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
            Widget* child = *it;
            if (child->visible_ && child->enabled_ &&
                child->effectiveGeometry().contains(localPos)) {
                Event childEvent = event;
                childEvent.pos = localPos;
                if (child->handleEvent(childEvent)) {
                    event.accepted = true;
                    return true;
                }
            }
        }
        break;
    }
    default:
        break;
    }

    return event.accepted;
}

Widget* Widget::hitTest(const Point& pos) {
    if (!visible_) return nullptr;

    Point local = {pos.x - geometry_.x, pos.y - geometry_.y};

    // Check children in reverse z-order
    for (auto it = children_.rbegin(); it != children_.rend(); ++it) {
        Widget* child = *it;
        if (child->geometry().contains(local)) {
            Widget* hit = child->hitTest(local);
            if (hit) return hit;
        }
    }

    if (geometry_.contains(pos)) {
        return this;
    }
    return nullptr;
}

} // namespace ltgui
