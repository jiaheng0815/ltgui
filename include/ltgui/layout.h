#pragma once
#include "geometry.h"
#include <vector>
#include <map>

namespace ltgui {

class Widget;

class Layout {
public:
    virtual ~Layout() = default;
    virtual void layout(Widget* container) = 0;
    virtual Size preferredSize(const Widget* container) const = 0;
};

class BoxLayout : public Layout {
public:
    enum Direction {
        LeftToRight,
        TopToBottom
    };

    BoxLayout(Direction dir, int spacing = 4, int margin = 8);

    void addStretch(int factor = 1);
    void setSpacing(int spacing);
    void setMargin(int margin);
    void setDirection(Direction dir);

    void layout(Widget* container) override;
    Size preferredSize(const Widget* container) const override;

private:
    Direction direction_;
    int spacing_;
    int margin_;
    std::vector<int> stretchFactors_;
};

class GridLayout : public Layout {
public:
    GridLayout(int cols, int rowSpacing = 4, int colSpacing = 4, int margin = 8);

    void setColumnStretch(int col, int factor);
    void setRowStretch(int row, int factor);

    void layout(Widget* container) override;
    Size preferredSize(const Widget* container) const override;

private:
    int cols_;
    int rowSpacing_;
    int colSpacing_;
    int margin_;
    std::map<int, int> colStretch_;
    std::map<int, int> rowStretch_;
};

} // namespace ltgui
