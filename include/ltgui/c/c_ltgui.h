// ltgui C SDK — a C23 binding for the whole LTGUI framework.
//
// Conventions
// -----------
//   * Every framework object is an OPAQUE handle. Widget handles are all the
//     same C type (ltgui_widget*); pass any control to any widget function.
//   * Constructors are xxx_create() (return nullptr on failure — call
//     ltgui_last_error()); destructors are xxx_destroy().
//   * Functions return an int status (0 = success, 0 > = error; see
//     LTGUI_ERR_*) or an opaque handle / bool where noted.
//   * Exceptions are caught at the boundary and converted to error codes.
//   * Signals connect a C function pointer + userdata:
//       ltgui_signal_connect(btn, LTGUI_SIGNAL_ON_CLICKED, cb, userdata);
//     Callbacks run on the UI thread with (userdata, arg); `arg` is a pointer
//     to the payload described in comment (int* for value signals, nullptr
//     for argument-less signals).
//
// Compile with a C23 (or later) compiler and the C++20+ library; the header
// is wrapped in extern "C" so C++ translation units may include it too.

#ifndef LTGUI_C_LTGUI_H
#define LTGUI_C_LTGUI_H

#ifdef __cplusplus
extern "C" {
#endif

// --- Opaque handles --------------------------------------------------------
typedef struct ltgui_widget ltgui_widget;  // any control / custom widget
typedef struct ltgui_window ltgui_window;
typedef struct ltgui_layout ltgui_layout;  // BoxLayout or GridLayout by id
typedef struct ltgui_timer ltgui_timer;
typedef struct ltgui_animation ltgui_animation;  // AnimatedFloat

// --- Error codes -----------------------------------------------------------
enum {
  LTGUI_OK = 0,
  LTGUI_ERR_NULL_PTR = -1,      // required argument was nullptr
  LTGUI_ERR_BAD_HANDLE = -2,    // handle does not match the operation
  LTGUI_ERR_BAD_ARGUMENT = -3,  // index/range/format out of bounds
  LTGUI_ERR_FAILED = -4         // framework-level failure (exceptions etc.)
};

[[nodiscard("check the code")]] extern const char *ltgui_last_error(void);

// --- System / app ----------------------------------------------------------
void ltgui_init(void);
// Run the event loop until all windows are closed. Returns the exit code.
[[nodiscard("check the code")]] extern int ltgui_run(void);
void ltgui_quit(void);
// Theme: switch by name ("Light","Dark","DarkBlue","HighContrast",
// "Solarized","Nord"). Repaints all windows.
[[nodiscard("check the code")]] extern int
ltgui_set_theme(const char *name);
[[nodiscard]] extern const char *ltgui_current_theme_name(void);

// --- Window ------------------------------------------------------------------
// Create (and keep) a top-level window. Returns nullptr on failure.
[[nodiscard]] extern ltgui_window *
ltgui_window_create(int width, int height, const char *title);
void ltgui_window_destroy(ltgui_window *win);
[[nodiscard]] extern int
ltgui_window_size(ltgui_window *win, int *width, int *height);
[[nodiscard]] extern int
ltgui_window_set_size(ltgui_window *win, int width, int height);
[[nodiscard]] extern int
ltgui_window_set_title(ltgui_window *win, const char *title);
void ltgui_window_show(ltgui_window *win);
void ltgui_window_hide(ltgui_window *win);
void ltgui_window_close(ltgui_window *win);
void ltgui_window_update(ltgui_window *win);         // repaint the window
// Takes ownership of the widget tree root.
[[nodiscard]] extern int
ltgui_window_set_central_widget(ltgui_window *win, ltgui_widget *root);
[[nodiscard]] extern ltgui_widget *
ltgui_window_central_widget(ltgui_window *win);
// Values: LTGUI_CURSOR_ARROW=0, IBEAM=1, WAIT=2, CROSSHAIR=3, SIZE_WE=4,
// SIZE_NS=5, SIZE_ALL=6, HAND=7, DENIED=8 — matches CursorShape order.
[[nodiscard]] extern int ltgui_window_set_cursor(ltgui_window *win, int cursor);

// --- Widgets -----------------------------------------------------------------
// Root/container widget. parent may be nullptr (attach via add_child or
// set_central_widget). Takes ownership of created widgets through the tree.
[[nodiscard]] extern ltgui_widget *ltgui_widget_create(ltgui_widget *parent);
// The one destroy for every control type (widget tree owns children).
void ltgui_widget_destroy(ltgui_widget *w);
[[nodiscard]] extern ltgui_widget *
ltgui_widget_add_child(ltgui_widget *parent, ltgui_widget *child);
[[nodiscard]] extern int ltgui_widget_remove_child(ltgui_widget *parent,
                                                   ltgui_widget *child);
[[nodiscard]] extern ltgui_widget *ltgui_widget_parent(ltgui_widget *w);
[[nodiscard]] extern int ltgui_widget_child_count(ltgui_widget *w);
// geometry: {x,y,width,height} — output params may be nullptr.
[[nodiscard]] extern int
ltgui_widget_geometry(ltgui_widget *w, int *x, int *y, int *width, int *height);
[[nodiscard]] extern int
ltgui_widget_set_geometry(ltgui_widget *w, int x, int y, int width, int height);
[[nodiscard]] extern int
ltgui_widget_size_hint(ltgui_widget *w, int *width, int *height);
[[nodiscard]] extern int ltgui_widget_set_visible(ltgui_widget *w, bool visible);
[[nodiscard]] extern int ltgui_widget_set_enabled(ltgui_widget *w, bool enabled);
[[nodiscard]] extern int ltgui_widget_raise_to_top(ltgui_widget *w);
// Layout takes ownership (a later set_layout releases the previous one).
[[nodiscard]] extern int
ltgui_widget_set_layout(ltgui_widget *w, ltgui_layout *layout);
[[nodiscard]] extern int ltgui_widget_update(ltgui_widget *w);
// Style shorthand: background ARGB (0xRRGGBBAA is NOT used — value is
// taken as 0xAARRGGBB), border width, border radius, text color. Pass
// 0xFFFFFFFF for "unset" on colors? No — see per-field docs:
[[nodiscard]] extern int ltgui_widget_set_bg_color(ltgui_widget *w,
                                                   unsigned argb8888);
[[nodiscard]] extern int ltgui_widget_set_fg_color(ltgui_widget *w,
                                                   unsigned argb8888);
[[nodiscard]] extern int
ltgui_widget_set_border(ltgui_widget *w, unsigned argb8888, int width,
                        int radius);

// Signals: generic connect. signal is one of the LTGUI_SIGNAL_* ids.
// cb may be nullptr (connects nothing) — use to register/disconnect later
// via ltgui_signal_disconnect_all. Returns an int id >= 0 or a negative
// error code.
[[nodiscard]] extern int
ltgui_signal_connect(ltgui_widget *w, int signal,
                     void (*cb)(void *, void *), void *userdata);
[[nodiscard]] extern int
ltgui_signal_disconnect_all(ltgui_widget *w, int signal);

// Signal ids.
enum {
  LTGUI_SIGNAL_NONE = 0,
  LTGUI_SIGNAL_ON_CLICKED = 1,              // arg: nullptr
  LTGUI_SIGNAL_ON_TOGGLED = 2,              // arg: bool* (new value)
  LTGUI_SIGNAL_ON_VALUE_CHANGED = 3,        // arg: int* (new value)
  LTGUI_SIGNAL_ON_SELECTION_CHANGED = 4,    // arg: int* (new index)
  LTGUI_SIGNAL_ON_TEXT_CHANGED = 5,         // arg: const char* (new text)
  LTGUI_SIGNAL_ON_ROW_SELECTED = 6,         // arg: int* (row)
  LTGUI_SIGNAL_ON_HEADER_CLICKED = 7,       // arg: int* (column)
  LTGUI_SIGNAL_ON_TREE_SELECTION = 8,       // arg: const char* (item text)
  LTGUI_SIGNAL_ON_FINISHED = 9,             // arg: int* (DialogResult)
  LTGUI_SIGNAL_ON_THEME_CHANGED = 10,       // arg: const char* (theme name)
  LTGUI_SIGNAL_ON_LOCALE_CHANGED = 11,      // arg: const char* (locale)
  LTGUI_SIGNAL_ON_ITEMS_CHANGED = 12,       // arg: nullptr
  LTGUI_SIGNAL_ON_ANIM_CYCLE = 13           // arg: nullptr
};

// --- Button / Label ----------------------------------------------------------
[[nodiscard]] extern ltgui_widget *
ltgui_button_create(const char *text, ltgui_widget *parent);
[[nodiscard]] extern int ltgui_button_set_text(ltgui_widget *btn,
                                               const char *text);
[[nodiscard]] extern const char *ltgui_button_text(ltgui_widget *btn);
// Use LTGUI_SIGNAL_ON_CLICKED for the click callback.

[[nodiscard]] extern ltgui_widget *
ltgui_label_create(const char *text, ltgui_widget *parent);
[[nodiscard]] extern int ltgui_label_set_text(ltgui_widget *label,
                                              const char *text);
[[nodiscard]] extern const char *ltgui_label_text(ltgui_widget *label);

// --- TextBox -----------------------------------------------------------------
[[nodiscard]] extern ltgui_widget *
ltgui_textbox_create(const char *text, ltgui_widget *parent);
[[nodiscard]] extern int ltgui_textbox_set_text(ltgui_widget *tb,
                                                const char *text);
[[nodiscard]] extern const char *ltgui_textbox_text(ltgui_widget *tb);
[[nodiscard]] extern int ltgui_textbox_set_multiline(ltgui_widget *tb,
                                                     bool multi);
[[nodiscard]] extern int ltgui_textbox_copy(ltgui_widget *tb);
[[nodiscard]] extern int ltgui_textbox_cut(ltgui_widget *tb);
[[nodiscard]] extern int ltgui_textbox_paste(ltgui_widget *tb);
[[nodiscard]] extern int ltgui_textbox_select_all(ltgui_widget *tb);
[[nodiscard]] extern int ltgui_textbox_undo(ltgui_widget *tb);
[[nodiscard]] extern int ltgui_textbox_redo(ltgui_widget *tb);
// Use LTGUI_SIGNAL_ON_TEXT_CHANGED for text changes.

// --- CheckBox / RadioButton ---------------------------------------------------
[[nodiscard]] extern ltgui_widget *
ltgui_checkbox_create(const char *text, ltgui_widget *parent);
[[nodiscard]] extern int ltgui_checkbox_set_checked(ltgui_widget *cb,
                                                    bool checked);
[[nodiscard]] extern int ltgui_checkbox_is_checked(ltgui_widget *cb);
// Use LTGUI_SIGNAL_ON_TOGGLED for state changes.

[[nodiscard]] extern ltgui_widget *
ltgui_radiobutton_create(const char *text, ltgui_widget *parent);
[[nodiscard]] extern int ltgui_radiobutton_set_checked(ltgui_widget *rb,
                                                       bool checked);
[[nodiscard]] extern int ltgui_radiobutton_is_checked(ltgui_widget *rb);

// --- Slider / ProgressBar (Range) --------------------------------------------
[[nodiscard]] extern ltgui_widget *
ltgui_slider_create(ltgui_widget *parent);
[[nodiscard]] extern int ltgui_slider_set_range(ltgui_widget *sl, int min_,
                                                int max_);
[[nodiscard]] extern int ltgui_slider_set_value(ltgui_widget *sl, int value);
[[nodiscard]] extern int ltgui_slider_value(ltgui_widget *sl);

[[nodiscard]] extern ltgui_widget *
ltgui_progressbar_create(ltgui_widget *parent);
[[nodiscard]] extern int ltgui_progressbar_set_range(ltgui_widget *pb,
                                                     int min_, int max_);
[[nodiscard]] extern int ltgui_progressbar_set_value(ltgui_widget *pb,
                                                     int value);
[[nodiscard]] extern int ltgui_progressbar_set_indeterminate(ltgui_widget *pb,
                                                             bool on);

// --- ListBox / ComboBox (item lists) -----------------------------------------
[[nodiscard]] extern ltgui_widget *ltgui_listbox_create(ltgui_widget *parent);
[[nodiscard]] extern ltgui_widget *ltgui_combobox_create(ltgui_widget *parent);
[[nodiscard]] extern int ltgui_list_add_item(ltgui_widget *list,
                                             const char *item);
[[nodiscard]] extern int ltgui_list_remove_item(ltgui_widget *list, int index);
[[nodiscard]] extern int ltgui_list_clear(ltgui_widget *list);
[[nodiscard]] extern int ltgui_list_count(ltgui_widget *list);
[[nodiscard]] extern const char *ltgui_list_item(ltgui_widget *list, int index);
[[nodiscard]] extern int ltgui_list_current_index(ltgui_widget *list);
[[nodiscard]] extern int ltgui_list_set_current_index(ltgui_widget *list,
                                                      int index);
[[nodiscard]] extern const char *ltgui_combobox_current_text(ltgui_widget *cb);

// --- Image / ScrollArea --------------------------------------------------------
[[nodiscard]] extern ltgui_widget *ltgui_image_create(ltgui_widget *parent);
[[nodiscard]] extern int ltgui_image_load(ltgui_widget *img, const char *path);
// fit: 0=Contain 1=Fill 2=Stretch (FitMode order in image.h).
[[nodiscard]] extern int ltgui_image_set_fit(ltgui_widget *img, int fit);

[[nodiscard]] extern ltgui_widget *
ltgui_scrollarea_create(ltgui_widget *parent);
// Takes ownership of the content widget.
[[nodiscard]] extern int ltgui_scrollarea_set_widget(ltgui_widget *area,
                                                     ltgui_widget *content);
[[nodiscard]] extern ltgui_widget *
ltgui_scrollarea_widget(ltgui_widget *area);

// --- TabWidget -------------------------------------------------------------------
[[nodiscard]] extern ltgui_widget *ltgui_tabwidget_create(ltgui_widget *parent);
[[nodiscard]] extern int ltgui_tabwidget_add_tab(ltgui_widget *tabs,
                                                 const char *label);
[[nodiscard]] extern int ltgui_tabwidget_count(ltgui_widget *tabs);
[[nodiscard]] extern int ltgui_tabwidget_set_current(ltgui_widget *tabs,
                                                     int index);
[[nodiscard]] extern int ltgui_tabwidget_current(ltgui_widget *tabs);
// The page widget for index (owned by the TabWidget); fill it with children.
[[nodiscard]] extern ltgui_widget *
ltgui_tabwidget_tab_content(ltgui_widget *tabs, int index);

// --- TableView (built-in in-memory model) ---------------------------------------
[[nodiscard]] extern ltgui_widget *ltgui_table_create(ltgui_widget *parent);
[[nodiscard]] extern int ltgui_table_add_column(ltgui_widget *table,
                                                const char *title, int width);
[[nodiscard]] extern int ltgui_table_add_row(ltgui_widget *table,
                                             const char *const *cells,
                                             int columnCount);
[[nodiscard]] extern int ltgui_table_row_count(ltgui_widget *table);
[[nodiscard]] extern int ltgui_table_cell_text(ltgui_widget *table, int row,
                                               int col,
                                               const char **outText);
[[nodiscard]] extern int ltgui_table_set_sort(ltgui_widget *table, int col,
                                              bool ascending);
[[nodiscard]] extern int ltgui_table_set_current(ltgui_widget *table, int row);
[[nodiscard]] extern int ltgui_table_current(ltgui_widget *table);
// Use LTGUI_SIGNAL_ON_ROW_SELECTED / ON_HEADER_CLICKED.

// --- TreeView --------------------------------------------------------------------
[[nodiscard]] extern ltgui_widget *ltgui_tree_create(ltgui_widget *parent);
// outItem_text: pointer to an item handle (ltgui_widget* per tree item id is
// not exposed — use the text payload + LTGUI_SIGNAL_ON_TREE_SELECTION).
[[nodiscard]] extern int ltgui_tree_add_item(ltgui_widget *tree,
                                             const char *parentText,
                                             const char *text);
[[nodiscard]] extern int ltgui_tree_set_expanded(ltgui_widget *tree,
                                                 const char *itemText,
                                                 bool expanded);
// --- MenuBar / ContextMenu ---------------------------------------------------------
[[nodiscard]] extern ltgui_widget *ltgui_menubar_create(ltgui_widget *parent);
[[nodiscard]] extern int ltgui_menubar_add_menu(ltgui_widget *bar,
                                                const char *label);
// cb signature: void(*)(void* userdata, void* arg) — arg is nullptr.
[[nodiscard]] extern int
ltgui_menubar_add_item(ltgui_widget *bar, int menuIdx, const char *text,
                       void (*cb)(void *, void *), void *userdata);
[[nodiscard]] extern int ltgui_menubar_add_separator(ltgui_widget *bar,
                                                     int menuIdx);
[[nodiscard]] extern int ltgui_menubar_exit_shortcut(ltgui_widget *bar,
                                                     int menuIdx, int itemIdx,
                                                     const char *shortcut);

[[nodiscard]] extern ltgui_widget *
ltgui_contextmenu_create(ltgui_widget *parent);
[[nodiscard]] extern int ltgui_contextmenu_add_item(ltgui_widget *menu,
                                                    const char *text,
                                                    void (*cb)(void *, void *),
                                                    void *userdata);
[[nodiscard]] extern int ltgui_contextmenu_add_separator(ltgui_widget *menu);
// The menu closes automatically outside clicks / item selection.
[[nodiscard]] extern int ltgui_contextmenu_popup(ltgui_widget *menu, int x,
                                                 int y);
[[nodiscard]] extern int ltgui_contextmenu_dismiss(ltgui_widget *menu);

// --- Dialog family -------------------------------------------------------------------
[[nodiscard]] extern int ltgui_messagebox_show(ltgui_widget *parent,
                                               const char *title,
                                               const char *message,
                                               int buttons, int icon);
// buttons: 1=OK 2=Cancel 4=Yes 8=No (OR together); icon: 0..4
// (None, Info, Warning, Error, Question). Returns DialogResult
// (0=None 1=OK 2=Cancel 3=Yes 4=No).
[[nodiscard]] extern const char *
ltgui_inputdialog_get_text(ltgui_widget *parent, const char *title,
                           const char *label, const char *defaultText,
                           int ok);  // ok=1: success; returned string freed by
                                     // caller via ltgui_string_free
// ── strings created by the API are owned by the caller ─────────────────────
void ltgui_string_free(char *str);

// --- FileDialog -----------------------------------------------------------------------
// mode: 0=OpenFile 1=OpenMultiple 2=SaveFile 3=SelectFolder.
[[nodiscard]] extern ltgui_widget *
ltgui_filedialog_create(ltgui_widget *parent, int mode);
[[nodiscard]] extern int ltgui_filedialog_add_filter(ltgui_widget *dlg,
                                                     const char *name,
                                                     const char *pattern);
[[nodiscard]] extern int ltgui_filedialog_set_default_path(ltgui_widget *dlg,
                                                           const char *path);
[[nodiscard]] extern int ltgui_filedialog_exec(ltgui_widget *dlg);
[[nodiscard]] extern const char *
ltgui_filedialog_selected_path(ltgui_widget *dlg);

// --- Layouts ------------------------------------------------------------------------
// dir: 0=LeftToRight 1=RightToLeft 2=TopToBottom 3=BottomToTop.
[[nodiscard]] extern ltgui_layout *
ltgui_boxlayout_create(int dir, int spacing, int margin);
[[nodiscard]] extern ltgui_layout *
ltgui_gridlayout_create(int columns, int rowSpacing, int colSpacing,
                        int margin);
[[nodiscard]] extern int ltgui_layout_add_stretch(ltgui_layout *layout,
                                                  int factor);
[[nodiscard]] extern int ltgui_layout_set_spacing(ltgui_layout *layout,
                                                  int spacing);
[[nodiscard]] extern int ltgui_layout_set_stretch(ltgui_layout *layout,
                                                  ltgui_widget *child,
                                                  int factor);
void ltgui_layout_destroy(ltgui_layout *layout);  // after set_layout: NO —
// destroy is a no-op once the layout was attached to a widget (widget owns it).

// --- Timer ----------------------------------------------------------------------------
// ms>0; repeating: true fires every ms, false once.
[[nodiscard]] extern ltgui_timer *
ltgui_timer_start(int ms, bool repeating, void (*cb)(void *), void *userdata);
void ltgui_timer_stop(ltgui_timer *timer);
[[nodiscard]] extern int ltgui_timer_single_shot(int ms, void (*cb)(void *),
                                                 void *userdata);

// --- Clipboard / I18n -------------------------------------------------------------------
[[nodiscard]] extern const char *ltgui_clipboard_get_text(void);
[[nodiscard]] extern int ltgui_clipboard_set_text(const char *text);
// i18n: load a JSON translation table (see README i18n section for format)
// into the given locale ("en","zh_CN",...); then tr() returns the
// translated key (or the key itself when missing). Strings are owned by the
// framework — do not free.
[[nodiscard]] extern int ltgui_i18n_add_table(const char *locale,
                                              const char *json);
[[nodiscard]] extern int ltgui_i18n_set_locale(const char *locale);
[[nodiscard]] extern const char *ltgui_i18n_tr(const char *key);
[[nodiscard]] extern const char *ltgui_i18n_tr_n(const char *key, long long n);

// --- Animation (AnimatedFloat) --------------------------------------------------------------
// easing: 0..33 — the Easing enum order in animation.h (Linear,
// EaseIn..., StepEnd). 0=Linear, 21=EaseInBounce, 22=EaseOutBounce,
// 31=StepStart, 32=StepEnd.
[[nodiscard]] extern ltgui_animation *
ltgui_animated_float_create(float initial);
[[nodiscard]] extern int
ltgui_animated_float_set_target(ltgui_animation *anim, float target, int ms,
                                int easing);
[[nodiscard]] extern int
ltgui_animated_float_set_immediate(ltgui_animation *anim, float value);
[[nodiscard]] extern int
ltgui_animated_float_set_loop(ltgui_animation *anim, bool loop, bool yoyo);
[[nodiscard]] extern int
ltgui_animated_float_target(ltgui_animation *anim, float *out);
[[nodiscard]] extern int
ltgui_animation_on_finished(ltgui_animation *anim, void (*cb)(void *),
                            void *userdata);
void ltgui_animated_float_destroy(ltgui_animation *anim);

#ifdef __cplusplus
}  // extern "C"
#endif

#endif  // LTGUI_C_LTGUI_H
