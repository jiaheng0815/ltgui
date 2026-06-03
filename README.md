# ltgui

[![CI](https://github.com/jiaheng0815/ltgui/actions/workflows/ci.yml/badge.svg)](https://github.com/jiaheng0815/ltgui/actions/workflows/ci.yml)
[![License](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

A from-scratch, cross-platform retained-mode GUI framework in C++17.
Zero dependencies beyond platform APIs.
一个从零构建的跨平台保留模式 C++17 GUI 框架，除平台 API 外零外部依赖。

[English](#english) | [中文](#中文)

---

# English

## Features

- **Cross-platform** — Windows (GDI+), Linux (X11+Xft), macOS (Cocoa)
- **GPU acceleration** — D3D11 (Windows) / OpenGL ES 3.0 (Linux), transparent fallback to CPU
- **18 built-in widgets** — Button, Label, TextBox, CheckBox, RadioButton, Slider, ListBox, ScrollArea, ComboBox, Image, ProgressBar, TabWidget, Tooltip, TreeView, ContextMenu, **MenuBar**
- **Retained widget tree** — `sizeHint()` → `setGeometry()` → `paint()` pipeline with hint caching
- **Layout system** — `BoxLayout` (H/V), `GridLayout` with stretch factors
- **Theme system** — Light/Dark themes, global toggle with auto-repaint
- **Animation** — `AnimatedFloat` with Linear/EaseIn/EaseOut/EaseInOut easing
- **Dirty-rect painting** — only repaints changed areas
- **Event system** — targeted MouseDown, broadcast MouseMove/MouseUp, focus management
- **Widget type enum** — `widgetType()` replaces `dynamic_cast` for fast type checks
- **Logging** — `LOG_INFO/ERROR/WARN/DEBUG` with category filtering, release-build level control
- **GPU font atlas** — glyph caching with automatic system font detection
- **UTF-8** — encode/decode, codepoint navigation, surrogate rejection
- **Timer** — `singleShot` and `interval` integrated with event loop
- **Signal** — type-safe callback with `disconnect()` and `ScopedConnection` auto-cleanup
- **Shortcuts** — per-window keyboard shortcut registration
- **Cursor** — per-window cursor shape control (Arrow, IBeam, Wait, Hand, etc.)
- **DPI scaling** — per-window DPI detection with global override
- **Focus chain** — Tab/Shift+Tab navigation in tree order
- **CMake + Python** — dual build system

## Quick Start

### Prerequisites

- **clang++** (C++17) or MSVC
- **Python 3**
- Windows: MSYS2 clang64 recommended

### Build

```bash
# Debug build
python ltgui.py build

# Release build
python ltgui.py build release

# Run tests (15 suites)
python ltgui.py test

# CMake alternative
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
```

### Minimal Example

```cpp
#include "ltgui.h"
using namespace ltgui;

int main() {
    Window window;
    window.create(400, 300, "Hello ltgui");

    auto* root = new Widget();
    root->setLayout(new BoxLayout(BoxLayout::TopToBottom, 8, 12));

    auto* label = root->makeChild<Label>("Welcome!");
    label->style().font = Font("Segoe UI", 18, FontWeight::Bold);

    auto* button = root->makeChild<Button>("Click Me!");
    button->onClick([&] { button->setText("Clicked!"); });

    window.setCentralWidget(root);
    window.show();
    return Application::instance().run();
}
```

---

## Widget Reference

| Widget | Description |
|--------|-------------|
| `Button` | Clickable button with hover/press state, `onClick` callback |
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
| `MenuBar` | Top-level menu bar with drop-down submenus |

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
#include "signal.h"

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
// TabWidget, Image, TreeView, ContextMenu
```

---

## Theme & Style

```cpp
// Switch theme — auto-repaints all windows
setTheme(Theme::Dark());

// Custom theme
Theme t;
t.accent = Color(0, 140, 235);
t.bgPrimary = Color(240, 240, 240);
setTheme(t);

// Per-widget style
Style s;
s.bgColor = Color::White;
s.borderWidth = 1;
s.borderRadius = 4;
s.setPadding(8, 4);   // h, v (negative values clamped to 0)
s.setMargin(4);
widget->setStyle(s);
```

Predefined theme colors: `bgPrimary`, `bgSecondary`, `bgTertiary`, `textPrimary`, `textSecondary`, `textDisabled`, `accent`, `accentHover`, `accentPressed`, `border`, `borderFocus`, `scrollbarTrack`, `scrollbarThumb`, `selectionBg`.

## Animation

```cpp
AnimatedFloat opacity(0.0f);
opacity.setTarget(1.0f, 300, Easing::EaseOut);  // 300ms

float v = opacity.value();  // interpolated each frame

// Easing: Linear, EaseIn, EaseOut, EaseInOut
```

## Layout

```cpp
// Box layout
auto* hbox = new BoxLayout(BoxLayout::LeftToRight, spacing, margin);
hbox->addStretch(1);

auto* vbox = new BoxLayout(BoxLayout::TopToBottom, spacing, margin);

// Grid layout
auto* grid = new GridLayout(columns, rowSpacing, colSpacing, margin);
grid->setColumnStretch(0, 2);

container->setLayout(hbox);
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
├── include/ltgui/
│   ├── ltgui.h                       # Umbrella
│   ├── widget.h, window.h, app.h     # Core
│   ├── layout.h, style.h, theme.h    # Layout & styling
│   ├── event.h, shortcut.h           # Input
│   ├── animation.h, timer.h          # Time
│   ├── signal.h, log.h               # Utilities
│   ├── geometry.h, color.h, font.h   # Primitives
│   ├── utf8.h                        # UTF-8
│   ├── canvas.h                      # Drawing
│   ├── platform/{native_window,native_canvas,platform}.h
│   ├── platform/win32/x11/cocoa/     # CPU backends
│   ├── platform/gpu/                 # GPU renderer (D3D11, GLES3)
│   └── widgets/                      # 18 widgets
├── src/
│   ├── platform/{win32,x11,cocoa,gpu}/
│   ├── widgets/                      # One .cpp per widget
│   ├── widget.cpp, window.cpp, app.cpp, ... 
│   └── timer.cpp
├── test/                             # 15 doctest suites
├── examples/                         # hello, demo
└── app/                              # main
```

## License

MIT

---

# 中文

## 特性

- **跨平台** — Windows (GDI+)，Linux (X11+Xft)，macOS (Cocoa)
- **GPU 加速** — D3D11 (Windows) / OpenGL ES 3.0 (Linux)，失败透明回退 CPU 渲染
- **18 个内置控件** — Button、Label、TextBox、CheckBox、RadioButton、Slider、ListBox、ScrollArea、ComboBox、Image、ProgressBar、TabWidget、Tooltip、TreeView、ContextMenu、**MenuBar**
- **保留模式控件树** — `sizeHint()` → `setGeometry()` → `paint()` 管线，sizeHint 带缓存
- **布局系统** — `BoxLayout`（水平/垂直）、`GridLayout`，支持拉伸因子
- **主题系统** — 亮色/暗色主题，全局切换自动重绘
- **动画** — `AnimatedFloat`，支持 Linear/EaseIn/EaseOut/EaseInOut 缓动
- **脏矩形绘制** — 仅重绘变化区域
- **事件系统** — MouseDown 定向投递，MouseMove/MouseUp 广播，焦点管理
- **控件类型枚举** — `widgetType()` 替代 `dynamic_cast`，O(1) 类型检查
- **日志** — `LOG_INFO/ERROR/WARN/DEBUG`，支持分类过滤，Release 编译仅输出 WARN/ERROR
- **GPU 字体图集** — 字形缓存，自动检测系统字体
- **UTF-8** — 编解码、码点导航、拒绝代理对
- **定时器** — `singleShot` / `interval`，集成事件循环
- **信号** — 类型安全回调，支持 `disconnect()` 和 `ScopedConnection` 自动清理
- **快捷键** — 窗口级键盘快捷键注册
- **光标** — 窗口级光标形状控制（Arrow、IBeam、Wait、Hand 等）
- **DPI 缩放** — 窗口级 DPI 检测 + 全局覆盖
- **焦点链** — Tab/Shift+Tab 按树序导航
- **CMake + Python** — 双构建系统

## 快速开始

### 环境要求

- **clang++** (C++17) 或 MSVC
- **Python 3**
- Windows: 推荐 MSYS2 clang64 工具链

### 编译

```bash
# 调试编译
python ltgui.py build

# 发布编译
python ltgui.py build release

# 运行测试 (15 个测试套件)
python ltgui.py test

# CMake
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
```

### 最小示例

```cpp
#include "ltgui.h"
using namespace ltgui;

int main() {
    Window window;
    window.create(400, 300, "Hello ltgui");

    auto* root = new Widget();
    root->setLayout(new BoxLayout(BoxLayout::TopToBottom, 8, 12));

    auto* label = root->makeChild<Label>("Welcome!");
    label->style().font = Font("Segoe UI", 18, FontWeight::Bold);

    auto* button = root->makeChild<Button>("Click Me!");
    button->onClick([&] { button->setText("Clicked!"); });

    window.setCentralWidget(root);
    window.show();
    return Application::instance().run();
}
```

---

## 控件参考

| 控件 | 描述 |
|------|------|
| `Button` | 可点击按钮，带悬停/按下状态，`onClick` 回调 |
| `Label` | 静态文本显示，支持对齐 |
| `TextBox` | 单行文本输入，光标、选择、剪贴板 |
| `CheckBox` | 可切换复选框，`onToggled(bool)` 回调 |
| `RadioButton` | 互斥单选按钮（自动取消同组其他） |
| `Slider` | 可拖动滑块，点击轨道跳转数值 |
| `ListBox` | 可滚动列表，键盘导航，动画滚动 |
| `ScrollArea` | 可滚动容器，带滚动条和滚轮支持 |
| `ComboBox` | 下拉选择，靠近窗口边缘自动反向弹出，z-order 安全 |
| `Image` | 图片显示，contain/stretch/cover 适配模式 |
| `ProgressBar` | 确定进度条 + 不确定滑动动画 |
| `TabWidget` | 选项卡容器，每个标签独立内容面板 |
| `Tooltip` | 弹出工具提示，`show()` 静态辅助方法 |
| `TreeView` | 层级树形视图，展开/折叠、选择 |
| `ContextMenu` | 右键弹出菜单，支持分隔线 |
| `MenuBar` | 顶级菜单栏，带下拉子菜单 |

---

## 定时器

```cpp
#include "timer.h"

// 500ms 后执行一次
Timer::singleShot(500, []{ LOG_INFO("App", "触发了!"); });

// 每 1000ms 执行，直到手动停止
Timer ticker;
ticker.start(1000, true, []{ updateClock(); });
ticker.stop();  // 取消
```

## 信号（安全回调）

```cpp
#include "signal.h"

Signal<int> onValueChanged;

// 连接，返回 ID 用于后续断开
int id = onValueChanged.connect([](int v) { handle(v); });
onValueChanged.emit(42);
onValueChanged.disconnect(id);

// ScopedConnection: 析构时自动断开
class MyClass {
    ScopedConnection<int> conn_;
public:
    MyClass(Signal<int>* sig) : conn_(sig, sig->connect([this](int v) { onValue(v); })) {}
};
```

## 光标

```cpp
window.setCursor(CursorShape::IBeam);   // 文本输入
window.setCursor(CursorShape::Wait);    // 忙碌
window.setCursor(CursorShape::Hand);    // 可点击
window.setCursor(CursorShape::SizeWE);  // 水平调整
```

可用值: `Arrow`, `IBeam`, `Wait`, `Crosshair`, `SizeWE`, `SizeNS`, `SizeAll`, `Hand`, `Denied`

## 键盘快捷键

```cpp
#include "shortcut.h"

window.registerShortcut(Shortcut(Key::S, KeyModifier::Control), []{ saveFile(); });
window.registerShortcut(Shortcut(Key::Z, KeyModifier::Control), []{ undo(); });
window.unregisterShortcut(Shortcut(Key::S, KeyModifier::Control));
```

## DPI 缩放

```cpp
// 设置全局 DPI 缩放（默认 1.0，Win32 自动检测）
Application::instance().setDpiScale(1.5f);

// 读取当前缩放
float s = Application::instance().dpiScale();
```

## 焦点链 / Tab 顺序

Tab 和 Shift+Tab 按深度优先树序遍历可聚焦控件。重写 `nextFocusWidget()` / `previousFocusWidget()` / `lastFocusableDescendant()` 自定义行为。

## 控件类型

```cpp
if (widget->widgetType() == WidgetType::RadioButton) { ... }
// 枚举值: Base, Button, Label, TextBox, CheckBox, RadioButton,
// Slider, ListBox, ScrollArea, ComboBox, ProgressBar, Tooltip,
// TabWidget, Image, TreeView, ContextMenu
```

---

## 主题与样式

```cpp
// 切换主题 — 所有窗口自动重绘
setTheme(Theme::Dark());

// 自定义主题
Theme t;
t.accent = Color(0, 140, 235);
t.bgPrimary = Color(240, 240, 240);
setTheme(t);

// 单控件样式
Style s;
s.bgColor = Color::White;
s.borderWidth = 1;
s.borderRadius = 4;
s.setPadding(8, 4);   // 水平, 垂直（负值被截断为 0）
s.setMargin(4);
widget->setStyle(s);
```

预定义主题颜色: `bgPrimary`, `bgSecondary`, `bgTertiary`, `textPrimary`, `textSecondary`, `textDisabled`, `accent`, `accentHover`, `accentPressed`, `border`, `borderFocus`, `scrollbarTrack`, `scrollbarThumb`, `selectionBg`。

## 动画

```cpp
AnimatedFloat opacity(0.0f);
opacity.setTarget(1.0f, 300, Easing::EaseOut);  // 300ms

float v = opacity.value();  // 每帧插值

// 缓动: Linear, EaseIn, EaseOut, EaseInOut
```

## 布局

```cpp
// 盒子布局
auto* hbox = new BoxLayout(BoxLayout::LeftToRight, spacing, margin);
hbox->addStretch(1);
auto* vbox = new BoxLayout(BoxLayout::TopToBottom, spacing, margin);

// 网格布局
auto* grid = new GridLayout(columns, rowSpacing, colSpacing, margin);
grid->setColumnStretch(0, 2);

container->setLayout(hbox);
```

## 日志

```cpp
#include "log.h"

LOG_DEBUG("GPU", "编译 shader 中...");
LOG_INFO("Window", "分辨率: %dx%d", w, h);
LOG_WARN("App", "重试 %d/%d", attempt, max);
LOG_ERROR("D3D11", "CreateTexture 失败: 0x%08lx", hr);

// Release 编译 (-DNDEBUG): 只输出 WARN 和 ERROR。
```

## UTF-8

```cpp
#include "utf8.h"

// 编码
utf8::encode(0x4E2D);    // "中" (3 字节)
utf8::encode(0x1F600);   // "😀" (4 字节)
utf8::encode(0xD800);    // "" — 代理对被拒绝

// 导航
utf8::nextPos("A中B", 0);  // 1
utf8::prevPos("A中B", 4);  // 1

// 字节长度
utf8::codePointLen(0xE4);  // 3
```

## GPU 加速

透明 GPU 渲染 + CPU 回退。Windows 用 D3D11，Linux 用 OpenGL ES 3.0。

```cpp
if (window.isGpuAccelerated()) {
    LOG_INFO("App", "GPU 已激活");
}
```

GPU 层包含: `Renderer2D`（延迟绘制命令、按纹理/颜色批排序）、`FontAtlas`（字形缓存）、NDC 正确的顶点着色器。

---

## 架构

```
+-------------------------------------------------+
|                   Application                   |
|  (事件循环, 定时器, DPI, 动画 tick)               |
+-------------------------------------------------+
|  控件树      |  布局       |  样式/主题          |
|  (保留模式)  |  (HBox,     |  (亮色/暗色)        |
|              |   VBox,     |                    |
|              |   Grid)     |                    |
+-------------------------------------------------+
|  事件系统     |  快捷键     |  焦点链             |
+-------------------------------------------------+
|  动画  |  定时器  |  信号  |  日志               |
+-------------------------------------------------+
|            平台抽象层                             |
|  +----------+----------+-------------------+    |
|  |  Win32   |   X11    |      Cocoa        |    |
|  |  GDI+    |  Xft     |  CoreGraphics     |    |
|  |  D3D11   |  GLES3   |                   |    |
|  +----------+----------+-------------------+    |
+-------------------------------------------------+
```

## 项目结构

```
ltgui/
├── ltgui.py                         # 构建脚本
├── CMakeLists.txt                    # CMake 构建
├── CLAUDE.md                         # 开发指南
├── README.md
├── include/ltgui/
│   ├── ltgui.h                       # 总头文件
│   ├── widget.h, window.h, app.h     # 核心
│   ├── layout.h, style.h, theme.h    # 布局和样式
│   ├── event.h, shortcut.h           # 输入
│   ├── animation.h, timer.h          # 时间
│   ├── signal.h, log.h               # 工具
│   ├── geometry.h, color.h, font.h   # 基础类型
│   ├── utf8.h                        # UTF-8
│   ├── canvas.h                      # 绘制
│   ├── platform/{native_window,native_canvas,platform}.h
│   ├── platform/win32/x11/cocoa/     # CPU 后端
│   ├── platform/gpu/                 # GPU 渲染器
│   └── widgets/                      # 18 个控件
├── src/
│   ├── platform/{win32,x11,cocoa,gpu}/
│   ├── widgets/                      # 每个控件一个 .cpp
│   ├── widget.cpp, window.cpp, app.cpp, ... 
│   └── timer.cpp
├── test/                             # 15 个 doctest 测试套件
├── examples/                         # hello, demo
└── app/                              # main
```

## 许可证

MIT
