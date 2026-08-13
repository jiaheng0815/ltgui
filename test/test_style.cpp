#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "style.h"

using namespace ltgui;

TEST_CASE("Style default") {
    Style s = Style::defaultStyle();

    SUBCASE("font is set") {
        CHECK_FALSE(s.font.family.empty());
    }
    SUBCASE("has sensible defaults") {
        CHECK(s.borderWidth >= 0);
        CHECK(s.borderRadius >= 0);
        CHECK(s.bgColor.a > 0);
        CHECK(s.fgColor.a > 0);
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

