#include "widgets/scrollarea.h"
#include "theme.h"
#include "platform/native_canvas.h"
#include <algorithm>

namespace ltgui {

ScrollArea::ScrollArea(Widget* parent) : Widget(parent) {
    style().bgColor = currentTheme().bgSecondary;
    style().borderWidth = 1;
    style().borderColor = currentTheme().border;
    style().borderRadius = 4;
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
    if (contentWidget_) {
        Size hint = contentWidget_->sizeHint();
        contentWidth_ = hint.width;
        contentHeight_ = hint.height;
    }
}

Size ScrollArea::sizeHint() const {
    if (!sizeHintDirty()) return cachedSizeHint();
    setCachedSizeHint({200, 200});
    return cachedSizeHint();
}

int ScrollArea::currentScrollY() {
    return static_cast<int>(scrollYAnim_.value());
}

void ScrollArea::scrollTo(int x, int y) {
    scrollX_ = std::max(0, std::min(x, std::max(0, contentWidth_ - width())));
    scrollY_ = std::max(0, std::min(y, std::max(0, contentHeight_ - height())));
    scrollYAnim_.setTarget(static_cast<float>(scrollY_), 200, Easing::EaseOut);
    if (contentWidget_) {
        contentWidget_->setGeometry(Rect(-scrollX_, -currentScrollY(), contentWidth_, contentHeight_));
    }
    update();
}

void ScrollArea::paintSelf(NativeCanvas* canvas) {
    Rect r = absoluteRect();
    Theme t = currentTheme();

    canvas->setColor(style().bgColor);
    canvas->fillRoundedRect(r, style().borderRadius);

    // Scrollbar track
    if (contentHeight_ > height()) {
        int sbWidth = 12;
        int scrollY = currentScrollY();
        int thumbH = std::max(24, height() * height() / contentHeight_);
        int maxScroll = contentHeight_ - height();
        int thumbY = r.y;
        if (maxScroll > 0) {
            thumbY += (scrollY * (height() - thumbH)) / maxScroll;
        }

        canvas->setColor(t.scrollbarTrack);
        canvas->fillRoundedRect(Rect(r.right() - sbWidth, r.y, sbWidth, r.height), 4);

        canvas->setColor(t.scrollbarThumb);
        canvas->fillRoundedRect(Rect(r.right() - sbWidth + 2, thumbY + 1, sbWidth - 4, thumbH - 2), 3);
    }

    // Content clipping — border stroke on top
    if (style().borderWidth > 0) {
        canvas->setColor(style().borderColor);
        canvas->strokeRoundedRect(r, style().borderRadius, style().borderWidth);
    }
}

bool ScrollArea::handleEvent(Event& event) {
    if (!isEnabled()) return false;

    if (event.type == EventType::MouseWheel) {
        int newY = scrollY_ - event.wheelDelta * 30;
        scrollTo(scrollX_, newY);
        event.accepted = true;
        return true;
    }

    if (contentWidget_) {
        int localX = event.pos.x - x();
        int localY = event.pos.y - y();
        bool inContent = contentWidget_->geometry().contains(
            {localX + scrollX_, localY + currentScrollY()});
        if (inContent && (event.type == EventType::MouseMove ||
                          event.type == EventType::MouseDown ||
                          event.type == EventType::MouseUp)) {
            Point savedPos = event.pos;
            event.pos = {localX + scrollX_, localY + currentScrollY()};
            bool handled = contentWidget_->handleEvent(event);
            event.pos = savedPos;
            if (handled) return true;
        }
    }

    return false;
}

} // namespace ltgui
