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
        all = std::clamp(all, 0, static_cast<int>(INT16_MAX));
        paddingLeft = paddingTop = paddingRight = paddingBottom = static_cast<int16_t>(all);
    }
    void setPadding(int h, int v) {
        h = std::clamp(h, 0, static_cast<int>(INT16_MAX));
        v = std::clamp(v, 0, static_cast<int>(INT16_MAX));
        paddingLeft = paddingRight = static_cast<int16_t>(h);
        paddingTop = paddingBottom = static_cast<int16_t>(v);
    }
    void setMargin(int all) {
        all = std::clamp(all, 0, static_cast<int>(INT16_MAX));
        marginLeft = marginTop = marginRight = marginBottom = static_cast<int16_t>(all);
    }
    void setMargin(int h, int v) {
        h = std::clamp(h, 0, static_cast<int>(INT16_MAX));
        v = std::clamp(v, 0, static_cast<int>(INT16_MAX));
        marginLeft = marginRight = static_cast<int16_t>(h);
        marginTop = marginBottom = static_cast<int16_t>(v);
    }

    int paddingHorz() const { return paddingLeft + paddingRight; }
    int paddingVert() const { return paddingTop + paddingBottom; }
};

} // namespace ltgui
