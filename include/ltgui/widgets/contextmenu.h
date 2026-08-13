#pragma once
#include "widget.h"
#include <string>
#include <vector>
#include <functional>

namespace ltgui {

class ContextMenu : public Widget {
public:
    explicit ContextMenu(Widget* parent = nullptr);

    using ItemCallback = std::function<void()>;

    int addItem(const std::string& text, ItemCallback cb = nullptr);
    void addSeparator();
    void clear();

    int count() const;
    std::string itemText(int index) const;

    // Show the menu at a screen position (absolute coordinates).
    // The menu closes automatically when clicking outside or selecting an item.
    void popup(const Point& screenPos);
    void dismiss();

    LTGUI_DECLARE_WIDGET_TYPE(ContextMenu)
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
    };
    std::vector<MenuItem> items_;
    int hovered_ = -1;
    int itemHeight_ = 26;

    int bestWidth() const;
};

} // namespace ltgui
