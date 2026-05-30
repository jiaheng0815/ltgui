#include "widgets/listbox.h"
#include "platform/native_canvas.h"
#include <algorithm>

namespace ltgui {

ListBox::ListBox(Widget* parent) : Widget(parent) {
    style().bgColor = Color::White;
    style().borderWidth = 1;
    style().borderColor = Color::Gray;
}

void ListBox::addItem(const std::string& item) {
    items_.push_back(item);
    update();
}

void ListBox::removeItem(int index) {
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        items_.erase(items_.begin() + index);
        if (selected_ == index) selected_ = -1;
        else if (selected_ > index) selected_--;
        update();
    }
}

void ListBox::clear() {
    items_.clear();
    selected_ = -1;
    update();
}

int ListBox::count() const {
    return static_cast<int>(items_.size());
}

std::string ListBox::item(int index) const {
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        return items_[index];
    }
    return {};
}

void ListBox::setSelected(int index) {
    if (index >= -1 && index < static_cast<int>(items_.size())) {
        selected_ = index;
        update();
        if (selectionCallback_) selectionCallback_(selected_);
    }
}

Size ListBox::sizeHint() const {
    return {160, 120};
}

int ListBox::visibleItems() const {
    return std::max(1, (height() - 2) / itemHeight_);
}

void ListBox::paintSelf(NativeCanvas* canvas) {
    Rect r = absoluteRect();

    // Background
    canvas->setColor(style().bgColor);
    canvas->fillRect(r);

    // Border
    if (style().borderWidth > 0) {
        canvas->setColor(style().borderColor);
        canvas->strokeRect(r, style().borderWidth);
    }

    // Items
    canvas->setFont(style().font);
    int visible = visibleItems();
    int maxOffset = std::max(0, static_cast<int>(items_.size()) - visible);
    scrollOffset_ = std::min(scrollOffset_, maxOffset);

    for (int i = scrollOffset_; i < std::min(static_cast<int>(items_.size()),
                                              scrollOffset_ + visible); i++) {
        int itemY = r.y + 1 + (i - scrollOffset_) * itemHeight_;
        Rect itemRect(r.x + 1, itemY, r.width - 2, itemHeight_);

        if (i == selected_) {
            canvas->setColor(Color(0, 120, 215));
            canvas->fillRect(itemRect);
            canvas->setColor(Color::White);
        } else {
            canvas->setColor(style().fgColor);
        }

        canvas->drawText(items_[i], itemRect,
                         NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter | NativeCanvas::SingleLine);
    }
}

bool ListBox::handleEvent(Event& event) {
    if (!isEnabled()) return false;

    int localY = event.pos.y - y();

    if (event.type == EventType::MouseDown && event.button == MouseButton::Left) {
        int relY = localY - 1;
        int index = scrollOffset_ + relY / itemHeight_;
        if (index >= 0 && index < static_cast<int>(items_.size())) {
            setSelected(index);
        }
        event.accepted = true;
        return true;
    }

    if (event.type == EventType::MouseWheel) {
        scrollOffset_ -= event.wheelDelta;
        scrollOffset_ = std::max(0, scrollOffset_);
        int visible = visibleItems();
        int maxOffset = std::max(0, static_cast<int>(items_.size()) - visible);
        scrollOffset_ = std::min(scrollOffset_, maxOffset);
        update();
        event.accepted = true;
        return true;
    }

    if (event.type == EventType::KeyDown) {
        if (event.key == Key::Down) {
            if (selected_ < static_cast<int>(items_.size()) - 1) {
                setSelected(selected_ + 1);
            }
            event.accepted = true;
            return true;
        }
        if (event.key == Key::Up) {
            if (selected_ > 0) {
                setSelected(selected_ - 1);
            }
            event.accepted = true;
            return true;
        }
    }

    return Widget::handleEvent(event);
}

} // namespace ltgui
