#include "widgets/combobox.h"
#include "window.h"
#include "theme.h"
#include "platform/native_canvas.h"
#include <algorithm>

namespace ltgui {

ComboBox* ComboBox::s_openCombo_ = nullptr;

ComboBox::ComboBox(Widget* parent) : Widget(parent) {
    style().bgColor = currentTheme().bgSecondary;
    style().fgColor = currentTheme().textPrimary;
    style().borderWidth = 1;
    style().borderColor = currentTheme().border;
    style().borderRadius = 4;
    style().setPadding(8, 4);
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
    if (!sizeHintDirty()) return cachedSizeHint();
    setCachedSizeHint({150, 30});
    return cachedSizeHint();
}

Rect ComboBox::effectiveGeometry() const {
    Rect r = absoluteRect();
    if (dropped_ && !items_.empty()) {
        int dropH = std::min(static_cast<int>(items_.size()) * 26, 200) + 2;
        bool dropDown = true;
        if (auto* win = window()) {
            int winH = win->getSize().height;
            if (r.bottom() + dropH > winH - 4) dropDown = false;
        }
        if (dropDown) {
            r.height += dropH;
        } else {
            r.y -= dropH;
            r.height += dropH;
        }
    }
    return r;
}

void ComboBox::paintSelf(NativeCanvas* canvas) {
    Rect r = absoluteRect();
    Theme t = currentTheme();

    canvas->setColor(style().bgColor);
    canvas->fillRoundedRect(r, style().borderRadius);

    if (style().borderWidth > 0) {
        canvas->setColor(dropped_ ? t.accent : style().borderColor);
        canvas->strokeRoundedRect(r, style().borderRadius, dropped_ ? 2 : style().borderWidth);
    }

    // Selected text
    canvas->setColor(style().fgColor);
    canvas->setFont(style().font);

    std::string displayText = currentText();
    Rect textRect(r.x + style().paddingLeft, r.y,
                  r.width - style().paddingHorz() - 20, r.height);
    canvas->drawText(displayText, textRect,
                     NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter | NativeCanvas::SingleLine);

    // Dropdown arrow (v-shape)
    int cx = r.right() - 13;
    int cy = r.y + r.height / 2;
    canvas->setColor(t.textSecondary);
    canvas->drawLine({cx - 4, cy - 2}, {cx, cy + 2}, 2);
    canvas->drawLine({cx, cy + 2}, {cx + 4, cy - 2}, 2);

    // Dropdown list — uses cached direction and height from handleEvent
    if (dropped_ && !items_.empty()) {
        Rect dropRect;
        if (dropDown_) {
            dropRect = Rect(r.x, r.bottom() + 1, r.width, dropHeight_);
        } else {
            int dropY = r.y - dropHeight_ - 1;
            if (dropY < 0) dropY = 0;
            dropRect = Rect(r.x, dropY, r.width, dropHeight_);
        }

        canvas->setColor(t.bgSecondary);
        canvas->fillRoundedRect(dropRect, 4);
        canvas->setColor(t.border);
        canvas->strokeRoundedRect(dropRect, 4);

        for (size_t i = 0; i < items_.size(); i++) {
            int itemY = dropRect.y + 1 + static_cast<int>(i) * 26;
            Rect itemRect(dropRect.x + 2, itemY, dropRect.width - 4, 26);

            if (static_cast<int>(i) == selected_) {
                canvas->setColor(t.accent);
                canvas->fillRoundedRect(itemRect.adjusted(2, 1, -2, -1), 3);
                canvas->setColor(Color::White);
            } else {
                canvas->setColor(style().fgColor);
            }

            canvas->drawText(items_[i], itemRect.adjusted(6, 0, -4, 0),
                             NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter | NativeCanvas::SingleLine);
        }
    }
}

void ComboBox::openDropdown() {
    dropped_ = true;
    dropHeight_ = std::min(static_cast<int>(items_.size()) * 26, 200) + 2;
    dropDown_ = true;
    if (auto* win = window()) {
        if (absoluteRect().bottom() + dropHeight_ > win->getSize().height - 4) {
            dropDown_ = false;
        }
    }
    // Track as the active open combo box
    if (s_openCombo_ && s_openCombo_ != this) {
        s_openCombo_->closeDropdown();
    }
    s_openCombo_ = this;
    update();
}

void ComboBox::closeDropdown() {
    if (!dropped_) return;
    dropped_ = false;
    if (s_openCombo_ == this) s_openCombo_ = nullptr;
    update();
}

bool ComboBox::closeIfClickOutside(const Point& absPos) {
    if (!s_openCombo_) return false;
    Rect eff = s_openCombo_->effectiveGeometry();
    if (!eff.contains(absPos)) {
        s_openCombo_->closeDropdown();
        return true;
    }
    return false;
}

bool ComboBox::handleEvent(Event& event) {
    if (!isEnabled()) return false;

    if (event.type == EventType::MouseDown && event.button == MouseButton::Left) {
        if (!dropped_) {
            openDropdown();
            event.accepted = true;
            return true;
        } else {
            int localY = event.pos.y - y();
            int relY;
            if (dropDown_) {
                relY = localY - height() - 1;
            } else {
                relY = localY + dropHeight_;
            }
            if (relY >= 0 && relY < dropHeight_) {
                int index = relY / 26;
                if (index >= 0 && index < static_cast<int>(items_.size())) {
                    setCurrentIndex(index);
                }
            }
            closeDropdown();
            event.accepted = true;
            return true;
        }
    }

    return false;
}

} // namespace ltgui
