#include "widgets/scrollarea.h"
#include "platform/native_canvas.h"
#include <algorithm>

namespace ltgui {

ScrollArea::ScrollArea(Widget* parent) : Widget(parent) {
    style().bgColor = Color::White;
    style().borderWidth = 1;
    style().borderColor = Color::Gray;
}

void ScrollArea::setWidget(Widget* widget) {
    if (contentWidget_) {
        removeChild(contentWidget_);
        delete contentWidget_;
    }
    contentWidget_ = widget;
    if (contentWidget_) {
        addChild(contentWidget_);
        Size hint = contentWidget_->sizeHint();
        contentWidth_ = hint.width;
        contentHeight_ = hint.height;
        contentWidget_->setGeometry(Rect(0, 0, hint.width, hint.height));
    }
    update();
}

void ScrollArea::updateScrollBars() {
    // Simple: content size is the widget's size hint
    if (contentWidget_) {
        Size hint = contentWidget_->sizeHint();
        contentWidth_ = hint.width;
        contentHeight_ = hint.height;
    }
}

Size ScrollArea::sizeHint() const {
    return {200, 200};
}

void ScrollArea::scrollTo(int x, int y) {
    scrollX_ = std::max(0, std::min(x, contentWidth_ - width()));
    scrollY_ = std::max(0, std::min(y, contentHeight_ - height()));
    if (contentWidget_) {
        contentWidget_->setGeometry(Rect(-scrollX_, -scrollY_, contentWidth_, contentHeight_));
    }
    update();
}

void ScrollArea::paintSelf(NativeCanvas* canvas) {
    Rect r = absoluteRect();

    // Background
    canvas->setColor(style().bgColor);
    canvas->fillRect(r);

    // Border
    if (style().borderWidth > 0) {
        canvas->setColor(style().borderColor);
        canvas->strokeRect(r, style().borderWidth);
    }

    // Scrollbar (if needed)
    if (contentHeight_ > height()) {
        int sbWidth = 12;
        int thumbH = std::max(20, height() * height() / contentHeight_);
        int thumbY = r.y + (scrollY_ * (height() - thumbH)) / (contentHeight_ - height());

        // Track
        canvas->setColor(Color::LightGray);
        canvas->fillRect(Rect(r.right() - sbWidth, r.y, sbWidth, r.height));

        // Thumb
        canvas->setColor(Color::Gray);
        canvas->fillRect(Rect(r.right() - sbWidth, thumbY, sbWidth, thumbH));
    }
}

bool ScrollArea::handleEvent(Event& event) {
    if (!isEnabled()) return false;

    int localX = event.pos.x - x();
    int localY = event.pos.y - y();

    if (event.type == EventType::MouseWheel) {
        scrollTo(scrollX_, scrollY_ - event.wheelDelta * 30);
        event.accepted = true;
        return true;
    }

    // Forward to content widget
    if (contentWidget_ && event.type == EventType::MouseMove) {
        Point localPos = {localX + scrollX_, localY + scrollY_};
        if (contentWidget_->geometry().contains(localPos)) {
            Event childEvent = event;
            childEvent.pos = localPos;
            return contentWidget_->handleEvent(childEvent);
        }
    }

    return Widget::handleEvent(event);
}

} // namespace ltgui
