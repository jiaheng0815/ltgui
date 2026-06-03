# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## Build Commands

```bash
# Python build script (original)
python ltgui.py build               # Debug build (clang++ -g -O0)
python ltgui.py build release       # Release build (clang++ -O2 -DNDEBUG)
python ltgui.py run <name>          # Build + run from examples/ or app/
python ltgui.py clean               # Remove build/ directory
python ltgui.py test                # Build and run all tests

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
- Overrides `widgetType()` returning its `WidgetType` enum value
- Calls `invalidateSizeHint()` when content changes
- Calls `update()` when its visual state changes
- Uses `currentTheme()` colors for theme-aware appearance

**Ownership.** Widgets are heap-allocated. Children are owned by their parent via `std::vector<std::unique_ptr<Widget>>`. `Window` owns the central widget via `unique_ptr`. The destructor of `Widget` automatically clears the window's focus pointer if it was the focused widget.

## Key Patterns

- **Event delivery**: override `handleEvent(Event&)` and return `true` if consumed, `false` for default dispatch (bubble to parent). MouseDown is targeted (only the child under cursor). MouseUp/MouseMove are broadcast so widgets can clear hover states.
- **Hit testing**: override `hitTest(Point)` to return the deepest child at a given coordinate, or `this` if no child matches.
- **Focus**: call `claimFocus()` to request keyboard focus. `Window` tracks one `focusWidget_`. Before dereferencing the focus pointer, `validateFocusWidget()` checks it's still in this window's tree.
- **Theme global**: `currentTheme()` returns the active `Theme` struct; `setTheme()` sets it and triggers repaint of all windows.
- **Widget type check**: use `widget->widgetType() == WidgetType::RadioButton` (or any other type) instead of `dynamic_cast` or ad-hoc virtual methods like the old `isRadioButton()`.
- **Logging**: use `LOG_INFO("category", "format", ...)`, `LOG_ERROR("category", "format", ...)`, etc. from `log.h`. Categories include `"Window"`, `"GPU"`, `"D3D11"`, `"GL"`. In release builds (`-DNDEBUG`), only `Warn` and `Error` levels print.
