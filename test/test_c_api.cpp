// C++-side doctest wrapper for the C SDK. The pure-C coverage lives in
// c_api_test.c (compiled as C23); this file asserts the fine-grained
// behaviors the C test cannot reach without an event loop.
#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "c/c_ltgui.h"

#include "doctest/doctest.h"

#include <string>

TEST_CASE("C API: layout ownership and error codes") {
  auto *root = ltgui_widget_create(nullptr);
  REQUIRE(root != nullptr);

  auto *layout = ltgui_boxlayout_create(2, 8, 12);
  REQUIRE(layout != nullptr);
  CHECK(ltgui_widget_set_layout(root, layout) == LTGUI_OK);
  // Destroy after attach: shell only, the widget owns the layout.
  ltgui_layout_destroy(layout);

  CHECK(ltgui_widget_set_layout(root, nullptr) == LTGUI_ERR_NULL_PTR);
  CHECK(ltgui_widget_set_visible(root, true) == LTGUI_OK);

  // Table model logic without a window.
  auto *table = ltgui_table_create(root);
  REQUIRE(table != nullptr);
  CHECK(ltgui_table_add_column(table, "Name", 150) == LTGUI_OK);
  const char *const row[] = {"a", "b"};
  CHECK(ltgui_table_add_row(table, row, 2) == LTGUI_OK);
  CHECK(ltgui_table_row_count(table) == 1);
  const char *cell = nullptr;
  CHECK(ltgui_table_cell_text(table, 0, 1, &cell) == LTGUI_OK);
  CHECK(std::string(cell) == "b");

  ltgui_widget_destroy(root);
}

TEST_CASE("C API: signals register and report") {
  auto *root = ltgui_widget_create(nullptr);
  REQUIRE(root != nullptr);
  auto *btn = ltgui_button_create("x", root);
  REQUIRE(btn != nullptr);
  int id = ltgui_signal_connect(btn, LTGUI_SIGNAL_ON_CLICKED, nullptr, nullptr);
  CHECK(id >= 0);
  CHECK(ltgui_signal_disconnect_all(btn, LTGUI_SIGNAL_ON_CLICKED) == LTGUI_OK);
  // Wrong signal for a button is rejected.
  CHECK(ltgui_signal_connect(btn, LTGUI_SIGNAL_ON_TEXT_CHANGED, nullptr,
                             nullptr) == LTGUI_ERR_BAD_ARGUMENT);
  ltgui_widget_destroy(root);
}

TEST_CASE("C API: error reporting on bad handles") {
  CHECK(ltgui_widget_set_visible(nullptr, true) == LTGUI_ERR_NULL_PTR);
  CHECK(ltgui_set_theme(nullptr) == LTGUI_ERR_NULL_PTR);
  CHECK(ltgui_set_theme("Nope") == LTGUI_ERR_BAD_ARGUMENT);
  CHECK(std::string(ltgui_last_error()).size() > 0);
}

TEST_CASE("C API: i18n and string pump") {
  static const char *const json =
      "{\"ok\":\"OK\"}";
  CHECK(ltgui_i18n_add_table("en", json) == LTGUI_OK);
  CHECK(ltgui_i18n_set_locale("en") == LTGUI_OK);
  CHECK(std::string(ltgui_i18n_tr("ok")) == "OK");
  // Untranslated key returns the key itself.
  CHECK(std::string(ltgui_i18n_tr("missing")) == "missing");
}
