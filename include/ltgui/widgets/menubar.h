#pragma once
#include "widget.h"
#include <string>
#include <vector>
#include <functional>

namespace ltgui {

class MenuBar : public Widget {
public:
    explicit MenuBar(Widget* parent = nullptr);

    using ItemCallback = std::function<void()>;

    int addMenu(const std::string& label);
    int addItem(int menuIdx, const std::string& text, ItemCallback cb = nullptr);
    void addSeparator(int menuIdx);

    // Per-item attributes (call after addItem)
    void setItemShortcut(int menuIdx, int itemIdx, const std::string& shortcut);
    void setItemCheckable(int menuIdx, int itemIdx, bool checkable);
    void setItemChecked(int menuIdx, int itemIdx, bool checked);
    bool isItemChecked(int menuIdx, int itemIdx) const;
    void setItemRadio(int menuIdx, int itemIdx, int radioGroup);
    int addSubmenu(int menuIdx, int itemIdx, const std::string& label);
    int addSubItem(int menuIdx, int itemIdx, int subIdx,
                   const std::string& text, ItemCallback cb = nullptr);
    void addSubSeparator(int menuIdx, int itemIdx, int subIdx);

    WidgetType widgetType() const override { return WidgetType::MenuBar; }
    bool canAcceptFocus() const override { return true; }
    Size sizeHint() const override;

protected:
    void paintSelf(NativeCanvas* canvas) override;
    bool handleEvent(Event& event) override;

private:
    struct MenuItem {
        std::string text;
        ItemCallback callback;
        bool separator = false;
        bool checkable = false;
        bool checked = false;
        bool radio = false;
        int radioGroup = -1;
        std::string shortcut;
        std::vector<MenuItem> submenu;
    };
    struct Menu {
        std::string label;
        std::vector<MenuItem> items;
    };

    std::vector<Menu> menus_;
    int hoveredMenu_ = -1;
    int openMenu_ = -1;
    int hoveredItem_ = -1;
    int openSubmenu_ = -1;  // submenu index on openMenu_'s item
    int itemHeight_ = 26;
    int menuBarHeight_ = 30;
    bool keyboardNav_ = false;

    int menuX(int idx) const;
    int menuWidth(int idx) const;
    int hitTestMenu(int localX, int localY) const;
    int hitTestItem(int localY) const;
    bool handleMouseDown(int menuIdx, int itemIdx);
    void closeMenu();
    void paintItem(NativeCanvas* canvas, const Rect& r, const MenuItem& item,
                   bool hovered, int depth);
    int dropWidth(const std::vector<MenuItem>& items, int depth) const;
};

} // namespace ltgui
