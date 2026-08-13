# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Python build script (primary)
python ltgui.py build               # Debug build (clang++ -g -O0)
python ltgui.py build release       # Release build (clang++ -O2 -DNDEBUG)
python ltgui.py build -j 4          # Parallel build with 4 jobs
python ltgui.py build --verbose     # Show full compiler output
python ltgui.py build --json        # Machine-readable JSON output for CI
python ltgui.py build --dll ./sdk   # Build shared library + headers into sdk/
python ltgui.py run <name>          # Build + run from examples/ or app/
python ltgui.py clean               # Remove build/ directory
python ltgui.py test                # Build and run all tests

# Development helpers
python ltgui.py info                # Show project structure and statistics
python ltgui.py watch               # Watch files and auto-rebuild on changes
python ltgui.py watch <name>        # Watch + auto-run a specific target
python ltgui.py debug <name>        # Build (debug) + launch with gdb/lldb
python ltgui.py profile <name>      # Build with -pg profiling flags + run

# Code quality
python ltgui.py fmt                 # Run clang-format on all source files
python ltgui.py lint                # Run clang-tidy on all source files

# Scaffolding
python ltgui.py new widget <name>   # Generate widget .h + .cpp boilerplate
python ltgui.py new example <name>  # Generate example .cpp boilerplate
python ltgui.py new app <name>      # Generate app .cpp boilerplate

# Distribution
python ltgui.py install --prefix /opt/ltgui  # Install lib + headers
python ltgui.py package --format zip         # Package SDK as archive

# CMake (alternative)
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build              # Run tests
```

The Python build script auto-detects platform (Windows/Linux/macOS), links the correct platform libraries, and produces a static library (`build/lib/ltgui.lib` or `libltgui.a`). It compiles all `.cpp` and `.mm` files found recursively under `src/`. The CMakeLists.txt provides an alternative build workflow using standard CMake tooling.

Tests use [doctest](https://github.com/doctest/doctest) (vendored at `vendor/doctest/doctest.h`). Each test file under `test/` is a self-contained executable with its own `main()`.

## Architecture

**Retained-mode widget tree.** `Widget` is the base class. Each widget owns a list of children (via `std::unique_ptr`) and an optional `Layout` engine. The layout pipeline is `sizeHint()` → `setGeometry()` → `paint()`. `sizeHint()` results are cached and only recomputed when `invalidateSizeHint()` is called (typically from content mutators like `setText()`).

**Single-threaded event loop.** `Application::instance().run()` pumps the platform event loop. Mouse events route via hit-testing in reverse z-order; keyboard events go directly to the focus widget. The `Application` singleton owns the set of open `Window` objects.

**Dirty-rect repaint.** When a widget calls `update()`, only its bounding rect is accumulated as dirty. On the next paint pass, `Window` iterates the widget tree and skips any widget that does not intersect the dirty region. `Canvas` wraps `NativeCanvas` with a save/restore/translate stack for nested coordinate transforms — widgets push a translation by their geometry origin before painting children.

**Platform abstraction.** `NativeWindow` and `NativeCanvas` are abstract interfaces. Concrete backends live in `src/platform/{win32,x11,cocoa}/`. The correct backend is selected via `#ifdef` in `platform.h`. `NativeCanvas` exposes fill/stroke/text/image primitives that map to GDI+ (Windows), Xft (Linux), or CoreGraphics (macOS).

**GPU rendering layer.** `src/platform/gpu/` contains a self-written 2D GPU renderer with D3D11 and OpenGL ES 3.0 backends. `GpuCanvas` implements `NativeCanvas` and is tried first in `Window::create()`; if GPU init fails, it falls back to the CPU backend transparently. The GPU renderer uses a `Renderer2D` with deferred draw commands sorted by texture and color to minimize state changes, and a `FontAtlas` for glyph caching.

**Widget implementation pattern.** Each widget subclass:
- Has a header in `include/ltgui/widgets/` and implementation in `src/widgets/`
- Overrides `paintSelf(NativeCanvas*)` for custom drawing
- Overrides `handleEvent(Event&)` for input handling
- Declares `LTGUI_DECLARE_WIDGET_TYPE(Name)` (one line) for type checks
- Calls `invalidateSizeHint()` when content changes
- Calls `update()` when its visual state changes
- Paints through `resolvedStyle()` (never `currentTheme()` directly, except
  for component-specific theme fields like scrollbar/table colors)

**Base class layer.** Text-bearing widgets derive `TextWidget` (text_ +
setText + textSizeHint helper); Slider/ProgressBar derive `Range` (value +
clamp + Signal); CheckBox/RadioButton add the `Checkable` mixin; ComboBox/
ListBox add the `ListItems` mixin; ListBox/TreeView add the `ScrollState`
mixin. Mixins are NOT Widget subclasses — combine via multiple inheritance
(`class CheckBox : public TextWidget, public Checkable`).

**Style system.** `Style` holds base colors (transparent = "unset") plus
per-state patches (`style().hovered.bgColor = ...`). `resolvedStyle()`
resolves `state patch > style > current theme` (hover/pressed get the
theme's accentHover/accentPressed automatically). Theme switches are picked
up on the next paint — no re-styling needed. `style().gradient` enables a
linear gradient background. Hover transitions animate via `AnimatedColor`.

**Ownership.** Widgets are heap-allocated. Children are owned by their parent via `std::vector<std::unique_ptr<Widget>>`. `Window` owns the central widget via `unique_ptr`. The destructor of `Widget` automatically clears the window's focus pointer if it was the focused widget.

## Key Patterns

- **Event delivery**: override `handleEvent(Event&)` and return `true` if consumed, `false` for default dispatch (bubble to parent). MouseDown is targeted (only the child under cursor). MouseUp/MouseMove are broadcast so widgets can clear hover states.
- **Hit testing**: override `hitTest(Point)` to return the deepest child at a given coordinate, or `this` if no child matches.
- **Focus**: call `claimFocus()` to request keyboard focus. `Window` tracks one `focusWidget_`. Before dereferencing the focus pointer, `validateFocusWidget()` checks it's still in this window's tree.
- **Theme global**: `ThemeManager::instance().currentTheme()` returns the active `Theme` struct; `setTheme()` / `ThemeManager::instance().setTheme()` sets it, emits `onThemeChanged` signal, and repaints all windows. Six presets: Light, Dark, DarkBlue, HighContrast, Solarized, Nord. Theme struct has 28 color fields. Every Widget subscribes to `onThemeChanged` and repaints, so theme switches are fresh even for transparent-style widgets.
- **Callbacks**: all widget callbacks are public `Signal<T>` members — `connect(cb)` to listen (multiple slots allowed, ScopedConnection auto-disconnects). Single-shot command APIs (Timer, Shortcut) keep std::function; `DropTarget::onDragOver` keeps std::function because it returns bool.
- **Widget type check**: use `widget->widgetType() == WidgetType::RadioButton` (or any other type) instead of `dynamic_cast` or ad-hoc virtual methods like the old `isRadioButton()`.
- **Logging**: use `LOG_INFO("category", "format", ...)`, `LOG_ERROR("category", "format", ...)`, etc. from `log.h`. Categories include `"Window"`, `"GPU"`, `"D3D11"`, `"GL"`. In release builds (`-DNDEBUG`), only `Warn` and `Error` levels print.
- **Layout relayout**: after content changes (setText, etc.), call `scheduleRelayout()` to walk up to the nearest layout-bearing ancestor and re-lay it out. This ensures widgets resize when text changes.
- **Effective geometry**: `effectiveGeometry()` returns the widget's hit-test area in **local** coordinates (origin 0,0). Callers must `translated()` by child position before comparing against parent-space coordinates.
- **FontAtlas dynamic sizing**: TTF data is stored once per (family,weight,style) with size=0. Size-specific caches are created on demand via `ensureFontLoaded()`. Widgets MUST call `canvas->setFont(style().font)` before `canvas->measureText()`.

## New Features (2026-06)

### Animation (`animation.h`)
- 30+ Robert Penner easing functions: Quad/Cubic/Quart/Quint/Sine/Expo/Circ/Back/Elastic/Bounce × In/Out/InOut, plus StepStart/StepEnd
- `AnimatedFloat` extended with `onFinished` signal, `setLoop`, `setRepeatCount`, `setYoyo`
- `WidgetAnimation` — animate any value with duration/easing/delay, play/pause/stop
- `KeyframeAnimation` — keyframe timeline with per-segment easing
- `AnimationManager` drives registered animations each tick

### Internationalization (`i18n.h`)
- `Locale` — language/country/variant, parse from "zh-CN" strings
- `PluralRules::formIndex()` — CLDR plural rules for 20+ languages (en/zh/ru/ar/pl/cs/ro/lt/lv/mt/sl/ga/cy)
- `TranslationTable` — key→value maps with plural form support; loads from flat JSON
- `I18n` — singleton: `setLocale()`, `tr(key)`, `tr(key, n)` for plurals, `onLocaleChanged` signal
- JSON format: `{"ok":"确定","files":["zero","one","two","few","many","other"]}`

### Dialog (`widgets/dialog.h`)
- `Dialog` — modal base class: `exec()` inner event loop, semi-transparent overlay, fade-in animation, Escape to cancel
- `MessageBox` — `show()` static method, Icon (Info/Warning/Error/Question), button flags (OK/Cancel/Yes/No)
- `InputDialog` — `getText()` static method, Enter to confirm
- All dialogs position a content panel internally with BoxLayout — add child widgets to `panel_`

### MenuBar (`widgets/menubar.h`)
- `MenuItem` extended: `shortcut` display, `checkable` + `checked`, `radio` + `radioGroup`, `submenu` vector
- Keyboard navigation: Left/Right switch menus, Up/Down move items, Enter activates, Escape closes
- `setItemShortcut()`, `setItemCheckable()`, `setItemChecked()`, `setItemRadio()`
- `addSubmenu()` / `addSubItem()` for nested submenus

### TableView (`widgets/tableview.h`)
- `TableModel` — virtual data source (rowCount/columnCount/cellText)
- `SimpleTableModel` — in-memory rows×cols with `sort()` and `addRow()`/`removeRow()`
- `TableView` — column headers, sort indicators (▲▼), resizable columns (drag edge), striped rows, row selection, scroll wheel

### Clipboard (`clipboard.h`)
- `ClipboardData` — multi-format container: text, HTML, RGBA image, file paths
- `Clipboard` — static API: `getText()`, `setText()`, `getData()`, `setData()`, `availableFormats()`
- Platform backends: Win32 `CF_UNICODETEXT` (existing), X11 `CLIPBOARD` atom (existing)

### FileDialog (`widgets/filedialog.h`)
- Inherits `Dialog`. Modes: OpenFile, OpenMultiple, SaveFile, SelectFolder
- `FileFilter` with name+pattern. Win32 FindFirstFile/FindNextFile directory scanning.
- Custom UI: path bar (TextBox), file list (ListBox), filter combo (ComboBox), Open/Cancel buttons

### Drag & Drop (`dragdrop.h`)
- `DragData` — MIME-typed byte blobs: `setText()`, `setFiles()`, `hasFormat()`, `data()`
- `DragSource` — attach to widget: `setDragData()`, `addMimeType()`
- `DropTarget` — attach to widget: `setAcceptedMimeTypes()`, `onDrop()`, `onDragOver()`
- Event types: `DragEnter`, `DragMove`, `DragLeave`, `DragDrop`
