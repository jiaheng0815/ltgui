#pragma once
#include "platform/native_canvas.h"
#include <vector>

namespace ltgui {

// Records NativeCanvas draw calls for headless paint assertions.
// setColor() pushes onto a color stack (last set wins, matching the
// real backends' stateful behavior).
class MockCanvas : public NativeCanvas {
public:
    struct Fill {
        Color color;
        Rect rect;
        int radius = 0;
    };
    struct Stroke {
        Color color;
        Rect rect;
        int radius = 0;
        int width = 0;
    };
    struct Ellipse {
        Color color;
        Rect rect;
        bool filled = false;
        int width = 0;
    };
    struct Text {
        std::string text;
        Rect rect;
        Color color;
    };

    std::vector<Fill>     fills;
    std::vector<Stroke>   strokes;
    std::vector<Ellipse>  ellipses;
    std::vector<Text>     texts;
    std::vector<Color>    colors; // every setColor call

    Color currentColor() const { return colors.empty() ? Color::Transparent : colors.back(); }
    void reset() { fills.clear(); strokes.clear(); texts.clear(); colors.clear(); }

    // --- NativeCanvas ---
    void resize(int, int) override {}
    void beginPaint() override {}
    void endPaint() override {}
    void setColor(const Color& c) override { colors.push_back(c); }
    void setFont(const Font&) override {}
    void fillRect(const Rect& r) override { fills.push_back({currentColor(), r, 0}); }
    void strokeRect(const Rect& r, int w = 1) override { strokes.push_back({currentColor(), r, 0, w}); }
    void fillRoundedRect(const Rect& r, int radius) override { fills.push_back({currentColor(), r, radius}); }
    void strokeRoundedRect(const Rect& r, int radius, int w = 1) override { strokes.push_back({currentColor(), r, radius, w}); }
    void drawText(const std::string& t, const Rect& r, int = 0) override { texts.push_back({t, r, currentColor()}); }
    void drawLine(const Point&, const Point&, int = 1) override {}
    void fillEllipse(const Rect& r) override { ellipses.push_back({currentColor(), r, true, 0}); }
    void strokeEllipse(const Rect& r, int w = 1) override { ellipses.push_back({currentColor(), r, false, w}); }
    void drawImage(const std::string&, const Rect&) override {}
    Size measureText(const std::string&) override { return {10, 10}; }

    // Test helpers
    bool anyFillWithColor(const Color& c) const {
        for (auto& f : fills) if (f.color == c) return true;
        return false;
    }
    int fillCountWithColor(const Color& c) const {
        int n = 0;
        for (auto& f : fills) if (f.color == c) n++;
        return n;
    }
};

} // namespace ltgui
