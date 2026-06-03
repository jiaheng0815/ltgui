#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "geometry.h"

using namespace ltgui;

TEST_CASE("Point") {
    SUBCASE("default construction") {
        Point p;
        CHECK(p.x == 0);
        CHECK(p.y == 0);
    }
    SUBCASE("parameterized construction") {
        Point p(3, 5);
        CHECK(p.x == 3);
        CHECK(p.y == 5);
    }
    SUBCASE("addition") {
        Point a(1, 2), b(3, 4);
        Point c = a + b;
        CHECK(c.x == 4);
        CHECK(c.y == 6);
    }
    SUBCASE("subtraction") {
        Point a(5, 8), b(2, 3);
        Point c = a - b;
        CHECK(c.x == 3);
        CHECK(c.y == 5);
    }
    SUBCASE("addition assignment") {
        Point a(1, 2);
        a += Point(3, 4);
        CHECK(a.x == 4);
        CHECK(a.y == 6);
    }
    SUBCASE("subtraction assignment") {
        Point a(5, 8);
        a -= Point(2, 3);
        CHECK(a.x == 3);
        CHECK(a.y == 5);
    }
    SUBCASE("equality") {
        CHECK(Point(1, 2) == Point(1, 2));
        CHECK_FALSE(Point(1, 2) == Point(3, 4));
    }
    SUBCASE("inequality") {
        CHECK(Point(1, 2) != Point(3, 4));
        CHECK_FALSE(Point(1, 2) != Point(1, 2));
    }
}

TEST_CASE("Size") {
    SUBCASE("default construction") {
        Size s;
        CHECK(s.width == 0);
        CHECK(s.height == 0);
    }
    SUBCASE("parameterized construction") {
        Size s(640, 480);
        CHECK(s.width == 640);
        CHECK(s.height == 480);
    }
    SUBCASE("isEmpty with positive dimensions") {
        CHECK_FALSE(Size(10, 10).isEmpty());
    }
    SUBCASE("isEmpty with zero width") {
        CHECK(Size(0, 10).isEmpty());
    }
    SUBCASE("isEmpty with zero height") {
        CHECK(Size(10, 0).isEmpty());
    }
    SUBCASE("isEmpty with negative dimensions") {
        CHECK(Size(-1, 10).isEmpty());
    }
    SUBCASE("equality") {
        CHECK(Size(1, 2) == Size(1, 2));
        CHECK_FALSE(Size(1, 2) == Size(3, 4));
    }
    SUBCASE("inequality") {
        CHECK(Size(1, 2) != Size(3, 4));
    }
    SUBCASE("addition") {
        Size c = Size(3, 4) + Size(1, 2);
        CHECK(c.width == 4);
        CHECK(c.height == 6);
    }
    SUBCASE("subtraction") {
        Size c = Size(5, 8) - Size(1, 2);
        CHECK(c.width == 4);
        CHECK(c.height == 6);
    }
}

TEST_CASE("Rect") {
    SUBCASE("default construction") {
        Rect r;
        CHECK(r.x == 0);
        CHECK(r.y == 0);
        CHECK(r.width == 0);
        CHECK(r.height == 0);
    }
    SUBCASE("parameterized construction") {
        Rect r(10, 20, 30, 40);
        CHECK(r.x == 10);
        CHECK(r.y == 20);
        CHECK(r.width == 30);
        CHECK(r.height == 40);
    }
    SUBCASE("Point+Size construction") {
        Rect r(Point(10, 20), Size(30, 40));
        CHECK(r.x == 10);
        CHECK(r.y == 20);
        CHECK(r.width == 30);
        CHECK(r.height == 40);
    }
    SUBCASE("accessors") {
        Rect r(10, 20, 30, 40);
        CHECK(r.left() == 10);
        CHECK(r.top() == 20);
        CHECK(r.right() == 40);
        CHECK(r.bottom() == 60);
    }
    SUBCASE("topLeft and bottomRight") {
        Rect r(10, 20, 30, 40);
        CHECK(r.topLeft() == Point(10, 20));
        CHECK(r.bottomRight() == Point(40, 60));
    }
    SUBCASE("center") {
        Rect r(0, 0, 100, 60);
        CHECK(r.center() == Point(50, 30));
    }
    SUBCASE("size") {
        Rect r(1, 2, 30, 40);
        CHECK(r.size() == Size(30, 40));
    }
    SUBCASE("contains point inside") {
        Rect r(10, 10, 20, 20);
        CHECK(r.contains(Point(15, 15)));
    }
    SUBCASE("contains point at origin") {
        Rect r(10, 10, 20, 20);
        CHECK(r.contains(Point(10, 10)));
    }
    SUBCASE("contains point on right edge") {
        Rect r(10, 10, 20, 20);
        CHECK_FALSE(r.contains(Point(30, 15)));
    }
    SUBCASE("contains point on bottom edge") {
        Rect r(10, 10, 20, 20);
        CHECK_FALSE(r.contains(Point(15, 30)));
    }
    SUBCASE("contains point outside") {
        Rect r(10, 10, 20, 20);
        CHECK_FALSE(r.contains(Point(5, 5)));
    }
    SUBCASE("contains rect fully inside") {
        Rect outer(0, 0, 100, 100);
        Rect inner(10, 10, 20, 20);
        CHECK(outer.contains(inner));
    }
    SUBCASE("contains rect partially overlapping") {
        Rect outer(0, 0, 100, 100);
        Rect inner(90, 90, 30, 30);
        CHECK_FALSE(outer.contains(inner));
    }
    SUBCASE("contains rect outside") {
        Rect outer(0, 0, 100, 100);
        Rect inner(200, 200, 20, 20);
        CHECK_FALSE(outer.contains(inner));
    }
    SUBCASE("intersects overlapping") {
        Rect a(0, 0, 50, 50), b(25, 25, 50, 50);
        CHECK(a.intersects(b));
    }
    SUBCASE("intersects non-overlapping") {
        Rect a(0, 0, 10, 10), b(100, 100, 10, 10);
        CHECK_FALSE(a.intersects(b));
    }
    SUBCASE("intersects edge-touching") {
        Rect a(0, 0, 10, 10), b(10, 0, 10, 10);
        CHECK_FALSE(a.intersects(b));
    }
    SUBCASE("intersected") {
        Rect a(0, 0, 50, 50), b(25, 25, 50, 50);
        Rect c = a.intersected(b);
        CHECK(c.x == 25);
        CHECK(c.y == 25);
        CHECK(c.width == 25);
        CHECK(c.height == 25);
    }
    SUBCASE("intersected non-overlapping returns empty") {
        Rect a(0, 0, 10, 10), b(100, 100, 10, 10);
        Rect c = a.intersected(b);
        CHECK(c.isEmpty());
    }
    SUBCASE("united") {
        Rect a(0, 0, 10, 10), b(5, 5, 15, 15);
        Rect u = a.united(b);
        CHECK(u.x == 0);
        CHECK(u.y == 0);
        CHECK(u.width == 20);
        CHECK(u.height == 20);
    }
    SUBCASE("isEmpty") {
        CHECK(Rect(0, 0, 0, 10).isEmpty());
        CHECK(Rect(0, 0, 10, 0).isEmpty());
        CHECK_FALSE(Rect(0, 0, 10, 10).isEmpty());
    }
    SUBCASE("equality") {
        CHECK(Rect(1, 2, 3, 4) == Rect(1, 2, 3, 4));
        CHECK_FALSE(Rect(1, 2, 3, 4) == Rect(5, 6, 7, 8));
    }
    SUBCASE("adjust") {
        Rect r(10, 10, 20, 20);
        r.adjust(1, 1, -1, -1);
        CHECK(r.x == 11);
        CHECK(r.y == 11);
        CHECK(r.width == 18);
        CHECK(r.height == 18);
    }
    SUBCASE("adjusted does not modify original") {
        Rect r(10, 10, 20, 20);
        Rect a = r.adjusted(1, 1, -1, -1);
        CHECK(r == Rect(10, 10, 20, 20));
        CHECK(a == Rect(11, 11, 18, 18));
    }
    SUBCASE("translated") {
        Rect r(10, 10, 20, 20);
        Rect t = r.translated(5, -3);
        CHECK(t.x == 15);
        CHECK(t.y == 7);
        CHECK(t.width == 20);
        CHECK(t.height == 20);
    }
}
