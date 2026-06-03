#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "widget.h"
#include "layout.h"
#include <memory>

using namespace ltgui;

TEST_CASE("BoxLayout preferredSize") {
    SUBCASE("empty container returns zero") {
        auto w = std::make_unique<Widget>();
        w->setLayout(std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 4, 4));
        Size sz = w->sizeHint();
        CHECK(sz.width == 8);  // 2 * margin
        CHECK(sz.height == 8);
    }

    SUBCASE("single child with known hint") {
        auto w = std::make_unique<Widget>();
        w->setLayout(std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 4, 4));
        auto* c = w->makeChild<Widget>();
        // Default sizeHint is {100, 24}
        Size sz = w->sizeHint();
        // width = child_w + 2*margin = 100 + 8 = 108
        // height = max(child_h, 0) + 2*margin = 24 + 8 = 32
        CHECK(sz.width == 108);
        CHECK(sz.height == 32);
    }

    SUBCASE("horizontal: two children, spacing between") {
        auto w = std::make_unique<Widget>();
        auto* layout = new BoxLayout(BoxLayout::LeftToRight, 10, 5);
        w->setLayout(std::unique_ptr<Layout>(layout));
        w->makeChild<Widget>(); // hint {100, 24}
        w->makeChild<Widget>(); // hint {100, 24}
        Size sz = w->sizeHint();
        // width = 100 + 10 + 100 + 2*5 = 220
        // height = max(24, 24) + 2*5 = 34
        CHECK(sz.width == 220);
        CHECK(sz.height == 34);
    }

    SUBCASE("vertical: two children, spacing between") {
        auto w = std::make_unique<Widget>();
        w->setLayout(std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 8, 6));
        w->makeChild<Widget>();
        w->makeChild<Widget>();
        Size sz = w->sizeHint();
        // width = max(100, 100) + 2*6 = 112
        // height = 24 + 8 + 24 + 2*6 = 68
        CHECK(sz.width == 112);
        CHECK(sz.height == 68);
    }

    SUBCASE("invisible children are ignored") {
        auto w = std::make_unique<Widget>();
        w->setLayout(std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 4, 4));
        auto* c = w->makeChild<Widget>();
        c->setVisible(false);
        Size sz = w->sizeHint();
        CHECK(sz.width == 8);   // 2 * margin only
        CHECK(sz.height == 8);
    }
}

TEST_CASE("BoxLayout layout positions") {
    SUBCASE("horizontal layout sets x positions") {
        auto w = std::make_unique<Widget>();
        auto* layout = new BoxLayout(BoxLayout::LeftToRight, 0, 0);
        w->setLayout(std::unique_ptr<Layout>(layout));
        auto* a = w->makeChild<Widget>();
        auto* b = w->makeChild<Widget>();
        w->setGeometry(Rect(0, 0, 300, 50));

        CHECK(a->x() == 0);
        CHECK(a->y() == 0);
        CHECK(a->width() == 100);   // hint width
        CHECK(a->height() == 50);   // fills container height

        CHECK(b->x() == 100);
        CHECK(b->y() == 0);
        CHECK(b->width() == 100);
        CHECK(b->height() == 50);
    }

    SUBCASE("horizontal with margin offsets children") {
        auto w = std::make_unique<Widget>();
        auto* layout = new BoxLayout(BoxLayout::LeftToRight, 4, 8);
        w->setLayout(std::unique_ptr<Layout>(layout));
        auto* a = w->makeChild<Widget>();
        w->setGeometry(Rect(0, 0, 200, 60));

        CHECK(a->x() == 8);
        CHECK(a->y() == 8);
    }

    SUBCASE("vertical layout sets y positions") {
        auto w = std::make_unique<Widget>();
        auto* layout = new BoxLayout(BoxLayout::TopToBottom, 0, 0);
        w->setLayout(std::unique_ptr<Layout>(layout));
        auto* a = w->makeChild<Widget>();
        auto* b = w->makeChild<Widget>();
        w->setGeometry(Rect(0, 0, 200, 100));

        CHECK(a->x() == 0);
        CHECK(a->y() == 0);
        CHECK(a->width() == 200);  // fills container width
        CHECK(a->height() == 24);  // hint height

        CHECK(b->x() == 0);
        CHECK(b->y() == 24);
    }

    SUBCASE("stretch factor distributes remaining space") {
        auto w = std::make_unique<Widget>();
        auto* layout = new BoxLayout(BoxLayout::LeftToRight, 0, 0);
        layout->addStretch(1);
        w->setLayout(std::unique_ptr<Layout>(layout));
        w->makeChild<Widget>(); // hint width = 100
        w->setGeometry(Rect(0, 0, 300, 50));

        auto* a = w->childAt(0);
        // total remaining = 300 - 100 = 200
        // stretch = 1, so child gets 100 + 200*1/1 = 300
        CHECK(a->width() == 300);
    }
}

TEST_CASE("GridLayout preferredSize") {
    SUBCASE("empty container") {
        auto w = std::make_unique<Widget>();
        w->setLayout(std::make_unique<GridLayout>(3, 4, 4, 4));
        Size sz = w->sizeHint();
        // GridLayout returns {0,0} when n==0 (unlike BoxLayout which returns 2*margin)
        CHECK(sz.width == 0);
        CHECK(sz.height == 0);
    }

    SUBCASE("single child in 3-column grid") {
        auto w = std::make_unique<Widget>();
        w->setLayout(std::make_unique<GridLayout>(3, 4, 4, 6));
        w->makeChild<Widget>(); // hint {100, 24}
        Size sz = w->sizeHint();
        // cols = 3, maxColW = 100. width = 3*100 + 2*4 + 2*6 = 320
        // rows = 1, maxRowH = 24.  height = 1*24  + 0*4 + 2*6 = 36
        CHECK(sz.width == 320);
        CHECK(sz.height == 36);
    }
}

TEST_CASE("Layout sizeHint caching") {
    SUBCASE("sizeHint is cached until invalidated") {
        auto w = std::make_unique<Widget>();
        w->setLayout(std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 4, 4));
        Size first = w->sizeHint();
        Size second = w->sizeHint();
        CHECK(first == second);
    }

    SUBCASE("adding child invalidates cache") {
        auto w = std::make_unique<Widget>();
        w->setLayout(std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 4, 4));
        Size before = w->sizeHint();
        w->makeChild<Widget>();
        // addChild now automatically invalidates parent sizeHint
        Size after = w->sizeHint();
        CHECK(before != after);
    }
}

TEST_CASE("sizeHint invalidation propagates upward") {
    auto parent = std::make_unique<Widget>();
    parent->setLayout(std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 0, 0));
    auto* child = parent->makeChild<Widget>();

    Size first = parent->sizeHint();
    // Force dirty — in a real widget, setText() etc. would call this.
    // The invalidation propagates to parent, forcing recomputation.
    child->invalidateSizeHint();
    Size second = parent->sizeHint();
    // Same value (child hint hasn't actually changed size), but the
    // cache was invalidated so it WAS recomputed via the layout.
    CHECK(first == second);
}
