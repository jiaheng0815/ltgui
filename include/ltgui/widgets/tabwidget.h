#pragma once
#include "widget.h"
#include <string>
#include <vector>

namespace ltgui {

class TabWidget : public Widget {
public:
    explicit TabWidget(Widget* parent = nullptr);

    int addTab(const std::string& label);
    void removeTab(int index);
    int count() const;
    int currentIndex() const { return current_; }
    void setCurrentIndex(int index);

    Widget* tabContent(int index) const;
    Widget* currentContent() const;

    Size sizeHint() const override;
    void setGeometry(const Rect& rect) override;

protected:
    void paintSelf(NativeCanvas* canvas) override;
    bool handleEvent(Event& event) override;

private:
    struct Tab {
        std::string label;
        Widget* content = nullptr;
    };
    std::vector<Tab> tabs_;
    int current_ = -1;
    int hovered_ = -1;
    int tabBarHeight_ = 32;

    Rect tabRect(int index) const;
    int totalTabWidth() const;
};

} // namespace ltgui
