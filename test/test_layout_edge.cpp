#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "widget.h"
#include "layout.h"
#include <memory>

using namespace ltgui;

// ============================================================
// 刁钻角度：layout 引擎的边界条件
// ============================================================

TEST_CASE("BoxLayout edge: zero and negative spacing") {
    SUBCASE("zero spacing produces no gap") {
        auto w = std::make_unique<Widget>();
        w->setLayout(std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 0, 0));
        w->makeChild<Widget>();
        w->makeChild<Widget>();
        w->setGeometry(Rect(0, 0, 500, 100));

        auto* a = w->childAt(0);
        auto* b = w->childAt(1);
        CHECK(a->x() == 0);
        CHECK(b->x() == a->width()); // tight, no gap
    }

    SUBCASE("negative spacing (should not crash)") {
        auto w = std::make_unique<Widget>();
        // Negative spacing is weird but shouldn't crash
        w->setLayout(std::make_unique<BoxLayout>(BoxLayout::LeftToRight, -10, 0));
        w->makeChild<Widget>();
        w->makeChild<Widget>();
        w->setGeometry(Rect(0, 0, 500, 100));
        // Just verify no crash; positions will overlap but that's fine
        CHECK(w->childAt(0) != nullptr);
        CHECK(w->childAt(1) != nullptr);
    }

    SUBCASE("negative margin (should not crash)") {
        auto w = std::make_unique<Widget>();
        w->setLayout(std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 4, -5));
        w->makeChild<Widget>();
        w->setGeometry(Rect(0, 0, 200, 50));
        // Negative margin: children extend beyond container, that's "fine"
        CHECK(w->childAt(0) != nullptr);
    }
}

TEST_CASE("BoxLayout edge: container smaller than children") {
    SUBCASE("available size is zero") {
        auto w = std::make_unique<Widget>();
        w->setLayout(std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 4, 4));
        w->makeChild<Widget>();
        w->setGeometry(Rect(0, 0, 0, 0));
        // Should not crash; children may get negative sizes
        CHECK(w->childAt(0) != nullptr);
    }

    SUBCASE("very large child count") {
        auto w = std::make_unique<Widget>();
        w->setLayout(std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 2, 2));
        for (int i = 0; i < 50; i++) {
            w->makeChild<Widget>();
        }
        w->setGeometry(Rect(0, 0, 10000, 200));
        Size sz = w->sizeHint();
        // Should handle many children without overflow
        CHECK(sz.width > 0);
        CHECK(sz.height > 0);
    }
}

TEST_CASE("BoxLayout edge: stretch factors") {
    SUBCASE("zero total stretch") {
        auto w = std::make_unique<Widget>();
        auto* layout = new BoxLayout(BoxLayout::LeftToRight, 0, 0);
        layout->addStretch(0); // zero stretch
        w->setLayout(std::unique_ptr<Layout>(layout));
        w->makeChild<Widget>();
        w->setGeometry(Rect(0, 0, 500, 50));
        // With zero total stretch, remaining space is not distributed
        auto* c = w->childAt(0);
        CHECK(c->width() == 100); // just its hint width
    }

    SUBCASE("stretch index beyond stretch list") {
        auto w = std::make_unique<Widget>();
        auto* layout = new BoxLayout(BoxLayout::LeftToRight, 0, 0);
        layout->addStretch(1); // only first child has stretch
        w->setLayout(std::unique_ptr<Layout>(layout));
        w->makeChild<Widget>();
        w->makeChild<Widget>(); // no stretch factor for this one
        w->setGeometry(Rect(0, 0, 500, 50));
        // Second child should get hint width only
        auto* b = w->childAt(1);
        CHECK(b->width() == 100);
    }
}

TEST_CASE("BoxLayout edge: single visible child among invisible") {
    SUBCASE("only invisible children") {
        auto w = std::make_unique<Widget>();
        w->setLayout(std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 4, 4));
        auto* c = w->makeChild<Widget>();
        c->setVisible(false);
        w->setGeometry(Rect(0, 0, 200, 50));
        // All invisible → visibleCount == 0 → layout early-returns
        // Should not crash
        Size sz = w->sizeHint();
        CHECK(sz.width == 8); // 2 * margin
    }
}

TEST_CASE("GridLayout edge: single column") {
    auto w = std::make_unique<Widget>();
    w->setLayout(std::make_unique<GridLayout>(1, 4, 4, 4)); // 1 column
    w->makeChild<Widget>();
    w->makeChild<Widget>();
    w->makeChild<Widget>();
    w->setGeometry(Rect(0, 0, 300, 300));

    // 3 items, 1 column → 3 rows
    CHECK(w->childAt(0)->x() >= 0);
    CHECK(w->childAt(1)->y() > w->childAt(0)->y()); // row 2 below row 1
    CHECK(w->childAt(2)->y() > w->childAt(1)->y()); // row 3 below row 2
}

TEST_CASE("Layout: deeply nested") {
    auto root = std::make_unique<Widget>();
    root->setLayout(std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 0, 0));

    Widget* parent = root.get();
    for (int i = 0; i < 100; i++) {
        auto child = std::make_unique<Widget>();
        child->setLayout(std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 0, 0));
        Widget* raw = child.get();
        parent->addChild(std::move(child));
        parent = raw;
    }

    root->setGeometry(Rect(0, 0, 800, 600));
    Size sz = root->sizeHint();
    // Deep nesting should not crash or overflow. The calculated hint
    // depends on the default {100, 24} hint at the leaf — as long as
    // we don't crash/overflow, the test passes.
    CHECK(sz.width >= 0);
    CHECK(sz.height >= 0);
}

TEST_CASE("Layout: replace layout on the fly") {
    auto w = std::make_unique<Widget>();
    w->setLayout(std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 4, 4));
    w->makeChild<Widget>();
    Size sz1 = w->sizeHint();

    // Replace with different layout type
    w->setLayout(std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 8, 8));
    w->invalidateSizeHint();
    Size sz2 = w->sizeHint();

    // Dimensions flipped in meaning
    CHECK(sz1 != sz2);
}

TEST_CASE("Layout: empty widget with no layout uses default sizeHint") {
    Widget w;
    Size sz = w.sizeHint();
    // Default fallback when no layout: {100, 24}
    CHECK(sz.width == 100);
    CHECK(sz.height == 24);
}
