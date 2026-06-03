#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "widget.h"
#include "widgets/button.h"
#include "widgets/label.h"
#include "widgets/checkbox.h"
#include "widgets/radiobutton.h"
#include "widgets/slider.h"
#include "widgets/textbox.h"
#include "widgets/combobox.h"
#include "widgets/listbox.h"
#include "widgets/progressbar.h"
#include "widgets/scrollarea.h"
#include "widgets/tabwidget.h"
#include "widgets/tooltip.h"
#include "widgets/treeview.h"
#include "widgets/contextmenu.h"
#include "widgets/image.h"
#include <memory>

using namespace ltgui;

TEST_CASE("WidgetType identification") {
    SUBCASE("base widget returns Base") {
        Widget w;
        CHECK(w.widgetType() == WidgetType::Base);
    }

    SUBCASE("Button") {
        Button b;
        CHECK(b.widgetType() == WidgetType::Button);
    }

    SUBCASE("Label") {
        Label l;
        CHECK(l.widgetType() == WidgetType::Label);
    }

    SUBCASE("CheckBox") {
        CheckBox cb;
        CHECK(cb.widgetType() == WidgetType::CheckBox);
    }

    SUBCASE("RadioButton") {
        RadioButton rb;
        CHECK(rb.widgetType() == WidgetType::RadioButton);
    }

    SUBCASE("Slider") {
        Slider s;
        CHECK(s.widgetType() == WidgetType::Slider);
    }

    SUBCASE("TextBox") {
        TextBox tb;
        CHECK(tb.widgetType() == WidgetType::TextBox);
    }

    SUBCASE("ComboBox") {
        ComboBox cb;
        CHECK(cb.widgetType() == WidgetType::ComboBox);
    }

    SUBCASE("ListBox") {
        ListBox lb;
        CHECK(lb.widgetType() == WidgetType::ListBox);
    }

    SUBCASE("ProgressBar") {
        ProgressBar pb;
        CHECK(pb.widgetType() == WidgetType::ProgressBar);
    }

    SUBCASE("ScrollArea") {
        ScrollArea sa;
        CHECK(sa.widgetType() == WidgetType::ScrollArea);
    }

    SUBCASE("TabWidget") {
        TabWidget tw;
        CHECK(tw.widgetType() == WidgetType::TabWidget);
    }

    SUBCASE("Tooltip") {
        Tooltip tt;
        CHECK(tt.widgetType() == WidgetType::Tooltip);
    }

    SUBCASE("TreeView") {
        TreeView tv;
        CHECK(tv.widgetType() == WidgetType::TreeView);
    }

    SUBCASE("ContextMenu") {
        ContextMenu cm;
        CHECK(cm.widgetType() == WidgetType::ContextMenu);
    }

    SUBCASE("Image") {
        Image img;
        CHECK(img.widgetType() == WidgetType::Image);
    }

    SUBCASE("RadioButton identified via base pointer") {
        auto rb = std::make_unique<RadioButton>();
        Widget* w = rb.get();
        CHECK(w->widgetType() == WidgetType::RadioButton);
    }
}
