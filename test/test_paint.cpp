#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "ltgui.h"
#include "mock_canvas.h"
#include <thread>
#include <chrono>

using namespace ltgui;

// Headless paint tests: widgets paint onto a MockCanvas that records draw
// calls, so we can assert the resolved (state-aware) colors end up on the
// canvas. Widgets are painted without a Window — text measurement falls
// back to the MockCanvas's fixed 10x10 size.
//
// State transitions (hover/press) are driven through real events so the
// tests exercise the actual handleEvent() code paths. handleEvent() is
// protected on some subclasses, so tests go through the Widget& interface.

namespace {

MockCanvas g_canvas;

void paintWidget(Widget& w, MockCanvas& canvas) {
    canvas.reset();
    w.setGeometry(Rect(0, 0, 120, 40));
    w.paint(&canvas, Rect(0, 0, 120, 40));
}

bool dispatch(Widget& w, EventType type, int x, int y, MouseButton button = MouseButton::Left) {
    Event e;
    e.type = type;
    e.pos = {x, y};
    e.button = button;
    return static_cast<Widget&>(w).handleEvent(e);
}

// Drives the AnimationManager with real time so 150ms hover transitions
// complete before the paint assertions.
void advanceAnimations(int ms) {
    auto& mgr = AnimationManager::instance();
    for (int elapsed = 0; elapsed < ms; elapsed += 10) {
        std::this_thread::sleep_for(std::chrono::milliseconds(10));
        mgr.tick();
    }
}

} // namespace

TEST_CASE("Button paint states use resolved colors") {
    Theme light = Theme::Light();
    Button btn("Click");
    btn.setGeometry(Rect(0, 0, 120, 40));

    SUBCASE("normal state paints theme bgSecondary") {
        paintWidget(btn, g_canvas);
        CHECK(g_canvas.fillCountWithColor(light.bgSecondary) >= 1);
    }

    SUBCASE("hovered state paints theme accentHover") {
        dispatch(btn, EventType::MouseMove, 20, 20); // inside bounds
        advanceAnimations(250); // let the 150ms hover transition finish
        paintWidget(btn, g_canvas);
        CHECK(g_canvas.fillCountWithColor(light.accentHover) >= 1);
    }

    SUBCASE("pressed state paints theme accentPressed") {
        dispatch(btn, EventType::MouseMove, 20, 20);
        dispatch(btn, EventType::MouseDown, 20, 20);
        advanceAnimations(250);
        paintWidget(btn, g_canvas);
        CHECK(g_canvas.fillCountWithColor(light.accentPressed) >= 1);
    }

    SUBCASE("disabled state paints theme bgTertiary") {
        btn.setEnabled(false);
        paintWidget(btn, g_canvas);
        CHECK(g_canvas.fillCountWithColor(light.bgTertiary) >= 1);
    }

    SUBCASE("custom style accent overrides theme accent") {
        btn.style().accent = Color(200, 10, 10);
        dispatch(btn, EventType::MouseMove, 20, 20);
        advanceAnimations(250);
        paintWidget(btn, g_canvas);
        CHECK(g_canvas.fillCountWithColor(Color(200, 10, 10)) >= 1);
    }
}

TEST_CASE("Button click emits onClicked") {
    Button btn("Go");
    btn.setGeometry(Rect(0, 0, 120, 40));
    int clicks = 0;
    btn.onClicked.connect([&]() { clicks++; });

    dispatch(btn, EventType::MouseMove, 10, 10);
    dispatch(btn, EventType::MouseDown, 10, 10);
    dispatch(btn, EventType::MouseUp, 10, 10);
    CHECK(clicks == 1);
}

TEST_CASE("CheckBox checked state paints accent") {
    Theme light = Theme::Light();
    CheckBox cb("Accept");
    cb.setGeometry(Rect(0, 0, 120, 30));
    cb.setChecked(true);
    cb.paint(&g_canvas, Rect(0, 0, 120, 30));
    CHECK(g_canvas.fillCountWithColor(light.accent) >= 1);
}

TEST_CASE("Slider thumb uses accent when hovered") {
    Theme light = Theme::Light();
    Slider s;
    s.setGeometry(Rect(0, 0, 150, 28));

    // Normal state: thumb border is the theme border color.
    s.paint(&g_canvas, Rect(0, 0, 150, 28));
    bool borderStroke = false;
    for (auto& e : g_canvas.ellipses) {
        if (!e.filled && e.color == light.border) borderStroke = true;
    }
    CHECK(borderStroke);

    // Hover over the thumb (value 0 -> thumb center at x=16, y=14).
    // The state-aware accent fallback gives hovered state accentHover.
    dispatch(s, EventType::MouseMove, 16, 14);
    advanceAnimations(250);
    paintWidget(s, g_canvas);
    bool accentStroke = false;
    for (auto& e : g_canvas.ellipses) {
        if (!e.filled && e.color == light.accentHover) accentStroke = true;
    }
    CHECK(accentStroke);
}

TEST_CASE("Tooltip uses theme tooltip colors") {
    Theme light = Theme::Light();
    Tooltip tip;
    tip.setText("Hello");
    tip.setGeometry(Rect(0, 0, 80, 26));
    tip.setVisible(true);
    tip.paint(&g_canvas, Rect(0, 0, 80, 26));
    // Background comes from the theme's tooltipBg (via style)
    CHECK(g_canvas.fillCountWithColor(light.tooltipBg) >= 1);
}

TEST_CASE("Theme switch updates resolved style automatically") {
    // Widget styles fall back to the CURRENT theme, so switching themes
    // changes what gets painted without touching the widget.
    Button btn("T");
    btn.setGeometry(Rect(0, 0, 120, 40));

    setTheme(Theme::Light());
    paintWidget(btn, g_canvas);
    CHECK(g_canvas.fillCountWithColor(Theme::Light().bgSecondary) >= 1);

    setTheme(Theme::Dark());
    paintWidget(btn, g_canvas);
    CHECK(g_canvas.fillCountWithColor(Theme::Dark().bgSecondary) >= 1);
    CHECK(g_canvas.fillCountWithColor(Theme::Light().bgSecondary) == 0);

    setTheme(Theme::Light()); // restore for other tests
}
