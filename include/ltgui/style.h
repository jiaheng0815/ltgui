#pragma once
#include "color.h"
#include "font.h"
#include <cstdint>
#include <algorithm>

namespace ltgui {

struct Style {
    Color bgColor;
    Color fgColor;
    Color borderColor;
    int borderWidth = 0;
    int borderRadius = 0;
    Font font;
    int16_t paddingLeft = 0;
    int16_t paddingTop = 0;
    int16_t paddingRight = 0;
    int16_t paddingBottom = 0;
    int16_t marginLeft = 0;
    int16_t marginTop = 0;
    int16_t marginRight = 0;
    int16_t marginBottom = 0;

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
