#include "widgets/combobox.h"
#include "platform/native_canvas.h"
#include <algorithm>

namespace ltgui {

ComboBox::ComboBox(Widget* parent) : Widget(parent) {
    style().bgColor = Color::White;
    style().borderWidth = 1;
    style().borderColor = Color::Gray;
    style().borderRadius = 2;
    style().setPadding(6, 4);
}

void ComboBox::addItem(const std::string& item) {
    items_.push_back(item);
    if (selected_ < 0) selected_ = 0;
    update();
}

void ComboBox::removeItem(int index) {
    if (index >= 0 && index < static_cast<int>(items_.size())) {
        items_.erase(items_.begin() + index);
        if (selected_ == index) selected_ = std::min(selected_, static_cast<int>(items_.size()) - 1);
        else if (selected_ > index) selected_--;
        update();
    }
}

void ComboBox::clear() {
    items_.clear();
    selected_ = -1;
    dropped_ = false;
    update();
}

int ComboBox::count() const {
    return static_cast<int>(items_.size());
}

std::string ComboBox::currentText() const {
    if (selected_ >= 0 && selected_ < static_cast<int>(items_.size())) {
        return items_[selected_];
    }
    return {};
}

void ComboBox::setCurrentIndex(int index) {
    if (index >= -1 && index < static_cast<int>(items_.size())) {
        selected_ = index;
        update();
        if (selectionCallback_) selectionCallback_(selected_);
    }
}

Size ComboBox::sizeHint() const {
    return {140, 28};
}

Rect ComboBox::effectiveGeometry() const {
    Rect r = absoluteRect();
    if (dropped_ && !items_.empty()) {
        int dropH = std::min(static_cast<int>(items_.size()) * 24, 200);
        r.height += dropH;
    }
    return r;
}

void ComboBox::paintSelf(NativeCanvas* canvas) {
    Rect r = absoluteRect();

    // Background
    canvas->setColor(style().bgColor);
    canvas->fillRect(r);

    // Border
    if (style().borderWidth > 0) {
        canvas->setColor(style().borderColor);
        canvas->strokeRect(r, style().borderWidth);
    }

    // Selected text
    canvas->setColor(style().fgColor);
    canvas->setFont(style().font);

    std::string displayText = currentText();
    Rect textRect(r.x + style().paddingLeft, r.y,
                  r.width - style().paddingHorz() - 20, r.height);
    canvas->drawText(displayText, textRect,
                     NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter | NativeCanvas::SingleLine);

    // Dropdown arrow
    int arrowX = r.right() - 16;
    int arrowY = r.y + r.height / 2;
    canvas->setColor(Color::Gray);
    canvas->drawLine({arrowX, arrowY - 3}, {arrowX + 6, arrowY - 3});
    canvas->drawLine({arrowX + 3, arrowY - 3}, {arrowX + 3, arrowY + 3});

    // Dropdown list
    if (dropped_ && !items_.empty()) {
        int dropH = std::min(static_cast<int>(items_.size()) * 24, 200);
        Rect dropRect(r.x, r.bottom(), r.width, dropH);

        canvas->setColor(Color::White);
        canvas->fillRect(dropRect);
        canvas->setColor(Color::Gray);
        canvas->strokeRect(dropRect);

        for (size_t i = 0; i < items_.size(); i++) {
            int itemY = dropRect.y + static_cast<int>(i) * 24;
            Rect itemRect(dropRect.x + 2, itemY, dropRect.width - 4, 24);

            if (static_cast<int>(i) == selected_) {
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
}

bool ComboBox::handleEvent(Event& event) {
    if (!isEnabled()) return false;

    int localY = event.pos.y - y();

    if (event.type == EventType::MouseDown && event.button == MouseButton::Left) {
        if (!dropped_) {
            dropped_ = true;
            update();
        } else {
            // Check if clicking on a dropdown item
            int dropH = std::min(static_cast<int>(items_.size()) * 24, 200);
            int relY = localY - height();
            if (relY >= 0 && relY < dropH) {
                int index = relY / 24;
                if (index >= 0 && index < static_cast<int>(items_.size())) {
                    setCurrentIndex(index);
                }
            }
            dropped_ = false;
            update();
        }
        event.accepted = true;
        return true;
    }

    return Widget::handleEvent(event);
}

} // namespace ltgui
