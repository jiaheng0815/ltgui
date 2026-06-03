#include "widgets/contextmenu.h"
#include "window.h"
#include "theme.h"
#include "platform/native_canvas.h"
#include <algorithm>

namespace ltgui {

ContextMenu::ContextMenu(Widget* parent) : Widget(parent) {
    style().bgColor = currentTheme().bgSecondary;
    style().fgColor = currentTheme().textPrimary;
    style().borderWidth = 1;
    style().borderColor = currentTheme().border;
    style().borderRadius = 6;
    style().setPadding(4, 2);
    setVisible(false);
}

int ContextMenu::addItem(const std::string& text, ItemCallback cb) {
    items_.push_back({text, std::move(cb), false});
    invalidateSizeHint();
    return static_cast<int>(items_.size()) - 1;
}

void ContextMenu::addSeparator() {
    items_.push_back({"", nullptr, true});
}

void ContextMenu::clear() {
    items_.clear();
    hovered_ = -1;
    invalidateSizeHint();
}

int ContextMenu::count() const { return static_cast<int>(items_.size()); }

std::string ContextMenu::itemText(int index) const {
    if (index >= 0 && index < static_cast<int>(items_.size())) return items_[index].text;
    return {};
}

void ContextMenu::popup(const Point& screenPos) {
    int w = bestWidth();
    int h = static_cast<int>(items_.size()) * itemHeight_ + style().paddingVert() + 4;
    setGeometry(Rect(screenPos.x, screenPos.y, w, h));
    hovered_ = -1;
    setVisible(true);
    raiseToTop();
    claimFocus();
    update();
}

void ContextMenu::dismiss() {
    setVisible(false);
}

Size ContextMenu::sizeHint() const {
    if (!sizeHintDirty()) return cachedSizeHint();
    setCachedSizeHint({bestWidth(), 100});
    return cachedSizeHint();
}

int ContextMenu::bestWidth() const {
    int w = 120;
    if (auto* win = window()) {
        if (auto* c = win->canvas()) {
            for (auto& item : items_) {
                if (item.separator) continue;
                int tw = c->measureText(item.text).width + 28;
                w = std::max(w, tw);
            }
        }
    }
    return w;
}

void ContextMenu::paintSelf(NativeCanvas* canvas) {
    Rect r = absoluteRect();
    Theme t = currentTheme();

    // Background
    canvas->setColor(style().bgColor);
    canvas->fillRoundedRect(r, style().borderRadius);

    // Border
    canvas->setColor(t.border);
    canvas->strokeRoundedRect(r, style().borderRadius, style().borderWidth);

    canvas->setFont(Font::systemDefault(12));
    int pad = style().paddingTop;
    int y = r.y + pad;

    for (int i = 0; i < static_cast<int>(items_.size()); i++) {
        if (items_[i].separator) {
            canvas->setColor(t.border);
            canvas->drawLine({r.x + 8, y + 3}, {r.right() - 8, y + 3}, 1);
            y += 8;
            continue;
        }

        Rect itemRect(r.x + 2, y, r.width - 4, itemHeight_);

        if (i == hovered_) {
            canvas->setColor(t.accent);
            canvas->fillRoundedRect(itemRect.adjusted(2, 1, -2, -1), 3);
            canvas->setColor(Color::White);
        } else {
            canvas->setColor(style().fgColor);
        }

        canvas->drawText(items_[i].text, itemRect.adjusted(12, 0, -8, 0),
                         NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter | NativeCanvas::SingleLine);
        y += itemHeight_;
    }
}

bool ContextMenu::handleEvent(Event& event) {
    if (!isEnabled() || !isVisible()) return false;

    int localY = event.pos.y - y();

    if (event.type == EventType::MouseMove) {
        int idx = (localY - style().paddingTop) / itemHeight_;
        if (idx >= 0 && idx < static_cast<int>(items_.size()) && !items_[idx].separator) {
            hovered_ = idx;
        } else {
            hovered_ = -1;
        }
        update();
        event.accepted = true;
        return true;
    }

    if (event.type == EventType::MouseDown && event.button == MouseButton::Left) {
        int idx = (localY - style().paddingTop) / itemHeight_;
        if (idx >= 0 && idx < static_cast<int>(items_.size()) && !items_[idx].separator) {
            if (items_[idx].callback) items_[idx].callback();
        }
        dismiss();
        event.accepted = true;
        return true;
    }

    if (event.type == EventType::MouseDown ||
        (event.type == EventType::KeyDown && event.key == Key::Escape)) {
        dismiss();
        event.accepted = true;
        return true;
    }

    return false;
}

} // namespace ltgui
