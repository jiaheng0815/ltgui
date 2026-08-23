// C23 binding implementation — the C boundary of LTGUI.
//
// Every public function catches at the boundary and converts C++ exceptions
// to error codes; ltgui_last_error() reports the last message.
#include "ltgui.h"
#include "c/c_ltgui.h"

#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <memory>
#include <string>
#include <vector>

using namespace ltgui;

// ----- opaque handle storage (C++ side of the C types in c_ltgui.h) -------
struct ltgui_widget {
  ltgui::Widget *ptr = nullptr;
  WidgetType type = WidgetType::Base;
  // TableView owns its model; keep it alive from the C side.
  std::shared_ptr<ltgui::SimpleTableModel> tableModel;
};

struct ltgui_window {
  ltgui::Window *win = nullptr;
};

struct ltgui_layout {
  std::unique_ptr<ltgui::Layout> layout;  // remaining after set_layout
  bool attached = false;                  // ownership moved to a widget
};

struct ltgui_timer {
  ltgui::Timer t;
  bool running = false;
};

struct ltgui_animation {
  ltgui::AnimatedFloat anim;
};

// ----- error reporting ------------------------------------------------------
static thread_local char g_lastError[512] = {};

#define LTGUI_TRY_BEGIN try {
#define LTGUI_TRY_END                                          \
  } catch (const std::exception &e) {                          \
    std::snprintf(g_lastError, sizeof g_lastError, "%s", e.what()); \
    return LTGUI_ERR_FAILED;                                   \
  } catch (...) {                                              \
    std::snprintf(g_lastError, sizeof g_lastError, "unknown C++ failure"); \
    return LTGUI_ERR_FAILED;                                   \
  }
#define LTGUI_TRY_END_VOID                                     \
  } catch (const std::exception &e) {                          \
    std::snprintf(g_lastError, sizeof g_lastError, "%s", e.what()); \
  } catch (...) {                                              \
    std::snprintf(g_lastError, sizeof g_lastError, "unknown C++ failure"); \
  }

// ----- helpers ----------------------------------------------------------------
template <typename T, typename... Args>
static ltgui_widget *make_widget(ltgui_widget *parent, Args &&...args) {
  auto *holder = new ltgui_widget();
  ltgui::Widget *w = nullptr;
  if (parent && parent->ptr)
    w = parent->ptr->makeChild<T>(std::forward<Args>(args)...);
  else
    w = new T(std::forward<Args>(args)...);
  holder->ptr = w;
  holder->type = w->widgetType();
  return holder;
}

static ltgui::Widget *wptr(ltgui_widget *w) {
  return w ? w->ptr : nullptr;
}

// ----- system ------------------------------------------------------------------
void ltgui_init(void) {}

int ltgui_run(void) {
  LTGUI_TRY_BEGIN
  return Application::instance().run();
  LTGUI_TRY_END
}

void ltgui_quit(void) { /* windows close through event loop; kept as no-op hook */ }

const char *ltgui_last_error(void) { return g_lastError; }

int ltgui_set_theme(const char *name) {
  LTGUI_TRY_BEGIN
  if (!name)
    return LTGUI_ERR_NULL_PTR;
  bool found = false;
  for (const auto &n : ThemeManager::instance().availableThemes())
    if (n == name)
      found = true;
  if (!found) {
    std::snprintf(g_lastError, sizeof g_lastError, "unknown theme '%s'", name);
    return LTGUI_ERR_BAD_ARGUMENT;
  }
  ThemeManager::instance().setThemeByName(name);
  return LTGUI_OK;
  LTGUI_TRY_END
}

const char *ltgui_current_theme_name(void) {
  LTGUI_TRY_BEGIN
  static std::string name;
  name = ThemeManager::instance().currentThemeName();
  return name.c_str();
  LTGUI_TRY_END_VOID
  return nullptr;
}

// ----- window --------------------------------------------------------------------
ltgui_window *ltgui_window_create(int width, int height, const char *title) {
  LTGUI_TRY_BEGIN
  auto *holder = new ltgui_window();
  holder->win = new ltgui::Window();
  if (!holder->win->create(width, height, title ? title : "ltgui")) {
    delete holder->win;
    delete holder;
    std::snprintf(g_lastError, sizeof g_lastError, "window create failed");
    return nullptr;
  }
  return holder;
  LTGUI_TRY_END_VOID
  return nullptr;
}

void ltgui_window_destroy(ltgui_window *win) {
  if (!win)
    return;
  LTGUI_TRY_BEGIN
  delete win->win;
  delete win;
  LTGUI_TRY_END_VOID
}

int ltgui_window_size(ltgui_window *win, int *w, int *h) {
  if (!win || !win->win)
    return LTGUI_ERR_NULL_PTR;
  auto s = win->win->size();
  if (w) *w = s.width;
  if (h) *h = s.height;
  return LTGUI_OK;
}

int ltgui_window_set_size(ltgui_window *win, int width, int height) {
  LTGUI_TRY_BEGIN
  if (!win || !win->win)
    return LTGUI_ERR_NULL_PTR;
  win->win->setSize(width, height);
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_window_set_title(ltgui_window *win, const char *title) {
  LTGUI_TRY_BEGIN
  if (!win || !win->win)
    return LTGUI_ERR_NULL_PTR;
  win->win->setTitle(title ? title : "");
  return LTGUI_OK;
  LTGUI_TRY_END
}

void ltgui_window_show(ltgui_window *win) {
  if (win && win->win)
    win->win->show();
}

void ltgui_window_hide(ltgui_window *win) {
  if (win && win->win)
    win->win->hide();
}

void ltgui_window_close(ltgui_window *win) {
  if (win && win->win)
    win->win->close();
}

void ltgui_window_update(ltgui_window *win) {
  if (win && win->win)
    win->win->update();
}

int ltgui_window_set_central_widget(ltgui_window *win, ltgui_widget *root) {
  LTGUI_TRY_BEGIN
  if (!win || !win->win)
    return LTGUI_ERR_NULL_PTR;
  if (!root || !root->ptr)
    return LTGUI_ERR_NULL_PTR;
  win->win->setCentralWidget(std::unique_ptr<ltgui::Widget>(root->ptr));
  root->ptr = nullptr;  // ownership transferred
  return LTGUI_OK;
  LTGUI_TRY_END
}

ltgui_widget *ltgui_window_central_widget(ltgui_window *win) {
  if (!win || !win->win)
    return nullptr;
  ltgui::Widget *w = win->win->centralWidget();
  if (!w)
    return nullptr;
  auto *holder = new ltgui_widget();
  holder->ptr = w;
  holder->type = w->widgetType();
  return holder;
}

int ltgui_window_set_cursor(ltgui_window *win, int cursor) {
  LTGUI_TRY_BEGIN
  if (!win || !win->win)
    return LTGUI_ERR_NULL_PTR;
  win->win->setCursor(static_cast<CursorShape>(cursor));
  return LTGUI_OK;
  LTGUI_TRY_END
}

// ----- widgets -------------------------------------------------------------------
ltgui_widget *ltgui_widget_create(ltgui_widget *parent) {
  LTGUI_TRY_BEGIN
  return make_widget<ltgui::Widget>(parent);
  LTGUI_TRY_END_VOID
  return nullptr;
}

void ltgui_widget_destroy(ltgui_widget *w) {
  if (!w)
    return;
  LTGUI_TRY_BEGIN
  // Detached root or standalone widget: delete outright if it still owns its
  // tree; attached handles (in a widget tree) must not be deleted (the tree
  // owns them) — they are released when the window closes.
  if (w->ptr) {
    // Take ownership by detaching from parent (no-op for tree roots).
    ltgui::Widget *p = w->ptr->parent();
    if (p) {
      std::unique_ptr<ltgui::Widget> owned = p->removeChild(w->ptr);
      (void)owned;  // freed at scope end
    }
  }
  delete w;
  LTGUI_TRY_END_VOID
}

ltgui_widget *ltgui_widget_add_child(ltgui_widget *parent, ltgui_widget *child) {
  LTGUI_TRY_BEGIN
  if (!parent || !parent->ptr || !child || !child->ptr)
    return nullptr;
  // Steal the child from its current parent (if any) — addChild requires
  // detached ownership.
  if (ltgui::Widget *cp = child->ptr->parent())
    (void)cp->removeChild(child->ptr);
  parent->ptr->addChild(std::unique_ptr<ltgui::Widget>(child->ptr));
  child->ptr = nullptr;
  return child;
  LTGUI_TRY_END_VOID
  return nullptr;
}

int ltgui_widget_remove_child(ltgui_widget *parent, ltgui_widget *child) {
  LTGUI_TRY_BEGIN
  if (!parent || !parent->ptr || !child || !child->ptr)
    return LTGUI_ERR_NULL_PTR;
  std::unique_ptr<ltgui::Widget> owned = parent->ptr->removeChild(child->ptr);
  if (!owned)
    return LTGUI_ERR_BAD_ARGUMENT;
  child->ptr = owned.release();
  return LTGUI_OK;
  LTGUI_TRY_END
}

ltgui_widget *ltgui_widget_parent(ltgui_widget *w) {
  if (!w || !w->ptr)
    return nullptr;
  ltgui::Widget *p = w->ptr->parent();
  if (!p)
    return nullptr;
  auto *holder = new ltgui_widget();
  holder->ptr = p;
  holder->type = p->widgetType();
  return holder;
}

int ltgui_widget_child_count(ltgui_widget *w) {
  if (!w || !w->ptr)
    return LTGUI_ERR_NULL_PTR;
  return static_cast<int>(w->ptr->children().size());
}

int ltgui_widget_geometry(ltgui_widget *w, int *x, int *y, int *ow, int *oh) {
  if (!w || !w->ptr)
    return LTGUI_ERR_NULL_PTR;
  auto r = w->ptr->geometry();
  if (x) *x = r.x;
  if (y) *y = r.y;
  if (ow) *ow = r.width;
  if (oh) *oh = r.height;
  return LTGUI_OK;
}

int ltgui_widget_set_geometry(ltgui_widget *w, int x, int y, int width,
                              int height) {
  LTGUI_TRY_BEGIN
  if (!w || !w->ptr)
    return LTGUI_ERR_NULL_PTR;
  w->ptr->setGeometry(Rect(x, y, width, height));
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_widget_size_hint(ltgui_widget *w, int *width, int *height) {
  LTGUI_TRY_BEGIN
  if (!w || !w->ptr)
    return LTGUI_ERR_NULL_PTR;
  auto s = w->ptr->sizeHint();
  if (width) *width = s.width;
  if (height) *height = s.height;
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_widget_set_visible(ltgui_widget *w, bool visible) {
  LTGUI_TRY_BEGIN
  if (!w || !w->ptr)
    return LTGUI_ERR_NULL_PTR;
  w->ptr->setVisible(visible);
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_widget_set_enabled(ltgui_widget *w, bool enabled) {
  LTGUI_TRY_BEGIN
  if (!w || !w->ptr)
    return LTGUI_ERR_NULL_PTR;
  w->ptr->setEnabled(enabled);
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_widget_raise_to_top(ltgui_widget *w) {
  LTGUI_TRY_BEGIN
  if (!w || !w->ptr)
    return LTGUI_ERR_NULL_PTR;
  w->ptr->raiseToTop();
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_widget_set_layout(ltgui_widget *w, ltgui_layout *layout) {
  LTGUI_TRY_BEGIN
  if (!w || !w->ptr)
    return LTGUI_ERR_NULL_PTR;
  if (!layout || !layout->layout)
    return LTGUI_ERR_NULL_PTR;
  w->ptr->setLayout(std::move(layout->layout));
  layout->attached = true;
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_widget_update(ltgui_widget *w) {
  if (!w || !w->ptr)
    return LTGUI_ERR_NULL_PTR;
  w->ptr->update();
  return LTGUI_OK;
}

int ltgui_widget_set_bg_color(ltgui_widget *w, unsigned argb8888) {
  LTGUI_TRY_BEGIN
  if (!w || !w->ptr)
    return LTGUI_ERR_NULL_PTR;
  auto &st = w->ptr->style();
  st.bgColor = Color(argb8888 >> 24, (argb8888 >> 16) & 0xFF, (argb8888 >> 8) & 0xFF,
                     argb8888 & 0xFF);
  w->ptr->update();
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_widget_set_fg_color(ltgui_widget *w, unsigned argb8888) {
  LTGUI_TRY_BEGIN
  if (!w || !w->ptr)
    return LTGUI_ERR_NULL_PTR;
  auto &st = w->ptr->style();
  st.fgColor = Color(argb8888 >> 24, (argb8888 >> 16) & 0xFF, (argb8888 >> 8) & 0xFF,
                     argb8888 & 0xFF);
  w->ptr->update();
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_widget_set_border(ltgui_widget *w, unsigned argb8888, int width,
                            int radius) {
  LTGUI_TRY_BEGIN
  if (!w || !w->ptr)
    return LTGUI_ERR_NULL_PTR;
  auto &st = w->ptr->style();
  st.borderColor = Color(argb8888 >> 24, (argb8888 >> 16) & 0xFF, (argb8888 >> 8) & 0xFF,
                         argb8888 & 0xFF);
  st.borderWidth = width;
  st.borderRadius = radius;
  w->ptr->update();
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_signal_connect(ltgui_widget *w, int signal,
                         void (*cb)(void *, void *), void *userdata) {
  LTGUI_TRY_BEGIN
  if (!w || !w->ptr)
    return LTGUI_ERR_NULL_PTR;
  switch (signal) {
    case LTGUI_SIGNAL_ON_CLICKED:
    case LTGUI_SIGNAL_ON_TOGGLED:
    case LTGUI_SIGNAL_ON_VALUE_CHANGED:
    case LTGUI_SIGNAL_ON_SELECTION_CHANGED:
    case LTGUI_SIGNAL_ON_TEXT_CHANGED:
    case LTGUI_SIGNAL_ON_ROW_SELECTED:
    case LTGUI_SIGNAL_ON_HEADER_CLICKED:
    case LTGUI_SIGNAL_ON_TREE_SELECTION:
    case LTGUI_SIGNAL_ON_FINISHED:
    case LTGUI_SIGNAL_ON_ITEMS_CHANGED:
      break;
    default:
      return LTGUI_ERR_BAD_ARGUMENT;
  }

  switch (w->type) {
    case WidgetType::Button: {
      auto *b = static_cast<ltgui::Button *>(w->ptr);
      if (signal == LTGUI_SIGNAL_ON_CLICKED)
        return b->onClicked.connect([cb, userdata](void) {
          if (cb) cb(userdata, nullptr);
        });
      break;
    }
    case WidgetType::CheckBox:
    case WidgetType::RadioButton: {
      auto *c = dynamic_cast<ltgui::Checkable *>(w->ptr);
      if (signal == LTGUI_SIGNAL_ON_TOGGLED)
        return c->onToggled.connect([cb, userdata](bool v) {
          if (cb) cb(userdata, &v);
        });
      break;
    }
    case WidgetType::Slider:
    case WidgetType::ProgressBar: {
      auto *r = static_cast<ltgui::Range *>(w->ptr);
      if (signal == LTGUI_SIGNAL_ON_VALUE_CHANGED)
        return r->onValueChanged.connect([cb, userdata](int v) {
          if (cb) cb(userdata, &v);
        });
      break;
    }
    case WidgetType::ListBox:
    case WidgetType::ComboBox: {
      auto *l = dynamic_cast<ltgui::ListItems *>(w->ptr);
      if (signal == LTGUI_SIGNAL_ON_SELECTION_CHANGED)
        return l->onSelectionChanged.connect([cb, userdata](int v) {
          if (cb) cb(userdata, &v);
        });
      if (signal == LTGUI_SIGNAL_ON_ITEMS_CHANGED)
        return l->onItemsChanged.connect([cb, userdata](void) {
          if (cb) cb(userdata, nullptr);
        });
      break;
    }
    case WidgetType::TextBox: {
      auto *t = static_cast<ltgui::TextBox *>(w->ptr);
      if (signal == LTGUI_SIGNAL_ON_TEXT_CHANGED)
        return t->onTextChanged.connect([cb, userdata](const std::string &s) {
          if (cb) cb(userdata, (void *)s.c_str());
        });
      break;
    }
    case WidgetType::TableView: {
      auto *t = static_cast<ltgui::TableView *>(w->ptr);
      if (signal == LTGUI_SIGNAL_ON_ROW_SELECTED)
        return t->onRowSelected.connect([cb, userdata](int v) {
          if (cb) cb(userdata, &v);
        });
      if (signal == LTGUI_SIGNAL_ON_HEADER_CLICKED)
        return t->onHeaderClicked.connect([cb, userdata](int v) {
          if (cb) cb(userdata, &v);
        });
      break;
    }
    case WidgetType::TreeView: {
      auto *t = static_cast<ltgui::TreeView *>(w->ptr);
      if (signal == LTGUI_SIGNAL_ON_TREE_SELECTION)
        return t->onSelectionChanged.connect([cb, userdata](TreeViewItem *i) {
          if (cb) cb(userdata, (void *)(i ? i->text().c_str() : nullptr));
        });
      break;
    }
    case WidgetType::Dialog: {
      auto *d = static_cast<ltgui::Dialog *>(w->ptr);
      if (signal == LTGUI_SIGNAL_ON_FINISHED)
        return d->onFinished.connect([cb, userdata](DialogResult r) {
          int v = static_cast<int>(r);
          if (cb) cb(userdata, &v);
        });
      break;
    }
    default:
      break;
  }
  return LTGUI_ERR_BAD_ARGUMENT;
  LTGUI_TRY_END
}

int ltgui_signal_disconnect_all(ltgui_widget *w, int signal) {
  LTGUI_TRY_BEGIN
  if (!w || !w->ptr)
    return LTGUI_ERR_NULL_PTR;
  switch (signal) {
    case LTGUI_SIGNAL_ON_CLICKED:
      if (w->type != WidgetType::Button)
        return LTGUI_ERR_BAD_HANDLE;
      static_cast<ltgui::Button *>(w->ptr)->onClicked.disconnectAll();
      break;
    case LTGUI_SIGNAL_ON_TOGGLED:
      dynamic_cast<ltgui::Checkable *>(w->ptr)->onToggled.disconnectAll();
      break;
    case LTGUI_SIGNAL_ON_VALUE_CHANGED:
      static_cast<ltgui::Range *>(w->ptr)->onValueChanged.disconnectAll();
      break;
    case LTGUI_SIGNAL_ON_SELECTION_CHANGED:
      dynamic_cast<ltgui::ListItems *>(w->ptr)->onSelectionChanged.disconnectAll();
      break;
    case LTGUI_SIGNAL_ON_TEXT_CHANGED:
      static_cast<ltgui::TextBox *>(w->ptr)->onTextChanged.disconnectAll();
      break;
    case LTGUI_SIGNAL_ON_ROW_SELECTED:
      static_cast<ltgui::TableView *>(w->ptr)->onRowSelected.disconnectAll();
      break;
    case LTGUI_SIGNAL_ON_HEADER_CLICKED:
      static_cast<ltgui::TableView *>(w->ptr)->onHeaderClicked.disconnectAll();
      break;
    case LTGUI_SIGNAL_ON_TREE_SELECTION:
      static_cast<ltgui::TreeView *>(w->ptr)->onSelectionChanged.disconnectAll();
      break;
    case LTGUI_SIGNAL_ON_FINISHED:
      static_cast<ltgui::Dialog *>(w->ptr)->onFinished.disconnectAll();
      break;
    default:
      return LTGUI_ERR_BAD_ARGUMENT;
  }
  return LTGUI_OK;
  LTGUI_TRY_END
}

// ----- Button / Label ----------------------------------------------------------------
ltgui_widget *ltgui_button_create(const char *text, ltgui_widget *parent) {
  LTGUI_TRY_BEGIN
  return make_widget<ltgui::Button>(parent, text ? text : "");
  LTGUI_TRY_END_VOID
  return nullptr;
}

int ltgui_button_set_text(ltgui_widget *btn, const char *text) {
  LTGUI_TRY_BEGIN
  if (!btn || !btn->ptr)
    return LTGUI_ERR_NULL_PTR;
  static_cast<ltgui::Button *>(btn->ptr)->setText(text ? text : "");
  return LTGUI_OK;
  LTGUI_TRY_END
}

const char *ltgui_button_text(ltgui_widget *btn) {
  static std::string tmp;
  if (!btn || !btn->ptr) {
    tmp.clear();
    return nullptr;
  }
  tmp = static_cast<ltgui::Button *>(btn->ptr)->text();
  return tmp.c_str();
}

ltgui_widget *ltgui_label_create(const char *text, ltgui_widget *parent) {
  LTGUI_TRY_BEGIN
  return make_widget<ltgui::Label>(parent, text ? text : "");
  LTGUI_TRY_END_VOID
  return nullptr;
}

int ltgui_label_set_text(ltgui_widget *label, const char *text) {
  LTGUI_TRY_BEGIN
  if (!label || !label->ptr)
    return LTGUI_ERR_NULL_PTR;
  static_cast<ltgui::Label *>(label->ptr)->setText(text ? text : "");
  return LTGUI_OK;
  LTGUI_TRY_END
}

const char *ltgui_label_text(ltgui_widget *label) {
  static std::string tmp;
  if (!label || !label->ptr) {
    tmp.clear();
    return nullptr;
  }
  tmp = static_cast<ltgui::Label *>(label->ptr)->text();
  return tmp.c_str();
}

// ----- TextBox ------------------------------------------------------------------------
ltgui_widget *ltgui_textbox_create(const char *text, ltgui_widget *parent) {
  LTGUI_TRY_BEGIN
  return make_widget<ltgui::TextBox>(parent, text ? text : "");
  LTGUI_TRY_END_VOID
  return nullptr;
}

int ltgui_textbox_set_text(ltgui_widget *tb, const char *text) {
  LTGUI_TRY_BEGIN
  if (!tb || !tb->ptr)
    return LTGUI_ERR_NULL_PTR;
  static_cast<ltgui::TextBox *>(tb->ptr)->setText(text ? text : "");
  return LTGUI_OK;
  LTGUI_TRY_END
}

const char *ltgui_textbox_text(ltgui_widget *tb) {
  static std::string tmp;
  if (!tb || !tb->ptr) {
    tmp.clear();
    return nullptr;
  }
  tmp = static_cast<ltgui::TextBox *>(tb->ptr)->text();
  return tmp.c_str();
}

int ltgui_textbox_set_multiline(ltgui_widget *tb, bool multi) {
  LTGUI_TRY_BEGIN
  if (!tb || !tb->ptr)
    return LTGUI_ERR_NULL_PTR;
  static_cast<ltgui::TextBox *>(tb->ptr)->setMultiLine(multi);
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_textbox_copy(ltgui_widget *tb) {
  LTGUI_TRY_BEGIN
  if (!tb || !tb->ptr)
    return LTGUI_ERR_NULL_PTR;
  static_cast<ltgui::TextBox *>(tb->ptr)->copy();
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_textbox_cut(ltgui_widget *tb) {
  LTGUI_TRY_BEGIN
  if (!tb || !tb->ptr)
    return LTGUI_ERR_NULL_PTR;
  static_cast<ltgui::TextBox *>(tb->ptr)->cut();
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_textbox_paste(ltgui_widget *tb) {
  LTGUI_TRY_BEGIN
  if (!tb || !tb->ptr)
    return LTGUI_ERR_NULL_PTR;
  static_cast<ltgui::TextBox *>(tb->ptr)->paste();
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_textbox_select_all(ltgui_widget *tb) {
  LTGUI_TRY_BEGIN
  if (!tb || !tb->ptr)
    return LTGUI_ERR_NULL_PTR;
  static_cast<ltgui::TextBox *>(tb->ptr)->selectAll();
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_textbox_undo(ltgui_widget *tb) {
  LTGUI_TRY_BEGIN
  if (!tb || !tb->ptr)
    return LTGUI_ERR_NULL_PTR;
  static_cast<ltgui::TextBox *>(tb->ptr)->undo();
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_textbox_redo(ltgui_widget *tb) {
  LTGUI_TRY_BEGIN
  if (!tb || !tb->ptr)
    return LTGUI_ERR_NULL_PTR;
  static_cast<ltgui::TextBox *>(tb->ptr)->redo();
  return LTGUI_OK;
  LTGUI_TRY_END
}

// ----- CheckBox / RadioButton ----------------------------------------------------------
ltgui_widget *ltgui_checkbox_create(const char *text, ltgui_widget *parent) {
  LTGUI_TRY_BEGIN
  return make_widget<ltgui::CheckBox>(parent, text ? text : "");
  LTGUI_TRY_END_VOID
  return nullptr;
}

int ltgui_checkbox_set_checked(ltgui_widget *cb, bool checked) {
  LTGUI_TRY_BEGIN
  if (!cb || !cb->ptr)
    return LTGUI_ERR_NULL_PTR;
  dynamic_cast<ltgui::Checkable *>(cb->ptr)->setChecked(checked);
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_checkbox_is_checked(ltgui_widget *cb) {
  if (!cb || !cb->ptr)
    return LTGUI_ERR_NULL_PTR;
  return dynamic_cast<ltgui::Checkable *>(cb->ptr)->isChecked() ? 1 : 0;
}

ltgui_widget *ltgui_radiobutton_create(const char *text, ltgui_widget *parent) {
  LTGUI_TRY_BEGIN
  return make_widget<ltgui::RadioButton>(parent, text ? text : "");
  LTGUI_TRY_END_VOID
  return nullptr;
}

int ltgui_radiobutton_set_checked(ltgui_widget *rb, bool checked) {
  LTGUI_TRY_BEGIN
  if (!rb || !rb->ptr)
    return LTGUI_ERR_NULL_PTR;
  static_cast<ltgui::RadioButton *>(rb->ptr)->setChecked(checked);
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_radiobutton_is_checked(ltgui_widget *rb) {
  if (!rb || !rb->ptr)
    return LTGUI_ERR_NULL_PTR;
  return static_cast<ltgui::RadioButton *>(rb->ptr)->isChecked() ? 1 : 0;
}

// ----- Slider / ProgressBar -------------------------------------------------------------
ltgui_widget *ltgui_slider_create(ltgui_widget *parent) {
  LTGUI_TRY_BEGIN
  return make_widget<ltgui::Slider>(parent, nullptr);
  LTGUI_TRY_END_VOID
  return nullptr;
}

int ltgui_slider_set_range(ltgui_widget *sl, int min_, int max_) {
  LTGUI_TRY_BEGIN
  if (!sl || !sl->ptr)
    return LTGUI_ERR_NULL_PTR;
  static_cast<ltgui::Range *>(sl->ptr)->setRange(min_, max_);
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_slider_set_value(ltgui_widget *sl, int value) {
  LTGUI_TRY_BEGIN
  if (!sl || !sl->ptr)
    return LTGUI_ERR_NULL_PTR;
  static_cast<ltgui::Range *>(sl->ptr)->setValue(value);
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_slider_value(ltgui_widget *sl) {
  if (!sl || !sl->ptr)
    return LTGUI_ERR_NULL_PTR;
  return static_cast<ltgui::Range *>(sl->ptr)->value();
}

ltgui_widget *ltgui_progressbar_create(ltgui_widget *parent) {
  LTGUI_TRY_BEGIN
  return make_widget<ltgui::ProgressBar>(parent, nullptr);
  LTGUI_TRY_END_VOID
  return nullptr;
}

int ltgui_progressbar_set_range(ltgui_widget *pb, int min_, int max_) {
  LTGUI_TRY_BEGIN
  if (!pb || !pb->ptr)
    return LTGUI_ERR_NULL_PTR;
  static_cast<ltgui::Range *>(pb->ptr)->setRange(min_, max_);
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_progressbar_set_value(ltgui_widget *pb, int value) {
  LTGUI_TRY_BEGIN
  if (!pb || !pb->ptr)
    return LTGUI_ERR_NULL_PTR;
  static_cast<ltgui::ProgressBar *>(pb->ptr)->setValue(value);
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_progressbar_set_indeterminate(ltgui_widget *pb, bool on) {
  LTGUI_TRY_BEGIN
  if (!pb || !pb->ptr)
    return LTGUI_ERR_NULL_PTR;
  static_cast<ltgui::ProgressBar *>(pb->ptr)->setIndeterminate(on);
  return LTGUI_OK;
  LTGUI_TRY_END
}

// ----- ListBox / ComboBox ----------------------------------------------------------------
ltgui_widget *ltgui_listbox_create(ltgui_widget *parent) {
  LTGUI_TRY_BEGIN
  return make_widget<ltgui::ListBox>(parent, nullptr);
  LTGUI_TRY_END_VOID
  return nullptr;
}

ltgui_widget *ltgui_combobox_create(ltgui_widget *parent) {
  LTGUI_TRY_BEGIN
  return make_widget<ltgui::ComboBox>(parent, nullptr);
  LTGUI_TRY_END_VOID
  return nullptr;
}

int ltgui_list_add_item(ltgui_widget *list, const char *item) {
  LTGUI_TRY_BEGIN
  if (!list || !list->ptr)
    return LTGUI_ERR_NULL_PTR;
  dynamic_cast<ltgui::ListItems *>(list->ptr)->addItem(item ? item : "");
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_list_remove_item(ltgui_widget *list, int index) {
  LTGUI_TRY_BEGIN
  if (!list || !list->ptr)
    return LTGUI_ERR_NULL_PTR;
  dynamic_cast<ltgui::ListItems *>(list->ptr)->removeItem(index);
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_list_clear(ltgui_widget *list) {
  LTGUI_TRY_BEGIN
  if (!list || !list->ptr)
    return LTGUI_ERR_NULL_PTR;
  dynamic_cast<ltgui::ListItems *>(list->ptr)->clear();
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_list_count(ltgui_widget *list) {
  if (!list || !list->ptr)
    return LTGUI_ERR_NULL_PTR;
  return dynamic_cast<ltgui::ListItems *>(list->ptr)->count();
}

const char *ltgui_list_item(ltgui_widget *list, int index) {
  static std::string tmp;
  if (!list || !list->ptr) {
    tmp.clear();
    return nullptr;
  }
  tmp = dynamic_cast<ltgui::ListItems *>(list->ptr)->item(index);
  return tmp.c_str();
}

int ltgui_list_current_index(ltgui_widget *list) {
  if (!list || !list->ptr)
    return LTGUI_ERR_NULL_PTR;
  return dynamic_cast<ltgui::ListItems *>(list->ptr)->currentIndex();
}

int ltgui_list_set_current_index(ltgui_widget *list, int index) {
  LTGUI_TRY_BEGIN
  if (!list || !list->ptr)
    return LTGUI_ERR_NULL_PTR;
  dynamic_cast<ltgui::ListItems *>(list->ptr)->setCurrentIndex(index);
  return LTGUI_OK;
  LTGUI_TRY_END
}

const char *ltgui_combobox_current_text(ltgui_widget *cb) {
  static std::string tmp;
  if (!cb || !cb->ptr) {
    tmp.clear();
    return nullptr;
  }
  tmp = static_cast<ltgui::ComboBox *>(cb->ptr)->currentText();
  return tmp.c_str();
}

// ----- Image / ScrollArea -----------------------------------------------------------------
ltgui_widget *ltgui_image_create(ltgui_widget *parent) {
  LTGUI_TRY_BEGIN
  return make_widget<ltgui::Image>(parent, nullptr);
  LTGUI_TRY_END_VOID
  return nullptr;
}

int ltgui_image_load(ltgui_widget *img, const char *path) {
  LTGUI_TRY_BEGIN
  if (!img || !img->ptr)
    return LTGUI_ERR_NULL_PTR;
  if (!static_cast<ltgui::Image *>(img->ptr)->load(path ? path : ""))
    return LTGUI_ERR_BAD_ARGUMENT;
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_image_set_fit(ltgui_widget *img, int fit) {
  LTGUI_TRY_BEGIN
  if (!img || !img->ptr)
    return LTGUI_ERR_NULL_PTR;
  static_cast<ltgui::Image *>(img->ptr)->setFitMode(
      static_cast<ltgui::Image::FitMode>(fit));
  return LTGUI_OK;
  LTGUI_TRY_END
}

ltgui_widget *ltgui_scrollarea_create(ltgui_widget *parent) {
  LTGUI_TRY_BEGIN
  return make_widget<ltgui::ScrollArea>(parent, nullptr);
  LTGUI_TRY_END_VOID
  return nullptr;
}

int ltgui_scrollarea_set_widget(ltgui_widget *area, ltgui_widget *content) {
  LTGUI_TRY_BEGIN
  if (!area || !area->ptr)
    return LTGUI_ERR_NULL_PTR;
  if (!content || !content->ptr)
    return LTGUI_ERR_NULL_PTR;
  static_cast<ltgui::ScrollArea *>(area->ptr)->setWidget(
      std::unique_ptr<ltgui::Widget>(content->ptr));
  content->ptr = nullptr;
  return LTGUI_OK;
  LTGUI_TRY_END
}

ltgui_widget *ltgui_scrollarea_widget(ltgui_widget *area) {
  if (!area || !area->ptr)
    return nullptr;
  ltgui::Widget *w = static_cast<ltgui::ScrollArea *>(area->ptr)->widget();
  if (!w)
    return nullptr;
  auto *holder = new ltgui_widget();
  holder->ptr = w;
  holder->type = w->widgetType();
  return holder;
}

// ----- TabWidget ----------------------------------------------------------------------------
ltgui_widget *ltgui_tabwidget_create(ltgui_widget *parent) {
  LTGUI_TRY_BEGIN
  return make_widget<ltgui::TabWidget>(parent, nullptr);
  LTGUI_TRY_END_VOID
  return nullptr;
}

int ltgui_tabwidget_add_tab(ltgui_widget *tabs, const char *label) {
  LTGUI_TRY_BEGIN
  if (!tabs || !tabs->ptr)
    return LTGUI_ERR_NULL_PTR;
  return static_cast<ltgui::TabWidget *>(tabs->ptr)->addTab(label ? label : "");
  LTGUI_TRY_END
}

int ltgui_tabwidget_count(ltgui_widget *tabs) {
  if (!tabs || !tabs->ptr)
    return LTGUI_ERR_NULL_PTR;
  return static_cast<ltgui::TabWidget *>(tabs->ptr)->count();
}

int ltgui_tabwidget_set_current(ltgui_widget *tabs, int index) {
  LTGUI_TRY_BEGIN
  if (!tabs || !tabs->ptr)
    return LTGUI_ERR_NULL_PTR;
  static_cast<ltgui::TabWidget *>(tabs->ptr)->setCurrentIndex(index);
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_tabwidget_current(ltgui_widget *tabs) {
  if (!tabs || !tabs->ptr)
    return LTGUI_ERR_NULL_PTR;
  return static_cast<ltgui::TabWidget *>(tabs->ptr)->currentIndex();
}

ltgui_widget *ltgui_tabwidget_tab_content(ltgui_widget *tabs, int index) {
  if (!tabs || !tabs->ptr)
    return nullptr;
  ltgui::Widget *w =
      static_cast<ltgui::TabWidget *>(tabs->ptr)->tabContent(index);
  if (!w)
    return nullptr;
  auto *holder = new ltgui_widget();
  holder->ptr = w;
  holder->type = w->widgetType();
  return holder;
}

// ----- TableView -----------------------------------------------------------------------------
ltgui_widget *ltgui_table_create(ltgui_widget *parent) {
  LTGUI_TRY_BEGIN
  ltgui_widget *holder = make_widget<ltgui::TableView>(parent, nullptr);
  if (!holder || !holder->ptr)
    return nullptr;
  holder->tableModel = std::make_shared<ltgui::SimpleTableModel>(0, 4);
  static_cast<ltgui::TableView *>(holder->ptr)->setModel(holder->tableModel);
  return holder;
  LTGUI_TRY_END_VOID
  return nullptr;
}

int ltgui_table_add_column(ltgui_widget *table, const char *title, int width) {
  LTGUI_TRY_BEGIN
  if (!table || !table->ptr)
    return LTGUI_ERR_NULL_PTR;
  static_cast<ltgui::TableView *>(table->ptr)->addColumn(
      TableColumn{title ? title : "", width, 30, true, true});
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_table_add_row(ltgui_widget *table, const char *const *cells,
                        int columnCount) {
  LTGUI_TRY_BEGIN
  if (!table || !table->ptr || !table->tableModel)
    return LTGUI_ERR_NULL_PTR;
  std::vector<std::string> row;
  for (int i = 0; i < columnCount; i++)
    row.push_back(cells[i] ? cells[i] : "");
  table->tableModel->addRow(row);
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_table_row_count(ltgui_widget *table) {
  if (!table || !table->tableModel)
    return LTGUI_ERR_NULL_PTR;
  return table->tableModel->rowCount();
}

int ltgui_table_cell_text(ltgui_widget *table, int row, int col,
                          const char **outText) {
  if (!table || !table->tableModel || !outText)
    return LTGUI_ERR_NULL_PTR;
  auto s = table->tableModel->cellText(row, col);
  static thread_local std::string tmp;
  tmp = s;
  *outText = tmp.c_str();
  return LTGUI_OK;
}

int ltgui_table_set_sort(ltgui_widget *table, int col, bool ascending) {
  LTGUI_TRY_BEGIN
  if (!table || !table->ptr || !table->tableModel)
    return LTGUI_ERR_NULL_PTR;
  table->tableModel->sort(col, ascending);
  static_cast<ltgui::TableView *>(table->ptr)->setSortColumn(col, ascending);
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_table_set_current(ltgui_widget *table, int row) {
  LTGUI_TRY_BEGIN
  if (!table || !table->ptr)
    return LTGUI_ERR_NULL_PTR;
  static_cast<ltgui::TableView *>(table->ptr)->setCurrentIndex(row);
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_table_current(ltgui_widget *table) {
  if (!table || !table->ptr)
    return LTGUI_ERR_NULL_PTR;
  return static_cast<ltgui::TableView *>(table->ptr)->currentIndex();
}

// ----- TreeView --------------------------------------------------------------------------------
static TreeViewItem *find_tree_item(TreeViewItem *root, const char *text) {
  if (!root)
    return nullptr;
  if (text && root->text() == text)
    return root;
  for (auto &child : root->children()) {
    if (TreeViewItem *f = find_tree_item(child.get(), text))
      return f;
  }
  return nullptr;
}

ltgui_widget *ltgui_tree_create(ltgui_widget *parent) {
  LTGUI_TRY_BEGIN
  return make_widget<ltgui::TreeView>(parent, nullptr);
  LTGUI_TRY_END_VOID
  return nullptr;
}

int ltgui_tree_add_item(ltgui_widget *tree, const char *parentText,
                        const char *text) {
  LTGUI_TRY_BEGIN
  if (!tree || !tree->ptr)
    return LTGUI_ERR_NULL_PTR;
  auto *tv = static_cast<ltgui::TreeView *>(tree->ptr);
  if (!parentText) {
    tv->rootItem()->addChild(text ? text : "");
    return LTGUI_OK;
  }
  TreeViewItem *parent = find_tree_item(tv->rootItem(), parentText);
  if (!parent)
    return LTGUI_ERR_BAD_ARGUMENT;
  parent->addChild(text ? text : "");
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_tree_set_expanded(ltgui_widget *tree, const char *itemText,
                            bool expanded) {
  LTGUI_TRY_BEGIN
  if (!tree || !tree->ptr)
    return LTGUI_ERR_NULL_PTR;
  TreeViewItem *item =
      find_tree_item(static_cast<ltgui::TreeView *>(tree->ptr)->rootItem(),
                     itemText);
  if (!item)
    return LTGUI_ERR_BAD_ARGUMENT;
  item->setExpanded(expanded);
  return LTGUI_OK;
  LTGUI_TRY_END
}

// ----- MenuBar / ContextMenu ----------------------------------------------------------------------
ltgui_widget *ltgui_menubar_create(ltgui_widget *parent) {
  LTGUI_TRY_BEGIN
  return make_widget<ltgui::MenuBar>(parent, nullptr);
  LTGUI_TRY_END_VOID
  return nullptr;
}

int ltgui_menubar_add_menu(ltgui_widget *bar, const char *label) {
  LTGUI_TRY_BEGIN
  if (!bar || !bar->ptr)
    return LTGUI_ERR_NULL_PTR;
  return static_cast<ltgui::MenuBar *>(bar->ptr)->addMenu(label ? label : "");
  LTGUI_TRY_END
}

int ltgui_menubar_add_item(ltgui_widget *bar, int menuIdx, const char *text,
                           void (*cb)(void *, void *), void *userdata) {
  LTGUI_TRY_BEGIN
  if (!bar || !bar->ptr)
    return LTGUI_ERR_NULL_PTR;
  return static_cast<ltgui::MenuBar *>(bar->ptr)->addItem(
      menuIdx, text ? text : "",
      [cb, userdata](void) { if (cb) cb(userdata, nullptr); });
  LTGUI_TRY_END
}

int ltgui_menubar_add_separator(ltgui_widget *bar, int menuIdx) {
  LTGUI_TRY_BEGIN
  if (!bar || !bar->ptr)
    return LTGUI_ERR_NULL_PTR;
  static_cast<ltgui::MenuBar *>(bar->ptr)->addSeparator(menuIdx);
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_menubar_exit_shortcut(ltgui_widget *bar, int menuIdx, int itemIdx,
                                const char *shortcut) {
  LTGUI_TRY_BEGIN
  if (!bar || !bar->ptr)
    return LTGUI_ERR_NULL_PTR;
  static_cast<ltgui::MenuBar *>(bar->ptr)->setItemShortcut(
      menuIdx, itemIdx, shortcut ? shortcut : "");
  return LTGUI_OK;
  LTGUI_TRY_END
}

ltgui_widget *ltgui_contextmenu_create(ltgui_widget *parent) {
  LTGUI_TRY_BEGIN
  return make_widget<ltgui::ContextMenu>(parent, nullptr);
  LTGUI_TRY_END_VOID
  return nullptr;
}

int ltgui_contextmenu_add_item(ltgui_widget *menu, const char *text,
                               void (*cb)(void *, void *), void *userdata) {
  LTGUI_TRY_BEGIN
  if (!menu || !menu->ptr)
    return LTGUI_ERR_NULL_PTR;
  return static_cast<ltgui::ContextMenu *>(menu->ptr)->addItem(
      text ? text : "",
      [cb, userdata](void) { if (cb) cb(userdata, nullptr); });
  LTGUI_TRY_END
}

int ltgui_contextmenu_add_separator(ltgui_widget *menu) {
  LTGUI_TRY_BEGIN
  if (!menu || !menu->ptr)
    return LTGUI_ERR_NULL_PTR;
  static_cast<ltgui::ContextMenu *>(menu->ptr)->addSeparator();
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_contextmenu_popup(ltgui_widget *menu, int x, int y) {
  LTGUI_TRY_BEGIN
  if (!menu || !menu->ptr)
    return LTGUI_ERR_NULL_PTR;
  static_cast<ltgui::ContextMenu *>(menu->ptr)->popup(Point{x, y});
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_contextmenu_dismiss(ltgui_widget *menu) {
  LTGUI_TRY_BEGIN
  if (!menu || !menu->ptr)
    return LTGUI_ERR_NULL_PTR;
  static_cast<ltgui::ContextMenu *>(menu->ptr)->dismiss();
  return LTGUI_OK;
  LTGUI_TRY_END
}

// ----- Dialog family ------------------------------------------------------------------------
int ltgui_messagebox_show(ltgui_widget *parent, const char *title,
                          const char *message, int buttons, int icon) {
  LTGUI_TRY_BEGIN
  ltgui::Widget *p = wptr(parent);
  DialogResult r = ltgui::MessageBox::show(
      p, title ? title : "", message ? message : "", buttons,
      static_cast<ltgui::MessageBox::Icon>(icon));
  return static_cast<int>(r);
  LTGUI_TRY_END
}

const char *ltgui_inputdialog_get_text(ltgui_widget *parent, const char *title,
                                       const char *label,
                                       const char *defaultText, int ok) {
  static thread_local std::string result;
  LTGUI_TRY_BEGIN
  if (ok) {
    result = ltgui::InputDialog::getText(wptr(parent), title ? title : "",
                                         label ? label : "",
                                         defaultText ? defaultText : "");
  } else {
    result.clear();
  }
  return result.c_str();
  LTGUI_TRY_END_VOID
  return nullptr;
}

void ltgui_string_free(char *str) { free((void *)str); }

// ----- FileDialog ------------------------------------------------------------------------------
ltgui_widget *ltgui_filedialog_create(ltgui_widget *parent, int mode) {
  LTGUI_TRY_BEGIN
  return make_widget<ltgui::FileDialog>(parent, nullptr);
  LTGUI_TRY_END_VOID
  return nullptr;
}

int ltgui_filedialog_add_filter(ltgui_widget *dlg, const char *name,
                                const char *pattern) {
  LTGUI_TRY_BEGIN
  if (!dlg || !dlg->ptr)
    return LTGUI_ERR_NULL_PTR;
  static_cast<ltgui::FileDialog *>(dlg->ptr)->addFilter(
      FileFilter{name ? name : "", pattern ? pattern : "*.*"});
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_filedialog_set_default_path(ltgui_widget *dlg, const char *path) {
  LTGUI_TRY_BEGIN
  if (!dlg || !dlg->ptr)
    return LTGUI_ERR_NULL_PTR;
  static_cast<ltgui::FileDialog *>(dlg->ptr)->setDefaultPath(path ? path : "");
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_filedialog_exec(ltgui_widget *dlg) {
  LTGUI_TRY_BEGIN
  if (!dlg || !dlg->ptr)
    return LTGUI_ERR_NULL_PTR;
  return static_cast<int>(static_cast<ltgui::FileDialog *>(dlg->ptr)->exec());
  LTGUI_TRY_END
}

const char *ltgui_filedialog_selected_path(ltgui_widget *dlg) {
  static thread_local std::string path;
  if (!dlg || !dlg->ptr) {
    path.clear();
    return nullptr;
  }
  path = static_cast<ltgui::FileDialog *>(dlg->ptr)->selectedPath();
  return path.c_str();
}

// ----- Layouts ------------------------------------------------------------------------------------
ltgui_layout *ltgui_boxlayout_create(int dir, int spacing, int margin) {
  LTGUI_TRY_BEGIN
  auto *holder = new ltgui_layout();
  holder->layout = std::make_unique<ltgui::BoxLayout>(
      static_cast<ltgui::BoxLayout::Direction>(dir), spacing, margin);
  return holder;
  LTGUI_TRY_END_VOID
  return nullptr;
}

ltgui_layout *ltgui_gridlayout_create(int columns, int rowSpacing,
                                      int colSpacing, int margin) {
  LTGUI_TRY_BEGIN
  auto *holder = new ltgui_layout();
  holder->layout = std::make_unique<ltgui::GridLayout>(columns, rowSpacing,
                                                       colSpacing, margin);
  return holder;
  LTGUI_TRY_END_VOID
  return nullptr;
}

int ltgui_layout_add_stretch(ltgui_layout *layout, int factor) {
  LTGUI_TRY_BEGIN
  if (!layout || !layout->layout)
    return LTGUI_ERR_NULL_PTR;
  auto *box = dynamic_cast<ltgui::BoxLayout *>(layout->layout.get());
  if (!box)
    return LTGUI_ERR_BAD_HANDLE;
  box->addStretch(factor);
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_layout_set_spacing(ltgui_layout *layout, int spacing) {
  LTGUI_TRY_BEGIN
  if (!layout || !layout->layout)
    return LTGUI_ERR_NULL_PTR;
  if (auto *box = dynamic_cast<ltgui::BoxLayout *>(layout->layout.get()))
    box->setSpacing(spacing);
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_layout_set_stretch(ltgui_layout *layout, ltgui_widget *child,
                             int factor) {
  LTGUI_TRY_BEGIN
  if (!layout || !layout->layout || !child || !child->ptr)
    return LTGUI_ERR_NULL_PTR;
  if (auto *box = dynamic_cast<ltgui::BoxLayout *>(layout->layout.get()))
    box->setStretch(child->ptr, factor);
  return LTGUI_OK;
  LTGUI_TRY_END
}

void ltgui_layout_destroy(ltgui_layout *layout) {
  if (!layout)
    return;
  delete layout;  // layout->attached means ownership moved; delete the shell
                  // only (the unique_ptr is empty in that case).
}

// ----- Timer ---------------------------------------------------------------------------------------
ltgui_timer *ltgui_timer_start(int ms, bool repeating, void (*cb)(void *),
                               void *userdata) {
  LTGUI_TRY_BEGIN
  if (ms <= 0 || !cb)
    return nullptr;
  auto *wrapper = new ltgui_timer();
  auto fn = [cb, userdata]() { cb(userdata); };
  wrapper->t.start(ms, repeating, fn);
  wrapper->running = true;
  return wrapper;
  LTGUI_TRY_END_VOID
  return nullptr;
}

void ltgui_timer_stop(ltgui_timer *timer) {
  if (!timer)
    return;
  timer->t.stop();
  timer->running = false;
}

int ltgui_timer_single_shot(int ms, void (*cb)(void *), void *userdata) {
  LTGUI_TRY_BEGIN
  if (ms <= 0 || !cb)
    return LTGUI_ERR_BAD_ARGUMENT;
  ltgui::Timer::singleShot(ms, [cb, userdata]() { cb(userdata); });
  return LTGUI_OK;
  LTGUI_TRY_END
}

// ----- Clipboard / I18n -------------------------------------------------------------------------------
const char *ltgui_clipboard_get_text(void) {
  static thread_local std::string text;
  LTGUI_TRY_BEGIN
  text = ltgui::Clipboard::getText();
  return text.c_str();
  LTGUI_TRY_END_VOID
  return nullptr;
}

int ltgui_clipboard_set_text(const char *text) {
  LTGUI_TRY_BEGIN
  if (!text)
    return LTGUI_ERR_NULL_PTR;
  ltgui::Clipboard::setText(text);
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_i18n_add_table(const char *locale, const char *json) {
  LTGUI_TRY_BEGIN
  if (!locale || !json)
    return LTGUI_ERR_NULL_PTR;
  ltgui::TranslationTable table;
  if (!table.loadFromJsonString(json))
    return LTGUI_ERR_BAD_ARGUMENT;
  ltgui::I18n::instance().addTable(ltgui::Locale(locale), table);
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_i18n_set_locale(const char *locale) {
  LTGUI_TRY_BEGIN
  if (!locale)
    return LTGUI_ERR_NULL_PTR;
  ltgui::I18n::instance().setLocale(ltgui::Locale(locale));
  return LTGUI_OK;
  LTGUI_TRY_END
}

const char *ltgui_i18n_tr(const char *key) {
  static thread_local std::string out;
  LTGUI_TRY_BEGIN
  out = ltgui::I18n::instance().tr(key ? key : "");
  return out.c_str();
  LTGUI_TRY_END_VOID
  return nullptr;
}

const char *ltgui_i18n_tr_n(const char *key, long long n) {
  static thread_local std::string out;
  LTGUI_TRY_BEGIN
  out = ltgui::I18n::instance().tr(key ? key : "", n);
  return out.c_str();
  LTGUI_TRY_END_VOID
  return nullptr;
}

// ----- Animation ----------------------------------------------------------------------------------------
ltgui_animation *ltgui_animated_float_create(float initial) {
  auto *wrapper = new ltgui_animation();
  wrapper->anim = ltgui::AnimatedFloat(initial);
  return wrapper;
}

int ltgui_animated_float_set_target(ltgui_animation *anim, float target,
                                    int ms, int easing) {
  LTGUI_TRY_BEGIN
  if (!anim)
    return LTGUI_ERR_NULL_PTR;
  anim->anim.setTarget(target, ms, static_cast<ltgui::Easing>(easing));
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_animated_float_set_immediate(ltgui_animation *anim, float value) {
  LTGUI_TRY_BEGIN
  if (!anim)
    return LTGUI_ERR_NULL_PTR;
  anim->anim.setImmediate(value);
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_animated_float_set_loop(ltgui_animation *anim, bool loop,
                                  bool yoyo) {
  LTGUI_TRY_BEGIN
  if (!anim)
    return LTGUI_ERR_NULL_PTR;
  anim->anim.setLoop(loop);
  anim->anim.setYoyo(yoyo);
  return LTGUI_OK;
  LTGUI_TRY_END
}

int ltgui_animated_float_target(ltgui_animation *anim, float *out) {
  if (!anim || !out)
    return LTGUI_ERR_NULL_PTR;
  *out = anim->anim.target();
  return LTGUI_OK;
}

int ltgui_animation_on_finished(ltgui_animation *anim, void (*cb)(void *),
                                void *userdata) {
  LTGUI_TRY_BEGIN
  if (!anim)
    return LTGUI_ERR_NULL_PTR;
  anim->anim.onFinished.connect(
      [cb, userdata](void) { if (cb) cb(userdata); });
  return LTGUI_OK;
  LTGUI_TRY_END
}

void ltgui_animated_float_destroy(ltgui_animation *anim) { delete anim; }
