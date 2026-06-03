#pragma once
#include "color.h"
#include "font.h"
#include <algorithm>

namespace ltgui {

struct Style {
    Color bgColor;
    Color fgColor;
    Color borderColor;
    int borderWidth = 0;
    int borderRadius = 0;
    Font font;
    int paddingLeft = 0;
    int paddingTop = 0;
    int paddingRight = 0;
    int paddingBottom = 0;
    int marginLeft = 0;
    int marginTop = 0;
    int marginRight = 0;
    int marginBottom = 0;

    static Style defaultStyle();

    void setPadding(int all) {
        all = std::max(0, all);
        paddingLeft = paddingTop = paddingRight = paddingBottom = all;
    }
    void setPadding(int h, int v) {
        h = std::max(0, h);
        v = std::max(0, v);
        paddingLeft = paddingRight = h;
        paddingTop = paddingBottom = v;
    }
    void setMargin(int all) {
        all = std::max(0, all);
        marginLeft = marginTop = marginRight = marginBottom = all;
    }
    void setMargin(int h, int v) {
        h = std::max(0, h);
        v = std::max(0, v);
        marginLeft = marginRight = h;
        marginTop = marginBottom = v;
    }

    int paddingHorz() const { return paddingLeft + paddingRight; }
    int paddingVert() const { return paddingTop + paddingBottom; }
};

} // namespace ltgui
