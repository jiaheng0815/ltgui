#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "color.h"

using namespace ltgui;

TEST_CASE("Color") {
    SUBCASE("default construction") {
        Color c;
        CHECK(c.r == 0);
        CHECK(c.g == 0);
        CHECK(c.b == 0);
        CHECK(c.a == 255);
    }
    SUBCASE("RGB construction") {
        Color c(10, 20, 30);
        CHECK(c.r == 10);
        CHECK(c.g == 20);
        CHECK(c.b == 30);
        CHECK(c.a == 255);
    }
    SUBCASE("RGBA construction") {
        Color c(10, 20, 30, 128);
        CHECK(c.a == 128);
    }
    SUBCASE("fromRGB") {
        Color c = Color::fromRGB(10, 20, 30);
        CHECK(c.r == 10);
        CHECK(c.g == 20);
        CHECK(c.b == 30);
        CHECK(c.a == 255);
    }
    SUBCASE("fromARGB") {
        Color c = Color::fromARGB(128, 10, 20, 30);
        CHECK(c.r == 10);
        CHECK(c.g == 20);
        CHECK(c.b == 30);
        CHECK(c.a == 128);
    }
    SUBCASE("toARGB bit pattern") {
        CHECK(Color(0, 0, 0, 255).toARGB() == 0xFF000000u);
        CHECK(Color(255, 0, 0, 255).toARGB() == 0xFFFF0000u);
        CHECK(Color(0, 255, 0, 255).toARGB() == 0xFF00FF00u);
        CHECK(Color(0, 0, 255, 255).toARGB() == 0xFF0000FFu);
        CHECK(Color(255, 255, 255, 255).toARGB() == 0xFFFFFFFFu);
    }
    SUBCASE("toARGB with alpha") {
        CHECK(Color(0, 255, 0, 128).toARGB() == 0x8000FF00u);
        CHECK(Color(0, 0, 0, 0).toARGB() == 0x00000000u);
    }
    SUBCASE("toABGR bit pattern (GPU vertex color)") {
        // GPU reads LE bytes as [R][G][B][A], so uint32 should be 0xAABBGGRR
        CHECK(Color(0, 0, 0, 255).toABGR() == 0xFF000000u);
        CHECK(Color(255, 0, 0, 255).toABGR() == 0xFF0000FFu);
        CHECK(Color(0, 255, 0, 255).toABGR() == 0xFF00FF00u);
        CHECK(Color(0, 0, 255, 255).toABGR() == 0xFFFF0000u);
        CHECK(Color(255, 255, 255, 255).toABGR() == 0xFFFFFFFFu);
    }
    SUBCASE("equality") {
        CHECK(Color(1, 2, 3, 4) == Color(1, 2, 3, 4));
        CHECK_FALSE(Color(1, 2, 3) == Color(5, 6, 7));
    }
    SUBCASE("inequality") {
        CHECK(Color(1, 2, 3) != Color(5, 6, 7));
    }
}

TEST_CASE("Color predefined constants") {
    SUBCASE("Transparent") { CHECK(Color::Transparent.a == 0); }
    SUBCASE("Black")       { CHECK(Color::Black.toARGB() == 0xFF000000u); }
    SUBCASE("White")       { CHECK(Color::White.toARGB() == 0xFFFFFFFFu); }
    SUBCASE("Red")         { CHECK(Color::Red.r == 255); CHECK(Color::Red.g == 0); CHECK(Color::Red.b == 0); }
    SUBCASE("Green")       { CHECK(Color::Green.r == 0); CHECK(Color::Green.g == 128); CHECK(Color::Green.b == 0); }
    SUBCASE("Blue")        { CHECK(Color::Blue.r == 0); CHECK(Color::Blue.g == 0); CHECK(Color::Blue.b == 255); }
    SUBCASE("Gray")        { CHECK(Color::Gray == Color(128, 128, 128)); }
    SUBCASE("constants exist") {
        CHECK(Color::Yellow != Color());
        CHECK(Color::Cyan != Color());
        CHECK(Color::Magenta != Color());
        CHECK(Color::LightGray != Color());
        CHECK(Color::DarkGray != Color());
        CHECK(Color::DarkBlue != Color());
        CHECK(Color::ButtonFace != Color());
        CHECK(Color::ButtonHighlight != Color());
        CHECK(Color::ButtonShadow != Color());
        CHECK(Color::WindowBg != Color());
        CHECK(Color::TextColor == Color(0, 0, 0));
    }
}
