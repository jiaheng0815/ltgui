// Pure-C23 test for the ltgui C SDK. Compiled with clang -std=c23 and linked
// against the ltgui static library. Deliberately exercises C23: auto
// (object inference), nullptr, bool, constexpr, [[nodiscard]].
#include "c/c_ltgui.h"

#include <stdio.h>
#include <string.h>

static int g_clicks = 0;

static void on_click(void *userdata, void *arg) {
  (void)arg;
  g_clicks++;
  (*(int *)userdata)++;  // the payload int is just another counter
}

static int fail(const char *what) {
  printf("FAIL: %s (%s)\n", what, ltgui_last_error());
  return 1;
}

int main(void) {
  // --- window/grids ------------------------------------------------------
  auto win = ltgui_window_create(420, 320, "ltgui C23 test");
  if (!win)
    return fail("window_create");

  auto root = ltgui_widget_create(nullptr);
  if (!root)
    return fail("widget_create");

  auto layout = ltgui_boxlayout_create(2 /*TopToBottom*/, 8, 12);
  if (!layout)
    return fail("boxlayout_create");
  if (ltgui_widget_set_layout(root, layout) != LTGUI_OK)
    return fail("set_layout");
  ltgui_layout_destroy(layout);  // no-op: owning moved to the widget

  // --- button / signal ----------------------------------------------------
  auto btn = ltgui_button_create("Click", root);
  if (!btn)
    return fail("button_create");

  int clicks = 0;
  if (ltgui_signal_connect(btn, LTGUI_SIGNAL_ON_CLICKED, on_click, &clicks) < 0)
    return fail("signal_connect");

  // C23: constexpr constant, bool parameters.
  constexpr int kExpectOk = LTGUI_OK;
  if (ltgui_widget_set_visible(btn, true) != kExpectOk)
    return fail("set_visible");

  ltgui_button_set_text(btn, "Hi");
  auto *text = ltgui_button_text(btn);
  if (!text || strcmp(text, "Hi") != 0)
    return fail("button text");

  // --- list (checked via the data model, no event loop needed) ------------
  auto list = ltgui_listbox_create(root);
  if (!list)
    return fail("listbox_create");
  if (ltgui_list_add_item(list, "alpha") != LTGUI_OK ||
      ltgui_list_add_item(list, "beta") != LTGUI_OK)
    return fail("add_item");
  if (ltgui_list_count(list) != 2)
    return fail("list count");
  if (ltgui_list_current_index(list) != 0)  // first item auto-selected
    return fail("auto selection");

  // --- table ---------------------------------------------------------------
  auto table = ltgui_table_create(root);
  if (!table)
    return fail("table_create");
  if (ltgui_table_add_column(table, "Name", 150) != LTGUI_OK ||
      ltgui_table_add_column(table, "Value", 90) != LTGUI_OK)
    return fail("add_column");
  const char *const row0[] = {"one", "1"};
  const char *const row1[] = {"two", "2"};
  if (ltgui_table_add_row(table, row0, 2) != LTGUI_OK ||
      ltgui_table_add_row(table, row1, 2) != LTGUI_OK)
    return fail("add_row");
  if (ltgui_table_row_count(table) != 2)
    return fail("row count");
  const char *cell;
  if (ltgui_table_cell_text(table, 1, 0, &cell) != LTGUI_OK ||
      strcmp(cell, "two") != 0)
    return fail("cell text");

  // --- error path -----------------------------------------------------------
  if (ltgui_set_theme("NoSuchTheme") != LTGUI_ERR_BAD_ARGUMENT)
    return fail("bad theme code");
  if (ltgui_set_theme("Nord") != LTGUI_OK)
    return fail("set theme");

  // --- i18n ------------------------------------------------------------------
  static const char *const zhjson =
      "{\"ok\":\"确定\",\"files\":[\"\",\"\",\"\",\"\",\"\",\"%d 个文件\"]}";
  if (ltgui_i18n_add_table("zh_CN", zhjson) != LTGUI_OK)
    return fail("i18n table");
  if (ltgui_i18n_set_locale("zh_CN") != LTGUI_OK)
    return fail("i18n locale");
  if (strcmp(ltgui_i18n_tr("ok"), "确定") != 0)
    return fail("i18n tr");

  // --- animation ---------------------------------------------------------------
  auto anim = ltgui_animated_float_create(0.0f);
  if (!anim)
    return fail("anim create");
  if (ltgui_animated_float_set_target(anim, 1.0f, 100, 22) != LTGUI_OK)
    return fail("anim target");
  float tgt = 0.0f;
  if (ltgui_animated_float_target(anim, &tgt) != LTGUI_OK || tgt != 1.0f)
    return fail("anim target value");
  // The animation only advances inside the event loop; immediate works without it.
  if (ltgui_animated_float_set_immediate(anim, 0.5f) != LTGUI_OK)
    return fail("anim immediate");
  ltgui_animated_float_destroy(anim);

  // --- clipboard ---------------------------------------------------------------
  if (ltgui_clipboard_set_text("c-sdk") != LTGUI_OK)
    return fail("clipboard set");
  if (strcmp(ltgui_clipboard_get_text(), "c-sdk") != 0)
    return fail("clipboard get");

  // --- teardown -------------------------------------------------------------------
  ltgui_window_set_central_widget(win, root);
  ltgui_widget_destroy(root);   // handle shell released (tree owned by window)
  ltgui_window_destroy(win);

  printf("PASS: c_sdk test\n");
  return 0;
}
