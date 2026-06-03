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

    WidgetType widgetType() const override { return WidgetType::Base; }
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
    int hoveredMenu_ = -1;
    int openMenu_ = -1;
    int hoveredItem_ = -1;
    int itemHeight_ = 26;
    int menuBarHeight_ = 30;

    int menuX(int idx) const;
    int menuWidth(int idx) const;

    // Hit-test helpers
    int hitTestMenu(int localX, int localY) const;
    int hitTestItem(int localY) const;

    bool handleMouseDown(int menuIdx, int itemIdx);
    void closeMenu();
};

} // namespace ltgui
