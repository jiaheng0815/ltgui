#include "widgets/menubar.h"
#include "window.h"
#include "theme.h"
#include "platform/native_canvas.h"

namespace ltgui {

MenuBar::MenuBar(Widget* parent) : Widget(parent) {
    style().bgColor = currentTheme().bgTertiary;
    style().fgColor = currentTheme().textPrimary;
}

int MenuBar::addMenu(const std::string& label) {
    menus_.push_back({label, {}});
    invalidateSizeHint();
    update();
    return static_cast<int>(menus_.size()) - 1;
}

int MenuBar::addItem(int menuIdx, const std::string& text, ItemCallback cb) {
    if (menuIdx < 0 || menuIdx >= static_cast<int>(menus_.size())) return -1;
    menus_[menuIdx].items.push_back({text, std::move(cb), false});
    return static_cast<int>(menus_[menuIdx].items.size()) - 1;
}

void MenuBar::addSeparator(int menuIdx) {
    if (menuIdx < 0 || menuIdx >= static_cast<int>(menus_.size())) return;
    menus_[menuIdx].items.push_back({"", nullptr, true});
}

Size MenuBar::sizeHint() const {
    if (!sizeHintDirty()) return cachedSizeHint();
    float dpi = window() ? window()->dpiScale() : 1.0f;
    setCachedSizeHint({static_cast<int>(400 * dpi), static_cast<int>(menuBarHeight_ * dpi)});
    return cachedSizeHint();
}

// --- internal helpers ---

int MenuBar::menuX(int idx) const {
    int x = 4;
    for (int i = 0; i < idx; i++)
        x += menuWidth(i) + 2;
    return x;
}

int MenuBar::menuWidth(int idx) const {
    if (idx < 0 || idx >= static_cast<int>(menus_.size())) return 0;
    // Approximate for 13px system font (~10px avg char width + 16px padding)
    return static_cast<int>(menus_[idx].label.size()) * 10 + 16;
}

int MenuBar::hitTestMenu(int localX, int localY) const {
    if (localY < 0 || localY > height()) return -1;
    int xPos = 4;
    for (int i = 0; i < static_cast<int>(menus_.size()); i++) {
        if (localX >= xPos && localX < xPos + menuWidth(i))
            return i;
        xPos += menuWidth(i) + 2;
    }
    return -1;
}

int MenuBar::hitTestItem(int localY) const {
    int relY = localY - height() - 2;
    int idx = relY / itemHeight_;
    if (idx < 0 || openMenu_ < 0 ||
        idx >= static_cast<int>(menus_[openMenu_].items.size()))
        return -1;
    return idx;
}

static void drawHighlightedItem(NativeCanvas* canvas, const Rect& r,
                                 const Color& accent) {
    canvas->setColor(accent);
    canvas->fillRoundedRect(r.adjusted(1, 2, -1, -2), 3);
    canvas->setColor(Color::White);
}

// --- paint ---

void MenuBar::paintSelf(NativeCanvas* canvas) {
    Rect r = absoluteRect();
    Theme t = currentTheme();

    canvas->setColor(style().bgColor);
    canvas->fillRect(r);
    canvas->setFont(Font::systemDefault(13));

    int x = r.x + 4;
    for (int i = 0; i < static_cast<int>(menus_.size()); i++) {
        int w = menuWidth(i);
        Rect itemRect(x, r.y, w, r.height);

        if (i == hoveredMenu_ || i == openMenu_)
            drawHighlightedItem(canvas, itemRect, t.accent);
        else
            canvas->setColor(style().fgColor);

        canvas->drawText(menus_[i].label,
                         itemRect.adjusted(8, 0, -8, 0),
                         NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter);
        x += w + 2;
    }

    // Drop-down submenu
    if (openMenu_ < 0 || openMenu_ >= static_cast<int>(menus_.size())) return;

    auto& items = menus_[openMenu_].items;
    int dropW = 180;
    int dropH = static_cast<int>(items.size()) * itemHeight_ + 4;
    int dropX = r.x + menuX(openMenu_);
    int dropY = r.bottom();

    canvas->setColor(t.bgSecondary);
    canvas->fillRoundedRect(Rect(dropX, dropY, dropW, dropH), 4);
    canvas->setColor(t.border);
    canvas->strokeRoundedRect(Rect(dropX, dropY, dropW, dropH), 4);
    canvas->setFont(Font::systemDefault(12));

    for (int j = 0; j < static_cast<int>(items.size()); j++) {
        int iy = dropY + 2 + j * itemHeight_;
        if (items[j].separator) {
            canvas->setColor(t.border);
            canvas->drawLine({dropX + 8, iy + itemHeight_ / 2},
                             {dropX + dropW - 8, iy + itemHeight_ / 2}, 1);
            continue;
        }
        Rect ir(dropX + 2, iy, dropW - 4, itemHeight_);
        if (j == hoveredItem_)
            drawHighlightedItem(canvas, ir, t.accent);
        else
            canvas->setColor(style().fgColor);
        canvas->drawText(items[j].text, ir.adjusted(12, 0, -8, 0),
                         NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter);
    }
}

// --- events ---

bool MenuBar::handleEvent(Event& event) {
    if (!isEnabled()) return false;

    int localX = event.pos.x - x();
    int localY = event.pos.y - y();
    int menuIdx = hitTestMenu(localX, localY);
    int itemIdx = openMenu_ >= 0 ? hitTestItem(localY) : -1;

    switch (event.type) {
    case EventType::MouseMove: {
        int prevHovered = hoveredMenu_;
        hoveredMenu_ = menuIdx;
        if (openMenu_ >= 0) hoveredItem_ = itemIdx;
        if (hoveredMenu_ != prevHovered) update();
        event.accepted = (openMenu_ >= 0 || menuIdx >= 0);
        return event.accepted;
    }
    case EventType::MouseDown:
        if (event.button != MouseButton::Left) return false;
        event.accepted = true;
        return handleMouseDown(menuIdx, itemIdx);
    default:
        break;
    }
    return false;
}

bool MenuBar::handleMouseDown(int menuIdx, int itemIdx) {
    if (openMenu_ >= 0) {
        if (itemIdx >= 0 && !menus_[openMenu_].items[itemIdx].separator) {
            auto& cb = menus_[openMenu_].items[itemIdx].callback;
            if (cb) cb();
        }
        closeMenu();
        return true;
    }

    if (menuIdx >= 0) {
        openMenu_ = menuIdx;
        hoveredItem_ = -1;
        update();
        return true;
    }

    closeMenu();
    return false;
}

void MenuBar::closeMenu() {
    openMenu_ = -1;
    hoveredItem_ = -1;
    update();
}

} // namespace ltgui
