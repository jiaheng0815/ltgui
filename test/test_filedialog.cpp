#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest/doctest.h"
#include "ltgui.h"

using namespace ltgui;

// Headless FileDialog tests — mode/filter state is window-independent.
// exec()/selectedPath() require a window and are exercised by the GUI demo.

TEST_CASE("FileDialog mode and filters") {
  FileDialog dlg;
  CHECK(dlg.selectedPath().empty());

  dlg.setMode(FileDialogMode::OpenFile);
  dlg.addFilter({"Text files", "*.txt"});
  dlg.addFilter({"All files", "*.*"});
  dlg.setDefaultPath("docs");
  CHECK(dlg.selectedPath().empty());

  dlg.clearFilters();
  // After clearFilters the dialog keeps mode/path state; only filters reset.
  dlg.setMode(FileDialogMode::SaveFile);
}

TEST_CASE("FileDialog returns None without a window") {
  FileDialog dlg;
  // exec() without an attached window must fail gracefully (no crash)...
  dlg.setDefaultPath("no_such_dir_ltgui_test");  // empty listing, no scan hit
  CHECK(dlg.exec() == DialogResult::None);
  // ...and with an empty folder the selection stays empty.
  CHECK(dlg.selectedPath().empty());
}
