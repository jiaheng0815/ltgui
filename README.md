# ltgui

A from-scratch, cross-platform retained-mode GUI framework in C++20.
Zero dependencies beyond platform APIs.

[中文 (Chinese)](README_CN.md)

---

## Features

- **Cross-platform** — Windows (GDI+/D3D11), Linux (X11+Xft/GLES3), macOS (Cocoa)
- **GPU acceleration** — D3D11 (Windows) / OpenGL ES 3.0 (Linux), transparent fallback to CPU
- **21 built-in widgets** — Button, Label, TextBox, CheckBox, RadioButton, Slider, ListBox, ScrollArea, ComboBox, Image, ProgressBar, TabWidget, Tooltip, TreeView, ContextMenu, MenuBar, **TableView**, **Dialog/MessageBox/InputDialog**, **FileDialog**
- **Retained widget tree** — `sizeHint()` → `setGeometry()` → `paint()` pipeline with hint caching
- **Layout system** — `BoxLayout` (H/V), `GridLayout` with stretch factors, auto-relayout via `scheduleRelayout()`
- **Theme system** — 6 presets (Light/Dark/DarkBlue/HighContrast/Solarized/Nord), `ThemeManager` with change signal, 28 color fields, custom theme support
- **Animation** — `AnimatedFloat` with 30+ Robert Penner easing functions, `WidgetAnimation`, `KeyframeAnimation`, loop/yoyo
- **Dirty-rect painting** — only repaints changed areas
- **Event system** — targeted MouseDown, broadcast MouseMove/MouseUp, focus management, drag-drop events
- **Widget type enum** — `widgetType()` replaces `dynamic_cast` for fast type checks
- **Logging** — `LOG_INFO/ERROR/WARN/DEBUG` with category filtering, release-build level control
- **GPU font atlas** — glyph caching with dynamic on-demand size rasterization from stored TTF data
- **UTF-8** — encode/decode, codepoint navigation, surrogate rejection
- **Timer** — `singleShot` and `interval` integrated with event loop
- **Signal** — type-safe callback with `disconnect()` and `ScopedConnection` auto-cleanup
- **Shortcuts** — per-window keyboard shortcut registration
- **Cursor** — per-window cursor shape control (Arrow, IBeam, Wait, Hand, etc.)
- **DPI scaling** — per-window DPI detection with global override
- **Focus chain** — Tab/Shift+Tab navigation in tree order
- **Internationalization** — `I18n` singleton, 20+ language plural rules, JSON translation files
- **Clipboard** — multi-format (Text/HTML/Image/Files), platform abstraction
- **Drag & Drop** — app-internal MIME-typed data transfer, `DragSource`/`DropTarget`
- **CMake + Python** — dual build system

## Quick Start

### Prerequisites

- **C++20 compiler** — clang++, MSVC, or g++
- **Python 3**

### Build

```bash
# Debug build (auto-detects clang++ or MSVC on Windows)
python ltgui.py build

# With MSVC (Visual Studio Developer Command Prompt)
python ltgui.py build --compiler msvc

# With clang
python ltgui.py build --compiler clang

# Release build
python ltgui.py build release

# Run tests (24 suites)
python ltgui.py test

# CMake alternative
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
```

### Minimal Example

```cpp
#include "ltgui.h"
#include <iostream>

using namespace ltgui;

int main() {
    Window window;
    if (!window.create(400, 300, "Hello ltgui"))
        return 1;

    auto root = std::make_unique<Widget>();
    auto layout = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 8, 12);

    auto* label = root->makeChild<Label>("Welcome!");
    label->style().font = Font("Segoe UI", 18, FontWeight::Bold);

    auto* button = root->makeChild<Button>("Click Me!");
    button->onClicked.connect([&] { button->setText("Clicked!"); });

    layout->addStretch(0);
    root->setLayout(std::move(layout));
    window.setCentralWidget(std::move(root));
    window.show();

    return Application::instance().run();
}
```

The equivalent minimal example lives in [`examples/hello.cpp`](examples/hello.cpp).

---

## Widget Reference

| Widget | Description |
|--------|-------------|
| `Button` | Clickable button with hover/press state, `onClicked` Signal |
| `Label` | Static text display with alignment |
| `TextBox` | Single-line text input, cursor, selection, clipboard |
| `CheckBox` | Toggleable box with label, `onToggled(bool)` |
| `RadioButton` | Mutually-exclusive radio (auto-unchecks siblings) |
| `Slider` | Draggable thumb, click-to-jump on track |
| `ListBox` | Scrollable item list, keyboard nav, animated scroll |
| `ScrollArea` | Scrollable container with scrollbar and wheel support |
| `ComboBox` | Drop-down selection, auto-flip near window edge, z-order safe |
| `Image` | Image display with contain/stretch/cover fit modes |
| `ProgressBar` | Determinate bar + indeterminate sliding animation |
| `TabWidget` | Tabbed container with per-tab content panels |
| `Tooltip` | Popup tooltip, static `show()` helper |
| `TreeView` | Hierarchical tree with expand/collapse, selection |
| `ContextMenu` | Right-click popup menu with separators |
| `MenuBar` | Top-level menu bar with drop-down submenus, keyboard nav, shortcuts, checkable items |
| `Dialog` | Modal dialog with overlay, fade-in animation, Escape to cancel |
| `MessageBox` | Pre-built dialog: Info/Warning/Error/Question icons, OK/Cancel/Yes/No buttons |
| `InputDialog` | Text input dialog with OK/Cancel, Enter to confirm |
| `TableView` | Data table with sortable columns, resizable headers, row selection, virtual scrolling |
| `FileDialog` | File open/save dialog with directory browsing and file filters |

---

## Timer

```cpp
#include "timer.h"

// Fire once after 500ms
Timer::singleShot(500, []{ LOG_INFO("App", "Fired!"); });

// Fire every 1000ms until stopped
Timer ticker;
ticker.start(1000, true, []{ updateClock(); });
ticker.stop();  // cancel
```

## Signal (Safe Callbacks)

```cpp
#include "signal.hpp"

Signal<int> onValueChanged;

// Connect, get ID for later disconnect
int id = onValueChanged.connect([](int v) { handle(v); });
onValueChanged.emit(42);
onValueChanged.disconnect(id);

// ScopedConnection auto-disconnects on destruction
class MyClass {
    ScopedConnection<int> conn_;
public:
    MyClass(Signal<int>* sig) : conn_(sig, sig->connect([this](int v) { onValue(v); })) {}
};
```

## Cursor Shapes

```cpp
window.setCursor(CursorShape::IBeam);   // text input
window.setCursor(CursorShape::Wait);    // busy
window.setCursor(CursorShape::Hand);    // clickable
window.setCursor(CursorShape::SizeWE);  // horizontal resize
```

Available: `Arrow`, `IBeam`, `Wait`, `Crosshair`, `SizeWE`, `SizeNS`, `SizeAll`, `Hand`, `Denied`

## Keyboard Shortcuts

```cpp
#include "shortcut.h"

window.registerShortcut(Shortcut(Key::S, KeyModifier::Control), []{ saveFile(); });
window.registerShortcut(Shortcut(Key::Z, KeyModifier::Control), []{ undo(); });
window.unregisterShortcut(Shortcut(Key::S, KeyModifier::Control));
```

## DPI Scaling

```cpp
// Set global DPI scale (default 1.0, auto-detected on Win32)
Application::instance().setDpiScale(1.5f);

// Read current scale
float s = Application::instance().dpiScale();
```

## Focus Chain / Tab Order

Tab and Shift+Tab navigate focusable widgets in depth-first tree order. Widgets override `nextFocusWidget()` / `previousFocusWidget()` / `lastFocusableDescendant()` to customize.

## Widget Type

```cpp
if (widget->widgetType() == WidgetType::RadioButton) { ... }
// Enum: Base, Button, Label, TextBox, CheckBox, RadioButton,
// Slider, ListBox, ScrollArea, ComboBox, ProgressBar, Tooltip,
// TabWidget, Image, TreeView, ContextMenu, MenuBar, Dialog,
// MessageBox, InputDialog, TableView, FileDialog
```

---

## Theme & Style

```cpp
// Switch theme — auto-repaints all windows, emits onThemeChanged signal
ThemeManager::instance().setTheme(Theme::Dark());
// or: setTheme(Theme::Dark());

// Available presets
ThemeManager::instance().setThemeByName("Nord");
ThemeManager::instance().setThemeByName("Solarized");
// Built-in: Light, Dark, DarkBlue, HighContrast, Solarized, Nord

// Custom theme (28 color fields)
Theme t;
t.name = "MyTheme";
t.accent = Color(0, 140, 235);
t.bgPrimary = Color(240, 240, 240);
t.dialogBg = Color(255, 255, 255);
t.tableHeaderBg = Color(240, 240, 240);
ThemeManager::instance().registerTheme("MyTheme", t);

// Listen to theme changes
ThemeManager::instance().onThemeChanged.connect([](const Theme& t) {
    LOG_INFO("UI", "Theme changed to: %s", t.name.c_str());
});

// Per-widget style. Colors default to transparent ("unset") — anything
// unset falls back to the CURRENT theme, so theme switches are picked up
// automatically without re-styling widgets.
Style s;
s.bgColor = Color::White;      // explicit color wins
s.borderWidth = 1;
s.borderRadius = 4;
s.setPadding(8, 4);
s.accent = Color(0, 120, 215); // semantic accent (hover/pressed states)
widget->setStyle(s);
```

### State styles (hover / pressed / focused / disabled)

Each widget resolves its effective style from the state it is in:
`state patch > style > theme default`. Override per-state colors:

```cpp
Style s = widget->style();
s.hovered.bgColor = Color(220, 230, 250);   // on hover
s.pressed.accent  = Color(0, 90, 175);      // while pressed
s.disabled.fgColor = Color(180, 180, 180);  // when disabled
widget->setStyle(s);
```

Hover transitions are animated by default (150ms ease-out) — Button
backgrounds and Slider thumbs ease between states automatically.

### Gradients

```cpp
Style s = widget->style();
s.gradient = Gradient{Color(0, 120, 215), Color(0, 200, 255), /*vertical=*/true};
widget->setStyle(s);
```

### Migrating from the legacy callback API

All widget callbacks are now `Signal<T>` members — connect instead of assign:

| Before                          | After                                  |
|---------------------------------|----------------------------------------|
| `btn.onClick([&]{ ... });`      | `btn.onClicked.connect([&]{ ... });`   |
| `slider.onValueChanged(cb);`    | `slider.onValueChanged.connect(cb);`   |
| `cb.onToggled(cb);`             | `cb.onToggled.connect(cb);`            |
| `box.onSelectionChanged(cb);`   | `box.onSelectionChanged.connect(cb);`  |
| `txt.onTextChanged(cb);`        | `txt.onTextChanged.connect(cb);`       |
| `tv.onRowSelected(cb);`         | `tv.onRowSelected.connect(cb);`        |
| `dlg.onFinished(cb);`           | `dlg.onFinished.connect(cb);`          |

Other renames: `selectedIndex()/setSelected()` → `currentIndex()/setCurrentIndex()`,
`TableView::selectedRow()/selectRow()` → `currentIndex()/setCurrentIndex()`,
`Window::getSize()` → `size()`, `Image::setFitMode(char)` → `setFitMode(FitMode)`,
`Style::setMargin()` removed. The deprecated aliases were dropped in v1.0.1 — use the names in the right column.

## Animation

```cpp
// AnimatedFloat (30+ easing functions)
AnimatedFloat opacity(0.0f);
opacity.setTarget(1.0f, 300, Easing::EaseOutBounce);
opacity.setLoop(true);
opacity.setYoyo(true);
opacity.onFinished.connect([]{ LOG_INFO("UI", "Done!"); });
float v = opacity.value();

// WidgetAnimation — animate any value with callback
WidgetAnimation anim;
anim.setStartValue(0.0f);
anim.setEndValue(100.0f);
anim.setDuration(500);
anim.setEasing(Easing::EaseOutCubic);
anim.setValueCallback([&](float v) { widget->setX((int)v); });
anim.onFinished.connect([]{ LOG_INFO("UI", "Animation complete"); });
anim.play();

// KeyframeAnimation
KeyframeAnimation kf;
kf.addKeyframe({0.0f, 0.0f, Easing::EaseOut});
kf.addKeyframe({0.5f, 1.0f, Easing::Linear});
kf.addKeyframe({1.0f, 0.0f, Easing::EaseIn});
kf.setDuration(1000);
kf.setLoop(true);
kf.setValueCallback([&](float v) { updateOpacity(v); });
kf.play();

// Easing: Linear, EaseIn/Out/InOut, Quad, Cubic, Quart, Quint,
//         Sine, Expo, Circ, Back, Elastic, Bounce (each × In/Out/InOut),
//         StepStart, StepEnd
```

## Dialog

```cpp
// MessageBox — modal, returns DialogResult
DialogResult r = MessageBox::show(parent, "Save Changes?",
    "Do you want to save before closing?",
    (int)(DialogButton::Yes | DialogButton::No | DialogButton::Cancel),
    MessageBox::Icon::Question);

// InputDialog — modal, returns text
std::string name = InputDialog::getText(parent,
    "Enter Name", "Please enter your name:", "default");

// Custom Dialog
class MyDialog : public Dialog {
    MyDialog(Widget* parent) : Dialog(parent) {
        setTitle("Custom Dialog");
        panelW_ = 400;
        panelH_ = 200;
        auto* btn = panel_->makeChild<Button>("OK");
        btn->onClicked.connect([this]() { done(DialogResult::OK); });
    }
};
MyDialog dlg(parent);
if (dlg.exec() == DialogResult::OK) { /* handle */ }
```

## TableView

```cpp
// Create model and view
auto model = std::make_shared<SimpleTableModel>(0, 3);
model->addRow({"Alice", "Engineering", "2023"});
model->addRow({"Bob", "Design", "2024"});
model->addRow({"Carol", "Marketing", "2022"});

auto* table = root->makeChild<TableView>();
table->addColumn({"Name", 150});
table->addColumn({"Department", 150});
table->addColumn({"Year", 80, 50, true, false}); // minWidth=50, not sortable
table->setModel(model);

// Sort by clicking header, or programmatically
table->setSortColumn(0, true);
model->sort(0, true);

// Selection
table->onRowSelected.connect([](int row) { LOG_INFO("UI", "Selected row %d", row); });
int sel = table->currentIndex();

// Column resize: drag edge between headers
table->setColumnWidth(0, 200);
```

## Internationalization (i18n)

```cpp
// Setup
I18n::instance().loadTableFromFile(Locale{"zh", "CN", ""}, "lang/zh-CN.json");
I18n::instance().loadTableFromFile(Locale{"en", "US", ""}, "lang/en-US.json");
I18n::instance().setLocale(Locale{"zh", "CN", ""});

// Translate
std::string t = I18n::instance().tr("dialog.ok");       // "确定"
std::string p = I18n::instance().tr("files.count", 5);  // "%d 个文件"

// Locale change notification
I18n::instance().onLocaleChanged.connect([](const Locale& loc) {
    LOG_INFO("UI", "Locale: %s", loc.toString().c_str());
});

// JSON translation file format:
// {"ok":"确定","files":["zero","one","two","few","many","%d 个文件"]}
```

## Clipboard

```cpp
// Plain text
Clipboard::setText("Hello World");
std::string text = Clipboard::getText();

// Rich data
ClipboardData data;
data.setText("Hello");
data.setHtml("<b>Hello</b>");
data.setImage(rgbaPixels, 64, 64);
data.setFiles({"C:/path/file.txt"});
Clipboard::setData(data);

// Read
ClipboardData read = Clipboard::getData();
if (read.hasFormat(ClipboardFormat::Image)) {
    int w = read.imageWidth(), h = read.imageHeight();
    const uint8_t* pixels = read.imageData();
}
```

## FileDialog

```cpp
FileDialog dlg(parent);
dlg.setTitle("Open Image");
dlg.setMode(FileDialogMode::OpenFile);
dlg.addFilter({"Images", "*.png;*.jpg"});
dlg.addFilter({"All Files", "*.*"});

if (dlg.exec()) {
    std::string path = dlg.selectedPath();
    loadImage(path);
}
```

## Drag & Drop

```cpp
// Make a widget draggable
auto* source = root->makeChild<Label>("Drag me");
DragSource dragSrc(source);
dragSrc.addMimeType("text/plain");
auto dragData = std::make_shared<DragData>();
dragData->setText("Dragged text");
dragSrc.setDragData(dragData);

// Make a widget accept drops
auto* target = root->makeChild<Label>("Drop here");
DropTarget dropTgt(target);
dropTgt.setAcceptedMimeTypes({"text/plain"});
dropTgt.onDrop([](const DragData& data) {
    LOG_INFO("UI", "Dropped: %s", data.text().c_str());
});
```

## Layout

```cpp
// Box layout (takes ownership via unique_ptr)
auto layout = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 8, 12);
layout->addStretch(1);
container->setLayout(std::move(layout));

// Grid layout
auto grid = std::make_unique<GridLayout>(2, 4, 4, 8);
grid->setColumnStretch(0, 2);
container2->setLayout(std::move(grid));
```

## Logging

```cpp
#include "log.h"

LOG_DEBUG("GPU", "Compiling shaders...");
LOG_INFO("Window", "Resolution: %dx%d", w, h);
LOG_WARN("App", "Retry %d/%d", attempt, max);
LOG_ERROR("D3D11", "CreateTexture failed: 0x%08lx", hr);

// Release builds (-DNDEBUG): only WARN and ERROR print.
```

## UTF-8

```cpp
#include "utf8.h"

// Encoding
utf8::encode(0x4E2D);    // "中" (3 bytes)
utf8::encode(0x1F600);   // "😀" (4 bytes)
utf8::encode(0xD800);    // "" — surrogates rejected

// Navigation
utf8::nextPos("A中B", 0);  // 1
utf8::prevPos("A中B", 4);  // 1

// Byte length
utf8::codePointLen(0xE4);  // 3
```

## GPU Acceleration

Transparent GPU rendering with CPU fallback. D3D11 on Windows, OpenGL ES 3.0 on Linux.

```cpp
if (window.isGpuAccelerated()) {
    LOG_INFO("App", "GPU active");
}
```

GPU layer includes: `Renderer2D` (deferred draw commands, batch-sorted), `FontAtlas` (glyph caching), NDC-correct vertex shaders.

---

## Architecture

```
+-------------------------------------------------+
|                   Application                   |
|  (run loop, timers, DPI, animation tick)        |
+-------------------------------------------------+
|  Widget Tree  |  Layout    |  Style/Theme       |
|  (retained)   |  (HBox,    |  (light/dark)      |
|               |   VBox,    |                    |
|               |   Grid)    |                    |
+-------------------------------------------------+
|  Event System  |  Shortcuts  |  Focus Chain     |
+-------------------------------------------------+
|  Animation  |  Timer  |  Signal  |  Logging     |
+-------------------------------------------------+
|          Platform Abstraction                   |
|  +----------+----------+-------------------+    |
|  |  Win32   |   X11    |      Cocoa        |    |
|  |  GDI+    |  Xft     |  CoreGraphics     |    |
|  |  D3D11   |  GLES3   |                   |    |
|  +----------+----------+-------------------+    |
+-------------------------------------------------+
```

## Project Structure

```
ltgui/
├── ltgui.py                         # Build script
├── CMakeLists.txt                    # CMake build
├── CLAUDE.md                         # Dev guide
├── README.md
├── CHANGELOG.md
├── docs/                             # Audit & design docs
├── include/ltgui/
│   ├── ltgui.h                       # Umbrella
│   ├── widget.h, window.h, app.h     # Core
│   ├── layout.h, style.h, theme.h    # Layout & styling
│   ├── event.h, shortcut.h           # Input
│   ├── animation.h, timer.h          # Time
│   ├── signal.hpp, log.h               # Utilities
│   ├── geometry.h, color.h, font.h   # Primitives
│   ├── utf8.h                        # UTF-8
│   ├── i18n.h                        # Internationalization
│   ├── clipboard.h                   # Clipboard
│   ├── dragdrop.h                    # Drag & drop
│   ├── canvas.h                      # Drawing
│   ├── platform/{native_window,native_canvas,platform}.h
│   ├── platform/win32/x11/cocoa/     # CPU backends
│   ├── platform/gpu/                 # GPU renderer (D3D11, GLES3)
│   └── widgets/                      # 21 widgets
├── src/
│   ├── i18n.cpp, clipboard.cpp, dragdrop.cpp
│   ├── platform/{win32,x11,cocoa,gpu}/
│   ├── widgets/                      # One .cpp per widget
│   ├── widget.cpp, window.cpp, app.cpp, ... 
│   └── timer.cpp
├── test/                             # 24 doctest suites
├── examples/                         # hello, demo, tableview, dialogs, ...
└── app/                              # main
```

## Build, Install, Package & SDK

### Static library (the default)

`python ltgui.py build` produces the static library `build/lib/ltgui.lib`
(Windows) or `build/lib/libltgui.a` (Linux/macOS), plus the app and all
examples. Consumers **must define `LTGUI_STATIC`** so the public headers do
not assume `__declspec(dllimport)` on Windows:

```bash
clang++ -std=c++20 -DLTGUI_STATIC -I <prefix>/include/ltgui my_app.cpp \
    <prefix>/lib/ltgui.lib -lgdi32 -lgdiplus -luser32 -lcomctl32 -lole32 \
    -limm32 -ld3d11 -ldxgi -ld3dcompiler
```

### Shared library SDK (`--dll`)

```bash
python ltgui.py build --dll ./sdk
```

The `sdk/` directory contains:

| File | Purpose |
|------|---------|
| `ltgui.h` | Amalgamated single-header SDK (defined + smoke-compiled) |
| `ltgui.dll` | Shared library (Windows) |
| `libltgui.dll.a` | Import library to link against the DLL |
| `stb_truetype.h` | Required by the GPU font atlas (shipped with the SDK) |

Consumers include `sdk/ltgui.h` **without** `LTGUI_STATIC` and link the
import library; the SDK header carries the `__declspec(dllimport)` decoration
on Windows automatically. On Linux the header is link-neutral
(`-lltgui` against `libltgui.so`); macOS uses `-dynamiclib` under the hood.

> MSVC (`cl`) is not supported for `--dll` (shared builds need per-symbol
> export markup or a .def); use `--compiler clang` for shared builds or build
> the static library instead.

### Install & package

```bash
python ltgui.py install --prefix /path           # copy lib + all headers
python ltgui.py package                           # build/ltgui-1.0.0-sdk.zip
```

`package` bundles the static library, the amalgamated `ltgui.h` and
`stb_truetype.h` under a versioned name taken from `include/ltgui/version.h`
(the single source of truth; git tags only warn on mismatch).

CMake consumers can use `find_package` directly after a CMake install:

```bash
cmake --install build-cmake --prefix /path/prefix
# consumer CMakeLists.txt:
#   find_package(ltgui REQUIRED)
#   target_link_libraries(app PRIVATE ltgui::ltgui)   # includes LTGUI_STATIC
```

### C SDK (C23)

A pure-C binding covers the whole framework: opaque handles (`ltgui_widget*`,
`ltgui_window*`), `xxx_create()/xxx_destroy()` lifetimes, an int error code
(`ltgui_last_error()` for the message) and C callbacks with a userdata pointer
(`ltgui_signal_connect`). Compile your C code as **C23** and link it through a
C++ driver (the library is C++):

```c
#include "ltgui_c.h"

static void on_clicked(void *userdata, void *arg) {
  (void)arg;
  (* (int *)userdata)++;
}

int main(void) {
  auto win = ltgui_window_create(400, 300, "from C");
  if (!win) return 1;
  auto root = ltgui_widget_create(nullptr);
  auto layout = ltgui_boxlayout_create(2 /*TopToBottom*/, 8, 12);
  ltgui_widget_set_layout(root, layout);
  ltgui_layout_destroy(layout);
  auto btn = ltgui_button_create("Click", root);
  int clicks = 0;
  ltgui_signal_connect(btn, LTGUI_SIGNAL_ON_CLICKED, on_clicked, &clicks);
  ltgui_window_set_central_widget(win, root);
  ltgui_window_show(win);
  return ltgui_run();
}
```

```bash
# Windows / Linux / macOS (clang):
clang -std=c23 -I sdk -c my_app.c
clang++ my_app.o -lltgui       # link with a C++ driver
```

Notes: MSVC's C compiler implements C11, not C23 — use clang/gcc for the C
binding (MSVC can still consume the header from a C++ translation unit).
`examples/c_hello.c` and `test/c_api_test.c` are living examples; the C SDK
header ships in the SDK archive as `ltgui_c.h`.

## License

MIT

---
