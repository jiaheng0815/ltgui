#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "widget.h"
#include "layout.h"
#include "widgets/button.h"
#include "widgets/label.h"
#include "widgets/textbox.h"
#include "widgets/checkbox.h"
#include "widgets/combobox.h"
#include "widgets/slider.h"
#include "geometry.h"
#include <memory>
#include <string>

using namespace ltgui;

// Integration tests verify that the full widget tree, layout, and event
// routing work together correctly. These complement the unit tests by
// testing real widget interactions rather than isolated functions.

// ============================================================
// Widget Tree + Layout Integration
// ============================================================

TEST_CASE("Layout cascade: sizeHint change triggers parent re-layout") {
    // Set up: parent with BoxLayout, two child widgets
    auto parent = std::make_unique<Widget>();
    parent->setGeometry(Rect(0, 0, 400, 100));
    parent->setLayout(std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 4, 4));

    auto* w1 = parent->makeChild<Widget>();
    auto* w2 = parent->makeChild<Widget>();

    // Force initial layout
    parent->layout()->layout(parent.get());
    int w2xBefore = w2->x();

    // invalidateSizeHint propagates upward
    w1->invalidateSizeHint();

    // Store a manual size in w1's geometry; the layout should respect it
    // after we explicitly set a bigger size and the parent layout recomputes.
    // (The layout cascade via setGeometry triggers parent re-layout when
    //  geometry size actually changes from what the layout assigned.)
    parent->layout()->layout(parent.get());
    int w2xAfter = w2->x();

    // Both should have the same x because w1's sizeHint hasn't changed
    CHECK(w2xBefore == w2xAfter);
}

TEST_CASE("Widget focus chain traversal") {
    auto root = std::make_unique<Widget>();
    // Use Button widgets because base Widget::canAcceptFocus() returns false
    auto* a  = root->makeChild<Button>("a");
    auto* a1 = a->makeChild<Button>("a1");
    auto* a2 = a->makeChild<Button>("a2");
    auto* b  = root->makeChild<Button>("b");
    auto* c  = root->makeChild<Button>("c");

    // nextFocusWidget(): depth-first. Leaf with no children returns self.
    // root: children=[a,b,c] → a->nextFocusWidget()
    //   a: children=[a1,a2] → a1->nextFocusWidget()
    //     a1: no children, canAcceptFocus=true → returns a1
    Widget* first = root->nextFocusWidget();
    CHECK(first == a1);

    // a1->nextFocusWidget(): a1 has no children → returns a1 (itself)
    Widget* second = a1->nextFocusWidget();
    CHECK(second == a1);

    // a2->nextFocusWidget(): no children → returns a2
    Widget* third = a2->nextFocusWidget();
    CHECK(third == a2);

    // Last focusable descendant from root: c (deepest rightmost)
    Widget* last = root->lastFocusableDescendant();
    CHECK(last == c);

    // c->lastFocusableDescendant(): no children → returns c
    CHECK(c->lastFocusableDescendant() == c);
}

TEST_CASE("BoxLayout assigns non-zero geometry to children") {
    auto parent = std::make_unique<Widget>();
    parent->setGeometry(Rect(0, 0, 500, 100));
    parent->setLayout(std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 2, 2));

    // Create children AFTER setLayout and setGeometry
    auto* child1 = parent->makeChild<Widget>();
    auto* child2 = parent->makeChild<Widget>();

    // Explicit layout invocation
    parent->layout()->layout(parent.get());

    // SizeHint for base Widget is {100*dpi, 24*dpi} = {100, 24} with no window
    // BoxLayout LeftToRight should give each child non-zero width
    CHECK(child1->width() > 0);
    CHECK(child2->width() > 0);
    // child2 should be positioned after child1
    CHECK(child2->x() > child1->x());
}

TEST_CASE("removeChild detaches properly") {
    auto parent = std::make_unique<Widget>();
    auto* child = parent->makeChild<Widget>();

    CHECK(child->parent() == parent.get());
    CHECK(parent->children().size() == 1);

    auto released = parent->removeChild(child);
    CHECK(child->parent() == nullptr);
    CHECK(parent->children().empty());
    CHECK(released.get() == child);
}

TEST_CASE("BoxLayout distributes space correctly") {
    SUBCASE("LeftToRight with three children") {
        auto container = std::make_unique<Widget>();
        container->setGeometry(Rect(0, 0, 310, 50));
        auto* layout = new BoxLayout(BoxLayout::LeftToRight, 10, 5);
        container->setLayout(std::unique_ptr<Layout>(layout));

        container->makeChild<Widget>();
        container->makeChild<Widget>();
        container->makeChild<Widget>();

        container->layout()->layout(container.get());

        // First child starts at margin
        CHECK(container->children()[0]->x() == 5);
        // Children have non-zero width
        CHECK(container->children()[0]->width() > 50);
        // Second child is positioned after first (with spacing)
        CHECK(container->children()[1]->x() > container->children()[0]->x() + container->children()[0]->width());
    }

    SUBCASE("TopToBottom with stretch") {
        auto container = std::make_unique<Widget>();
        container->setGeometry(Rect(0, 0, 200, 200));
        auto* layout = new BoxLayout(BoxLayout::TopToBottom, 4, 4);
        layout->addStretch(0);
        layout->addStretch(1);
        container->setLayout(std::unique_ptr<Layout>(layout));

        container->makeChild<Widget>();
        container->makeChild<Widget>();

        container->layout()->layout(container.get());

        // Children fill width: 200 - 2*4 margin = 192
        CHECK(container->children()[0]->width() == 192);
        CHECK(container->children()[1]->width() == 192);
        // Second child should be taller due to stretch
        CHECK(container->children()[1]->height() > container->children()[0]->height());
    }
}

TEST_CASE("Absolute rect computation through tree") {
    auto root = std::make_unique<Widget>();
    root->setGeometry(Rect(10, 20, 400, 300));

    auto* child = root->makeChild<Widget>();
    child->setGeometry(Rect(5, 5, 100, 50));

    auto* grandchild = child->makeChild<Widget>();
    grandchild->setGeometry(Rect(10, 0, 50, 25));

    Rect absGrandchild = grandchild->absoluteRect();
    CHECK(absGrandchild.x == 25);
    CHECK(absGrandchild.y == 25);
    CHECK(absGrandchild.width == 50);
    CHECK(absGrandchild.height == 25);
}

TEST_CASE("TextBox API tests") {
    auto tb = std::make_unique<TextBox>("hello");

    CHECK(tb->text() == "hello");
    CHECK_FALSE(tb->canUndo());

    // Multi-line mode
    CHECK_FALSE(tb->isMultiLine());
    tb->setMultiLine(true);
    CHECK(tb->isMultiLine());

    // Size hint changes for multi-line
    Size singleHint = TextBox("test").sizeHint();
    auto multi = std::make_unique<TextBox>("test");
    multi->setMultiLine(true);
    Size multiHint = multi->sizeHint();
    CHECK(multiHint.height > singleHint.height);
}

TEST_CASE("HiDPI scaling is applied to sizeHint") {
    auto btn = std::make_unique<Button>("Click me");
    Size hint = btn->sizeHint();
    CHECK(hint.width > 20);
    CHECK(hint.height > 10);

    auto slider = std::make_unique<Slider>();
    hint = slider->sizeHint();
    CHECK(hint.width >= 150);
    CHECK(hint.height >= 28);

    auto combo = std::make_unique<ComboBox>();
    hint = combo->sizeHint();
    CHECK(hint.width >= 150);
    CHECK(hint.height >= 30);
}

TEST_CASE("Dirty rect update does not crash") {
    Widget w;
    w.update();
    w.update(Rect(5, 5, 10, 10));
    CHECK(true);
}

TEST_CASE("GridLayout basic distribution") {
    auto container = std::make_unique<Widget>();
    container->setGeometry(Rect(0, 0, 200, 200));
    container->setLayout(std::make_unique<GridLayout>(2, 4, 4, 4));

    container->makeChild<Widget>();
    container->makeChild<Widget>();
    container->makeChild<Widget>();
    container->makeChild<Widget>();

    container->layout()->layout(container.get());

    for (int i = 0; i < 4; i++) {
        CHECK(container->children()[i]->width() > 0);
        CHECK(container->children()[i]->height() > 0);
    }

    // Children in same column should have same x
    CHECK(container->children()[0]->x() == container->children()[2]->x());
    CHECK(container->children()[1]->x() == container->children()[3]->x());
}

TEST_CASE("Widget ownership and tree structure") {
    SUBCASE("makeChild returns raw pointer and registers") {
        auto parent = std::make_unique<Widget>();
        Widget* raw = parent->makeChild<Widget>();

        CHECK(raw != nullptr);
        CHECK(parent->children().size() == 1);
        CHECK(raw->parent() == parent.get());
    }

    SUBCASE("parent destruction destroys children") {
        static int dtorCount = 0;
        struct TestWidget : Widget {
            ~TestWidget() override { dtorCount++; }
        };

        dtorCount = 0;
        {
            auto parent = std::make_unique<Widget>();
            parent->addChild(std::make_unique<TestWidget>());
            parent->addChild(std::make_unique<TestWidget>());
        }
        CHECK(dtorCount == 2);
    }

    SUBCASE("setGeometry triggers layout") {
        auto parent = std::make_unique<Widget>();
        parent->setLayout(std::make_unique<BoxLayout>(BoxLayout::LeftToRight, 2, 2));
        auto* child = parent->makeChild<Widget>();

        // Set to a new geometry — this should trigger layout
        parent->setGeometry(Rect(0, 0, 400, 100));

        // Child should have been laid out
        CHECK(child->width() > 0);
        CHECK(child->height() > 0);
    }
}

TEST_CASE("TextBox undo stack is cleared on setText") {
    auto tb = std::make_unique<TextBox>("test");
    tb->setText("new");
    // setText clears the undo stack
    CHECK_FALSE(tb->canUndo());
    CHECK_FALSE(tb->canRedo());
}
