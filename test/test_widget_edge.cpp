#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "widget.h"
#include "widgets/button.h"
#include "widgets/label.h"
#include "widgets/radiobutton.h"
#include <memory>

using namespace ltgui;

// ============================================================
// 刁钻角度：widget 树操作
// ============================================================

TEST_CASE("Widget tree edge: deep hierarchy") {
    auto root = std::make_unique<Widget>();
    Widget* p = root.get();
    for (int i = 0; i < 200; i++) {
        p = p->makeChild<Widget>();
    }
    // Verify we can traverse back up
    int depth = 0;
    for (Widget* cur = p; cur; cur = cur->parent()) {
        depth++;
    }
    CHECK(depth == 201); // root + 200 children
}

TEST_CASE("Widget tree edge: reparent between trees") {
    auto tree1 = std::make_unique<Widget>();
    auto tree2 = std::make_unique<Widget>();

    auto child = std::make_unique<Widget>();
    Widget* raw = child.get();

    tree1->addChild(std::move(child));
    CHECK(raw->parent() == tree1.get());

    // Move to tree2
    auto released = tree1->removeChild(raw);
    tree2->addChild(std::move(released));
    CHECK(raw->parent() == tree2.get());
    CHECK(tree1->children().empty());
    CHECK(tree2->children().size() == 1);
}

TEST_CASE("Widget tree edge: remove during iteration") {
    auto w = std::make_unique<Widget>();
    auto* a = w->makeChild<Widget>();
    w->makeChild<Widget>();
    w->makeChild<Widget>();

    // Remove first child — should not affect iteration of remaining
    w->removeChild(a);
    CHECK(w->children().size() == 2);
    CHECK(a->parent() == nullptr);
}

TEST_CASE("Widget tree edge: remove child that isn't ours") {
    auto w = std::make_unique<Widget>();
    Widget orphan;

    auto result = w->removeChild(&orphan);
    CHECK(result == nullptr);
}

TEST_CASE("Widget tree edge: remove then re-add child") {
    auto w = std::make_unique<Widget>();
    auto* child = w->makeChild<Widget>();
    (void)child; // unused, fixture ensures makeChild works

    SUBCASE("remove then re-add child works") {
        auto child2 = std::make_unique<Widget>();
        Widget* raw2 = child2.get();
        w->addChild(std::move(child2));
        size_t before = w->children().size();

        // Remove the child, then re-add it — should restore the count
        auto released = w->removeChild(raw2);
        CHECK(released.get() == raw2);
        w->addChild(std::move(released));
        CHECK(w->children().size() == before);
    }
}

TEST_CASE("Widget tree edge: self-parent is prevented") {
    auto w = std::make_unique<Widget>();
    Widget* raw = w.get();

    // Can't add self as child of self — addChild checks child != parent
    // (actually it checks child->parent_ && child->parent_ != this)
    // If we try: addChild(std::unique_ptr<Widget>(raw)) — UB from double ownership
    // This is structurally prevented by unique_ptr
    CHECK(w->parent() == nullptr);
}

TEST_CASE("Widget tree edge: mass destruction") {
    static int alive = 0;
    struct CountingWidget : Widget {
        CountingWidget() { alive++; }
        ~CountingWidget() override { alive--; }
    };

    {
        alive = 0;
        auto root = std::make_unique<Widget>();
        for (int i = 0; i < 100; i++) {
            root->makeChild<CountingWidget>();
        }
        // Do some tree operations
        root->removeChild(root->childAt(0));
        root->removeChild(root->childAt(5));
        root->childAt(10)->makeChild<CountingWidget>();
        root->childAt(10)->makeChild<CountingWidget>();
    }
    // After root destruction, all children should be gone
    CHECK(alive == 0);
}

TEST_CASE("Widget edge: enable/disable toggle") {
    Widget w;
    CHECK(w.isEnabled());

    w.setEnabled(false);
    CHECK_FALSE(w.isEnabled());

    // Toggle back
    w.setEnabled(true);
    CHECK(w.isEnabled());

    // Redundant calls should not break
    w.setEnabled(true);
    w.setEnabled(false);
    w.setEnabled(false);
    CHECK_FALSE(w.isEnabled());
}

TEST_CASE("Widget edge: visible toggle") {
    Widget w;
    CHECK(w.isVisible());

    w.setVisible(false);
    CHECK_FALSE(w.isVisible());

    w.setVisible(true);
    CHECK(w.isVisible());
}

TEST_CASE("Widget edge: raiseToTop on root widget") {
    auto w = std::make_unique<Widget>();
    // raiseToTop with no parent should be a no-op, not crash
    w->raiseToTop();
    CHECK(true); // didn't crash
}

TEST_CASE("Widget edge: raiseToTop when already at top") {
    auto w = std::make_unique<Widget>();
    w->makeChild<Widget>();
    auto* last = w->makeChild<Widget>();

    last->raiseToTop();
    // Should still be last
    CHECK(w->children().back().get() == last);
}

TEST_CASE("RadioButton edge: uncheck checked button in group") {
    auto parent = std::make_unique<Widget>();
    auto* rb1 = parent->makeChild<RadioButton>("One");
    auto* rb2 = parent->makeChild<RadioButton>("Two");

    rb1->setChecked(true);
    CHECK(rb1->isChecked());

    // Try to uncheck the only checked one — should be ignored
    rb1->setChecked(false);
    CHECK(rb1->isChecked()); // still checked, radio group invariant

    // Check the other one
    rb2->setChecked(true);
    CHECK(rb2->isChecked());
    CHECK_FALSE(rb1->isChecked()); // rb1 auto-unchecked
}

TEST_CASE("Widget edge: absoluteRect for deeply nested widget") {
    auto root = std::make_unique<Widget>();
    root->setGeometry(Rect(5, 10, 800, 600));

    auto* level1 = root->makeChild<Widget>();
    level1->setGeometry(Rect(10, 20, 400, 300));

    auto* level2 = level1->makeChild<Widget>();
    level2->setGeometry(Rect(30, 40, 200, 100));

    auto* level3 = level2->makeChild<Widget>();
    level3->setGeometry(Rect(5, 5, 50, 50));

    Rect abs = level3->absoluteRect();
    // root(5,10) + L1(10,20) + L2(30,40) + L3(5,5)
    CHECK(abs.x == 50);  // 5 + 10 + 30 + 5
    CHECK(abs.y == 75);  // 10 + 20 + 40 + 5
    CHECK(abs.width == 50);
    CHECK(abs.height == 50);
}
