#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "ltgui.h"

// Windows headers define MessageBox -> MessageBoxW; undefine so the ltgui
// class name resolves.
#ifdef MessageBox
#undef MessageBox
#endif

using namespace ltgui;

// Headless Dialog tests — dialog behavior that does not require a real
// window (widgetType discrimination, result signal, title/state).

TEST_CASE("Dialog result signal") {
  Dialog dlg;
  int lastResult = -1;
  dlg.onFinished.connect(
      [&](DialogResult r) { lastResult = static_cast<int>(r); });

  SUBCASE("done(OK) emits onFinished") {
    dlg.done(DialogResult::OK);
    CHECK(lastResult == static_cast<int>(DialogResult::OK));
  }

  SUBCASE("done(Cancel) emits onFinished") {
    dlg.done(DialogResult::Cancel);
    CHECK(lastResult == static_cast<int>(DialogResult::Cancel));
  }
}

TEST_CASE("Dialog result() reflects done()") {
  Dialog dlg;
  CHECK(dlg.result() == DialogResult::None);
  dlg.done(DialogResult::Yes);
  CHECK(dlg.result() == DialogResult::Yes);
}

TEST_CASE("Dialog types are distinguishable via widgetType") {
  Dialog dlg;
  ltgui::MessageBox mb;
  InputDialog id;
  FileDialog fd;

  CHECK(dlg.widgetType() == WidgetType::Dialog);
  CHECK(mb.widgetType() == WidgetType::MessageBox);
  CHECK(id.widgetType() == WidgetType::InputDialog);
  CHECK(fd.widgetType() == WidgetType::FileDialog);

  // Dialogs are focusable for modal input.
  CHECK(dlg.canAcceptFocus());
  CHECK(mb.canAcceptFocus());
  CHECK(id.canAcceptFocus());
}

TEST_CASE("MessageBox setup") {
  ltgui::MessageBox mb;
  mb.setTitle("Confirm");
  mb.setMessage("Are you sure?");
  mb.setButtons(static_cast<int>(DialogButton::OK) |
                static_cast<int>(DialogButton::Cancel));
  // No crash and sane default title behavior — title is stored via setTitle.
  CHECK(mb.result() == DialogResult::None);
}

TEST_CASE("InputDialog setup") {
  InputDialog id;
  id.setLabel("Name:");
  id.setText("default");
  CHECK(id.text() == "default");
}
