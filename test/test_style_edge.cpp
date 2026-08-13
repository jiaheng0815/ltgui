#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "style.h"
#include "color.h"
#include "theme.h"

using namespace ltgui;

// ============================================================
// 刁钻角度：Style 和 Theme
// ============================================================

TEST_CASE("Style edge: padding") {
    SUBCASE("negative padding is clamped to zero") {
        Style s;
        s.setPadding(-5);
        CHECK(s.paddingLeft == 0);
        CHECK(s.paddingHorz() == 0);
        CHECK(s.paddingVert() == 0);
    }

    SUBCASE("mixed h/v padding") {
        Style s;
        s.setPadding(10, 5);
        CHECK(s.paddingLeft == 10);
        CHECK(s.paddingRight == 10);
        CHECK(s.paddingTop == 5);
        CHECK(s.paddingBottom == 5);
    }

    SUBCASE("zero padding doesn't crash") {
        Style s;
        s.setPadding(0);
        CHECK(s.paddingLeft == 0);
        CHECK(s.paddingHorz() == 0);
        CHECK(s.paddingVert() == 0);
    }
}

TEST_CASE("Style edge: default style is consistent") {
    Style s1 = Style::defaultStyle();
    Style s2 = Style::defaultStyle();
    // Should be the same every time
    CHECK(s1.bgColor == s2.bgColor);
    CHECK(s1.fgColor == s2.fgColor);
}

TEST_CASE("Color edge: static constants") {
    SUBCASE("Transparent has alpha 0") {
        CHECK(Color::Transparent.a == 0);
    }

    SUBCASE("Black and White are opposites") {
        CHECK(Color::Black.r == 0);
        CHECK(Color::Black.g == 0);
        CHECK(Color::Black.b == 0);
        CHECK(Color::White.r == 255);
        CHECK(Color::White.g == 255);
        CHECK(Color::White.b == 255);
    }

    SUBCASE("toARGB roundtrips through components") {
        Color c(10, 20, 30, 40);
        uint32_t argb = c.toARGB();
        CHECK(((argb >> 24) & 0xFF) == 40); // A
        CHECK(((argb >> 16) & 0xFF) == 10); // R
        CHECK(((argb >> 8)  & 0xFF) == 20); // G
        CHECK((argb         & 0xFF) == 30); // B
    }
}

TEST_CASE("Theme edge: Light and Dark are different") {
    Theme light = Theme::Light();
    Theme dark = Theme::Dark();

    CHECK(light.bgPrimary != dark.bgPrimary);
    CHECK(light.textPrimary != dark.textPrimary);
    CHECK(light.accent != dark.accent);
}

TEST_CASE("Theme edge: setTheme to same value is no-op") {
    Theme t = currentTheme();
    // setTheme with a different theme to change state first
    setTheme(Theme::Dark());
    Theme afterDark = currentTheme();
    CHECK(afterDark.bgPrimary == Theme::Dark().bgPrimary);

    // Set again with same — should be no-op
    setTheme(Theme::Dark());
    Theme same = currentTheme();
    CHECK(same.bgPrimary == Theme::Dark().bgPrimary);

    // Restore
    setTheme(t);
}
