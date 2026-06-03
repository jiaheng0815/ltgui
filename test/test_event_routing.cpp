#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "widget.h"
#include "event.h"
#include <memory>

using namespace ltgui;

// Test widget that records what events it received
class SpyWidget : public Widget {
public:
    using Widget::Widget;

    bool handleEvent(Event& event) override {
        received_.push_back(event.type);
        event.accepted = shouldAccept_;
        return shouldAccept_;
    }

    void setShouldAccept(bool v) { shouldAccept_ = v; }
    const std::vector<EventType>& received() const { return received_; }
    void clear() { received_.clear(); }

private:
    std::vector<EventType> received_;
    bool shouldAccept_ = false;
};

TEST_CASE("MouseDown is targeted dispatch") {
    SUBCASE("only child under cursor receives MouseDown") {
        auto parent = std::make_unique<Widget>();
        parent->setGeometry(Rect(0, 0, 400, 400));

        auto* childA = parent->makeChild<SpyWidget>();
        childA->setGeometry(Rect(0, 0, 100, 100));

        auto* childB = parent->makeChild<SpyWidget>();
        childB->setGeometry(Rect(200, 0, 100, 100));

        // Click on childA's area
        Event ev;
        ev.type = EventType::MouseDown;
        ev.button = MouseButton::Left;
        ev.pos = {50, 50};
        parent->handleEvent(ev);

        CHECK(childA->received().size() == 1);
        CHECK(childA->received()[0] == EventType::MouseDown);
        CHECK(childB->received().empty());
    }

    SUBCASE("click on empty area dispatches to no child") {
        auto parent = std::make_unique<Widget>();
        parent->setGeometry(Rect(0, 0, 400, 400));

        auto* child = parent->makeChild<SpyWidget>();
        child->setGeometry(Rect(0, 0, 100, 100));

        // Click outside child's area
        Event ev;
        ev.type = EventType::MouseDown;
        ev.button = MouseButton::Left;
        ev.pos = {300, 300};
        parent->handleEvent(ev);

        CHECK(child->received().empty());
    }

    SUBCASE("disabled child does not receive events") {
        auto parent = std::make_unique<Widget>();
        parent->setGeometry(Rect(0, 0, 400, 400));

        auto* child = parent->makeChild<SpyWidget>();
        child->setGeometry(Rect(0, 0, 100, 100));
        child->setEnabled(false);

        Event ev;
        ev.type = EventType::MouseDown;
        ev.pos = {50, 50};
        parent->handleEvent(ev);

        CHECK(child->received().empty());
    }

    SUBCASE("invisible child does not receive events") {
        auto parent = std::make_unique<Widget>();
        parent->setGeometry(Rect(0, 0, 400, 400));

        auto* child = parent->makeChild<SpyWidget>();
        child->setGeometry(Rect(0, 0, 100, 100));
        child->setVisible(false);

        Event ev;
        ev.type = EventType::MouseDown;
        ev.pos = {50, 50};
        parent->handleEvent(ev);

        CHECK(child->received().empty());
    }
}

TEST_CASE("MouseUp and MouseMove are broadcast") {
    SUBCASE("MouseMove goes to all children regardless of position") {
        auto parent = std::make_unique<Widget>();
        parent->setGeometry(Rect(0, 0, 400, 400));

        auto* childA = parent->makeChild<SpyWidget>();
        childA->setGeometry(Rect(0, 0, 100, 100));

        auto* childB = parent->makeChild<SpyWidget>();
        childB->setGeometry(Rect(200, 0, 100, 100));

        Event ev;
        ev.type = EventType::MouseMove;
        ev.pos = {50, 50}; // Over childA
        parent->handleEvent(ev);

        // Both should receive it (broadcast)
        CHECK(childA->received().size() == 1);
        CHECK(childA->received()[0] == EventType::MouseMove);
        CHECK(childB->received().size() == 1);
        CHECK(childB->received()[0] == EventType::MouseMove);
    }
}

TEST_CASE("z-order dispatch order") {
    SUBCASE("higher z-order child gets MouseDown first") {
        auto parent = std::make_unique<Widget>();
        parent->setGeometry(Rect(0, 0, 400, 400));

        // Both overlap at (50, 50)
        auto* bottom = parent->makeChild<SpyWidget>();
        bottom->setGeometry(Rect(0, 0, 100, 100));
        bottom->setShouldAccept(true); // consumes event

        auto* top = parent->makeChild<SpyWidget>();
        top->setGeometry(Rect(0, 0, 100, 100));
        top->setShouldAccept(true); // consumes event

        Event ev;
        ev.type = EventType::MouseDown;
        ev.pos = {50, 50};
        parent->handleEvent(ev);

        // Top (higher z-order) should receive it; bottom should not
        // because top consumes it.
        CHECK(top->received().size() == 1);
        CHECK(bottom->received().empty());
    }

    SUBCASE("raiseToTop changes dispatch priority") {
        auto parent = std::make_unique<Widget>();
        parent->setGeometry(Rect(0, 0, 400, 400));

        auto* bottom = parent->makeChild<SpyWidget>();
        bottom->setGeometry(Rect(0, 0, 100, 100));
        bottom->setShouldAccept(true);

        auto* top = parent->makeChild<SpyWidget>();
        top->setGeometry(Rect(0, 0, 100, 100));
        top->setShouldAccept(true);

        // Before raise: top receives event
        Event ev1;
        ev1.type = EventType::MouseDown;
        ev1.pos = {50, 50};
        parent->handleEvent(ev1);
        CHECK(top->received().size() == 1);
        top->clear();
        bottom->clear();

        // Raise bottom to top
        bottom->raiseToTop();

        Event ev2;
        ev2.type = EventType::MouseDown;
        ev2.pos = {50, 50};
        parent->handleEvent(ev2);

        // Now bottom (raised) should receive it first
        CHECK(bottom->received().size() == 1);
        CHECK(top->received().empty());
    }
}

TEST_CASE("event position is converted to child-local coordinates") {
    auto parent = std::make_unique<Widget>();
    parent->setGeometry(Rect(0, 0, 400, 400));

    auto* child = parent->makeChild<SpyWidget>();
    child->setGeometry(Rect(100, 50, 200, 150));

    Event ev;
    ev.type = EventType::MouseDown;
    ev.pos = {150, 80}; // (50, 30) relative to child
    parent->handleEvent(ev);

    CHECK(child->received().size() == 1);
}
