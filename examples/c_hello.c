// Pure C23 minimal ltgui example — mirrors examples/hello.cpp.
//
// Build (Windows, clang):
//   clang -std=c23 -I include/ltgui -c examples/c_hello.c -o build/c_hello.o
//   clang++ build/c_hello.o build/lib/ltgui.lib [-l...] -o build/c_hello.exe
#include "c/c_ltgui.h"

#include <stdio.h>

static void on_clicked(void *userdata, void *arg) {
  (void)arg;
  int *count = (int *)userdata;
  (*count)++;
  // Reads/writes through the API from the callback are fine (same thread).
  printf("clicked %d times\n", *count);
}

int main(void) {
  auto win = ltgui_window_create(400, 300, "ltgui from C");
  if (!win) {
    fprintf(stderr, "window failed: %s\n", ltgui_last_error());
    return 1;
  }

  auto root = ltgui_widget_create(nullptr);
  auto layout = ltgui_boxlayout_create(2 /*TopToBottom*/, 8, 12);
  (void)ltgui_widget_set_layout(root, layout);
  ltgui_layout_destroy(layout);

  auto label = ltgui_label_create("Welcome from C23!", root);
  (void)label;
  auto button = ltgui_button_create("Click Me", root);
  int count = 0;
  (void)ltgui_signal_connect(button, LTGUI_SIGNAL_ON_CLICKED, on_clicked,
                             &count);

  (void)ltgui_window_set_central_widget(win, root);
  ltgui_window_show(win);
  return ltgui_run();
}
