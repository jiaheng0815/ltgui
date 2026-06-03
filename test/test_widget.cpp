#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "widget.h"
#include "window.h"
#include "layout.h"
#include <memory>

using namespace ltgui;

TEST_CASE("Widget tree ownership") {
    SUBCASE("parent takes ownership via addChild") {
        auto parent = std::make_unique<Widget>();
        auto child = std::make_unique<Widget>();
        Widget* raw = child.get();

        parent->addChild(std::move(child));
        CHECK(parent->children().size() == 1);
        CHECK(parent->children()[0].get() == raw);
        CHECK(raw->parent() == parent.get());
    }

    SUBCASE("removeChild returns ownership") {
        auto parent = std::make_unique<Widget>();
        auto child = std::make_unique<Widget>();
        Widget* raw = child.get();

        parent->addChild(std::move(child));
        auto released = parent->removeChild(raw);

        CHECK(released.get() == raw);
        CHECK(parent->children().empty());
        CHECK(raw->parent() == nullptr);
    }

    SUBCASE("addChild with parent already set reparents") {
        auto parent1 = std::make_unique<Widget>();
        auto parent2 = std::make_unique<Widget>();
        auto child = std::make_unique<Widget>();
        Widget* raw = child.get();

        parent1->addChild(std::move(child));
        // Now move to parent2
        parent2->addChild(parent1->removeChild(raw));

        CHECK(parent1->children().empty());
        CHECK(parent2->children().size() == 1);
        CHECK(raw->parent() == parent2.get());
    }

    SUBCASE("null child is safely ignored") {
        auto parent = std::make_unique<Widget>();
        CHECK(parent->addChild(nullptr) == nullptr);
        CHECK(parent->children().empty());
    }

    SUBCASE("removeChild with unknown child returns nullptr") {
        auto parent = std::make_unique<Widget>();
        Widget orphan;
        CHECK(parent->removeChild(&orphan) == nullptr);
    }

    SUBCASE("parent destruction destroys children") {
        // Use a flag to verify child destructor runs
        static int dtorCount = 0;
        struct TestWidget : Widget {
            ~TestWidget() override { dtorCount++; }
        };

        dtorCount = 0;
        {
            auto parent = std::make_unique<Widget>();
            parent->addChild(std::make_unique<TestWidget>());
            parent->addChild(std::make_unique<TestWidget>());
            CHECK(dtorCount == 0);
        }
        CHECK(dtorCount == 2);
    }

    SUBCASE("makeChild returns raw pointer and registers") {
        auto parent = std::make_unique<Widget>();
        Widget* raw = parent->makeChild<Widget>();

        CHECK(raw != nullptr);
        CHECK(parent->children().size() == 1);
        CHECK(raw->parent() == parent.get());
    }
}

TEST_CASE("Widget tree ordering") {
    SUBCASE("raiseToTop moves child to end") {
        auto parent = std::make_unique<Widget>();
        auto* a = parent->makeChild<Widget>();
        auto* b = parent->makeChild<Widget>();
        auto* c = parent->makeChild<Widget>();

        CHECK(parent->children()[0].get() == a);
        CHECK(parent->children()[2].get() == c);

        a->raiseToTop();
        CHECK(parent->children()[0].get() == b);
        CHECK(parent->children()[1].get() == c);
        CHECK(parent->children()[2].get() == a);
    }

    SUBCASE("childAt returns correct pointer") {
        auto parent = std::make_unique<Widget>();
        auto* a = parent->makeChild<Widget>();
        auto* b = parent->makeChild<Widget>();

        CHECK(parent->childAt(0) == a);
        CHECK(parent->childAt(1) == b);
        CHECK(parent->childAt(-1) == nullptr);
        CHECK(parent->childAt(99) == nullptr);
    }
}

TEST_CASE("Layout ownership") {
    SUBCASE("setLayout takes ownership") {
        auto widget = std::make_unique<Widget>();
        auto layout = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 4, 4);

        widget->setLayout(std::move(layout));
        CHECK(widget->layout() != nullptr);
    }

    SUBCASE("replacing layout destroys old one") {
        auto widget = std::make_unique<Widget>();
        widget->setLayout(std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 4, 4));
        auto* first = widget->layout();
        widget->setLayout(std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 8, 8));
        CHECK(widget->layout() != first);
    }
}
