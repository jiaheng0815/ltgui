#pragma once
#include "widget.h"
#include <string>
#include <vector>
#include <functional>

namespace ltgui {

class ContextMenu;

class MenuBar : public Widget {
public:
    explicit MenuBar(Widget* parent = nullptr);

    using ItemCallback = std::function<void()>;

    // Add a top-level menu (e.g. "File", "Edit"). Returns menu index.
    int addMenu(const std::string& label);

    // Add an item to a submenu. Returns item index within that menu.
    int addItem(int menuIdx, const std::string& text, ItemCallback cb = nullptr);
    void addSeparator(int menuIdx);

    WidgetType widgetType() const override { return WidgetType::Base; } // FIXME: add MenuBar to WidgetType enum
    Size sizeHint() const override;

protected:
    void paintSelf(NativeCanvas* canvas) override;
    bool handleEvent(Event& event) override;

private:
    struct MenuItem {
        std::string text;
        ItemCallback callback;
        bool separator = false;
    };
    struct Menu {
        std::string label;
        std::vector<MenuItem> items;
    };

    std::vector<Menu> menus_;
    int hoveredMenu_ = -1;      // which top-level menu is hovered
    int openMenu_ = -1;         // which top-level menu is open
    int hoveredItem_ = -1;      // which item in the open menu is hovered
    int itemHeight_ = 26;
    int menuBarHeight_ = 30;

    int menuX(int idx) const;
    int menuWidth(int idx) const;
};

} // namespace ltgui
