#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "style.h"
#include "theme.h"

using namespace ltgui;

TEST_CASE("Style default") {
    Style s = Style::defaultStyle();

    SUBCASE("font is set") {
        CHECK_FALSE(s.font.family.empty());
    }
    SUBCASE("has sensible defaults") {
        CHECK(s.borderWidth > 0);
        CHECK(s.borderRadius > 0);
        // Colors are left transparent so resolve() falls back to the
        // current theme (theme switches are picked up automatically).
        CHECK(s.bgColor == Color::Transparent);
        CHECK(s.fgColor == Color::Transparent);
    }
}

TEST_CASE("Style padding") {
    Style s;

    SUBCASE("zero by default") {
        CHECK(s.paddingLeft == 0);
        CHECK(s.paddingTop == 0);
        CHECK(s.paddingRight == 0);
        CHECK(s.paddingBottom == 0);
    }
    SUBCASE("setPadding all") {
        s.setPadding(8);
        CHECK(s.paddingLeft == 8);
        CHECK(s.paddingTop == 8);
        CHECK(s.paddingRight == 8);
        CHECK(s.paddingBottom == 8);
    }
    SUBCASE("setPadding h,v") {
        s.setPadding(10, 5);
        CHECK(s.paddingLeft == 10);
        CHECK(s.paddingRight == 10);
        CHECK(s.paddingTop == 5);
        CHECK(s.paddingBottom == 5);
    }
    SUBCASE("paddingHorz") {
        s.setPadding(10, 5);
        CHECK(s.paddingHorz() == 20);
    }
    SUBCASE("paddingVert") {
        s.setPadding(10, 5);
        CHECK(s.paddingVert() == 10);
    }
}


TEST_CASE("Style resolve priority") {
    Theme theme = Theme::Light();
    Style s;
    s.bgColor = Color(10, 20, 30);

    SUBCASE("base style wins over theme") {
        auto r = s.resolve(WidgetState::Normal, theme);
        CHECK(r.bgColor == Color(10, 20, 30));
    }

    SUBCASE("transparent fields fall back to theme") {
        s.fgColor = Color::Transparent;
        s.borderColor = Color::Transparent;
        s.accent = Color::Transparent;
        auto r = s.resolve(WidgetState::Normal, theme);
        CHECK(r.fgColor == theme.textPrimary);
        CHECK(r.borderColor == theme.border);
        CHECK(r.accent == theme.accent);
    }

    SUBCASE("state patch wins over base style") {
        s.hovered.bgColor = Color(99, 88, 77);
        auto r = s.resolve(WidgetState::Hovered, theme);
        CHECK(r.bgColor == Color(99, 88, 77));
    }

    SUBCASE("hover patch does not affect normal state") {
        s.hovered.bgColor = Color(99, 88, 77);
        auto r = s.resolve(WidgetState::Normal, theme);
        CHECK(r.bgColor == Color(10, 20, 30));
    }

    SUBCASE("disabled state uses its own patch") {
        s.disabled.fgColor = Color(1, 1, 1);
        auto r = s.resolve(WidgetState::Disabled, theme);
        CHECK(r.fgColor == Color(1, 1, 1));
    }

    SUBCASE("patch color overrides theme fallback") {
        s.focused.borderColor = Color(200, 0, 0);
        auto r = s.resolve(WidgetState::Focused, theme);
        CHECK(r.borderColor == Color(200, 0, 0));
    }
}

TEST_CASE("Style fromTheme") {
    Theme dark = Theme::Dark();
    Style s = Style::fromTheme(dark);
    // Colors are left transparent so resolve() falls back to the current
    // theme (theme switches are picked up automatically).
    CHECK(s.bgColor == Color::Transparent);
    CHECK(s.fgColor == Color::Transparent);
    CHECK(s.accent == Color::Transparent);
    CHECK(s.borderWidth > 0);
    CHECK(s.borderRadius > 0);
    CHECK_FALSE(s.font.family.empty());
}

TEST_CASE("Color lerp") {
    SUBCASE("endpoints") {
        CHECK(Color::lerp(Color::Black, Color::White, 0.0f) == Color::Black);
        CHECK(Color::lerp(Color::Black, Color::White, 1.0f) == Color::White);
    }
    SUBCASE("midpoint") {
        Color m = Color::lerp(Color(0, 0, 0), Color(100, 200, 255), 0.5f);
        CHECK(m.r == 50);
        CHECK(m.g == 100);
        CHECK(m.b == 127); // 127.5 truncates toward zero
    }
    SUBCASE("clamps t outside [0,1]") {
        CHECK(Color::lerp(Color::Black, Color::White, -1.0f) == Color::Black);
        CHECK(Color::lerp(Color::Black, Color::White, 2.0f) == Color::White);
    }
    SUBCASE("alpha interpolates") {
        Color a(255, 0, 0, 0), b(255, 0, 0, 255);
        CHECK(Color::lerp(a, b, 0.5f).a == 127);
    }
}
