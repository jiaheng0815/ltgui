#include "widgets/menubar.h"
#include "widgets/contextmenu.h"
#include "window.h"
#include "theme.h"
#include "platform/native_canvas.h"
#include <algorithm>

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
    setCachedSizeHint({400, menuBarHeight_});
    return cachedSizeHint();
}

int MenuBar::menuX(int idx) const {
    int x = 4;
    for (int i = 0; i < idx; i++) {
        x += menuWidth(i) + 2;
    }
    return x;
}

int MenuBar::menuWidth(int idx) const {
    if (idx < 0 || idx >= static_cast<int>(menus_.size())) return 0;
    // Approximate: 10px per char + padding
    return static_cast<int>(menus_[idx].label.size()) * 10 + 16;
}

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

        if (i == hoveredMenu_ || i == openMenu_) {
            canvas->setColor(t.accent);
            canvas->fillRoundedRect(itemRect.adjusted(1, 2, -1, -2), 3);
            canvas->setColor(Color::White);
        } else {
            canvas->setColor(style().fgColor);
        }

        canvas->drawText(menus_[i].label,
                         itemRect.adjusted(8, 0, -8, 0),
                         NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter);

        x += w + 2;
    }

    // Draw open submenu
    if (openMenu_ >= 0 && openMenu_ < static_cast<int>(menus_.size())) {
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
            if (j == hoveredItem_) {
                canvas->setColor(t.accent);
                canvas->fillRoundedRect(ir.adjusted(2, 1, -2, -1), 3);
                canvas->setColor(Color::White);
            } else {
                canvas->setColor(style().fgColor);
            }
            canvas->drawText(items[j].text, ir.adjusted(12, 0, -8, 0),
                             NativeCanvas::AlignLeft | NativeCanvas::AlignVCenter);
        }
    }
}

bool MenuBar::handleEvent(Event& event) {
    if (!isEnabled()) return false;

    int localX = event.pos.x - x();
    int localY = event.pos.y - y();

    // Figure out which top-level menu is under cursor
    int menuIdx = -1;
    int xPos = 4;
    for (int i = 0; i < static_cast<int>(menus_.size()); i++) {
        int w = menuWidth(i);
        if (localX >= xPos && localX < xPos + w && localY >= 0 && localY <= height()) {
            menuIdx = i;
            break;
        }
        xPos += w + 2;
    }

    // Figure out which submenu item is under cursor (if open)
    int itemIdx = -1;
    if (openMenu_ >= 0) {
        int dropY = height();
        int relY = localY - dropY - 2;
        itemIdx = relY / itemHeight_;
        if (itemIdx < 0 || itemIdx >= static_cast<int>(menus_[openMenu_].items.size()))
            itemIdx = -1;
    }

    switch (event.type) {
    case EventType::MouseMove:
        hoveredMenu_ = menuIdx;
        if (openMenu_ >= 0) hoveredItem_ = itemIdx;
        update();
        if (openMenu_ >= 0 || menuIdx >= 0) {
            event.accepted = true;
            return true;
        }
        return false;

    case EventType::MouseDown:
        if (event.button != MouseButton::Left) return false;

        if (openMenu_ >= 0) {
            // Click on a submenu item
            if (itemIdx >= 0 && itemIdx < static_cast<int>(menus_[openMenu_].items.size())) {
                auto& item = menus_[openMenu_].items[itemIdx];
                if (!item.separator && item.callback) {
                    item.callback();
                }
            }
            openMenu_ = -1;
            hoveredItem_ = -1;
            update();
            event.accepted = true;
            return true;
        }

        // Open/switch menu
        if (menuIdx >= 0) {
            openMenu_ = menuIdx;
            hoveredItem_ = -1;
            update();
            event.accepted = true;
            return true;
        }

        // Click outside: close
        openMenu_ = -1;
        update();
        return false;

    default:
        break;
    }

    return false;
}

} // namespace ltgui
