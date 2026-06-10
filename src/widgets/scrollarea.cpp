#include "widgets/scrollarea.h"
#include "window.h"
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

Widget* ScrollArea::widget() const {
    return contentWidget();
}

Widget* ScrollArea::contentWidget() const {
    // Guard against stale pointer: if the stored pointer's parent isn't
    // us anymore (e.g. removeChild was called externally), fall back to
    // the first child in the list.
    if (contentWidget_ && contentWidget_->parent() == this)
        return contentWidget_;
    return children().empty() ? nullptr : children()[0].get();
}

void ScrollArea::setWidget(std::unique_ptr<Widget> widget) {
    if (auto* old = contentWidget()) {
        removeChild(old);
        contentWidget_ = nullptr;
    }
    if (widget) {
        contentWidget_ = widget.get();
        addChild(std::move(widget));
        Size hint = contentWidget_->sizeHint();
        contentWidth_ = hint.width;
        contentHeight_ = hint.height;
        contentWidget_->setGeometry(Rect(0, 0, hint.width, hint.height));
    }
    update();
}

void ScrollArea::updateScrollBars() {
    if (auto* cw = contentWidget()) {
        Size hint = cw->sizeHint();
        contentWidth_ = hint.width;
        contentHeight_ = hint.height;
    }
}

Size ScrollArea::sizeHint() const {
    if (!sizeHintDirty()) return cachedSizeHint();
    float dpi = window() ? window()->dpiScale() : 1.0f;
    setCachedSizeHint({static_cast<int>(200 * dpi), static_cast<int>(200 * dpi)});
    return cachedSizeHint();
}

int ScrollArea::currentScrollY() {
    return static_cast<int>(scrollYAnim_.value());
}

void ScrollArea::scrollTo(int x, int y) {
    scrollX_ = std::max(0, std::min(x, std::max(0, contentWidth_ - width())));
    scrollY_ = std::max(0, std::min(y, std::max(0, contentHeight_ - height())));
    scrollYAnim_.setTarget(static_cast<float>(scrollY_), 200, Easing::EaseOut);
    if (auto* cw = contentWidget()) {
        cw->setGeometry(Rect(-scrollX_, -currentScrollY(), contentWidth_, contentHeight_));
    }
    update();
}

void ScrollArea::paintSelf(NativeCanvas* canvas) {
    Rect r = absoluteRect();
    Theme t = currentTheme();

    // Update the content widget position from the current animated
    // scroll value each frame.  scrollTo() sets the animation target
    // but only sets the geometry once; without this the content jumps
    // to the target while the scrollbar thumb animates smoothly —
    // producing a visual disconnect.  Updating here ties the content
    // position to the animation frame rate (~60 FPS while animating).
    if (auto* cw = contentWidget()) {
        int curScrollY = currentScrollY();
        Rect targetGeo(-scrollX_, -curScrollY, contentWidth_, contentHeight_);
        if (cw->geometry() != targetGeo) {
            cw->setGeometry(targetGeo);
        }
    }

    canvas->setColor(style().bgColor);
    canvas->fillRoundedRect(r, style().borderRadius);

    // Scrollbar track — guard against zero contentHeight (avoids div-by-zero)
    if (contentHeight_ > 0 && contentHeight_ > height()) {
        int sbWidth = 12;
        int scrollY = currentScrollY();
        // Use int64_t to avoid overflow when height >= 46341 (sqrt(INT_MAX))
        int thumbH = std::max(24, (int)((int64_t)height() * height() / contentHeight_));
        int maxScroll = contentHeight_ - height();
        int thumbY = r.y;
        if (maxScroll > 0) {
            thumbY += (int)((int64_t)scrollY * (height() - thumbH) / maxScroll);
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

    // Scrollbar hit test — check if mouse is on the scrollbar track
    int localX = event.pos.x - x();
    int localY = event.pos.y - y();
    bool onScrollbar = (contentHeight_ > 0 && contentHeight_ > height()) && (localX >= width() - 12);

    if (event.type == EventType::MouseDown && onScrollbar) {
        // Calculate scrollbar thumb position (contentHeight_ > 0 guaranteed by onScrollbar)
        int curScrollY = currentScrollY();
        int thumbH = std::max(24, (int)((int64_t)height() * height() / contentHeight_));
        int maxScroll = contentHeight_ - height();
        int thumbY = 0;
        if (maxScroll > 0) {
            thumbY = (curScrollY * (height() - thumbH)) / maxScroll;
        }
        if (localY < thumbY) {
            // Click above thumb: page up
            int newY = scrollY_ - height();
            scrollTo(scrollX_, newY);
        } else if (localY > thumbY + thumbH) {
            // Click below thumb: page down
            int newY = scrollY_ + height();
            scrollTo(scrollX_, newY);
        } else {
            // Click on thumb: start dragging
            draggingScrollbar_ = true;
            dragStartMouseY_ = event.pos.y;
            dragStartScrollY_ = scrollY_;
        }
        event.accepted = true;
        return true;
    }

    if (event.type == EventType::MouseMove && draggingScrollbar_) {
        int thumbH = std::max(24, (int)((int64_t)height() * height() / contentHeight_));
        int maxScroll = contentHeight_ - height();
        int trackH = height() - thumbH;
        if (maxScroll > 0 && trackH > 0) {
            int dy = event.pos.y - dragStartMouseY_;
            int newY = dragStartScrollY_ + (dy * maxScroll) / trackH;
            scrollTo(scrollX_, newY);
        }
        event.accepted = true;
        return true;
    }

    if (event.type == EventType::MouseUp && draggingScrollbar_) {
        draggingScrollbar_ = false;
        event.accepted = true;
        return true;
    }

    if (auto* cw = contentWidget()) {
        if (!onScrollbar) {
            // Check against content logical rect (0, 0, contentW, contentH),
            // NOT cw->geometry() which includes the -scrollXY offset.
            int cx = localX + scrollX_;
            int cy = localY + currentScrollY();
            bool inContent = cx >= 0 && cy >= 0 &&
                             cx < contentWidth_ && cy < contentHeight_;
            if (inContent && (event.type == EventType::MouseMove ||
                              event.type == EventType::MouseDown ||
                              event.type == EventType::MouseUp)) {
                Point savedPos = event.pos;
                event.pos = {cx, cy};
                bool handled = cw->handleEvent(event);
                event.pos = savedPos;
                if (handled) return true;
            }
        }
    }

    return false;
}

} // namespace ltgui
