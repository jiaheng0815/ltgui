# ltgui

**[English](#english) | [中文](#中文)**

---

# English

A from-scratch, cross-platform retained-mode GUI framework written in C++17. Zero external dependencies beyond platform APIs.

## Features

- **Cross-platform** — Windows (GDI+), Linux/X11, macOS/Cocoa (stub)
- **17 built-in widgets** — Button, Label, TextBox, CheckBox, RadioButton, Slider, ListBox, ScrollArea, ComboBox, Image, ProgressBar, TabWidget, Tooltip, TreeView
- **Retained widget tree** — classical OOP widget hierarchy with parent-child relationships
- **Layout system** — `BoxLayout` (horizontal/vertical) and `GridLayout` with stretch factors
- **Style system** — per-widget styling: colors, fonts, borders, padding, margins, rounded corners
- **Theme system** — built-in Light and Dark themes, switchable globally with automatic repaint
- **Animation system** — `AnimatedFloat` with Linear/EaseIn/EaseOut/EaseInOut easing, auto-drives repaint loop
- **Dirty-rect painting** — only repaints areas that actually changed, minimizing draw overhead
- **Event system** — event routing via hit-testing, focus management for keyboard input
- **Canvas transforms** — `Canvas` class with save/restore, translate, and clip-rect support
- **UTF-8 support** — full Unicode text rendering and input via GDI+ with proper IME handling
- **Font & image caching** — fonts and images loaded once and reused across draw calls
- **Python build system** — single `ltgui.py` script for compilation, no CMake required

## Quick Start

### Prerequisites

- **clang++** (C++17) or MSVC
- **Python 3**
- Windows: MSYS2 with clang64 toolchain recommended

### Build & Run

```bash
# Debug build
python ltgui.py build

# Release build
python ltgui.py build release

# Build and run an example
python ltgui.py run hello
python ltgui.py run demo

# Clean build artifacts
python ltgui.py clean
```

### Minimal Example

```cpp
#include "ltgui.h"
using namespace ltgui;

int main() {
    Window window;
    window.create(400, 300, "Hello ltgui");

    auto* root = new Widget();
    auto* layout = new BoxLayout(BoxLayout::TopToBottom, 8, 12);

    auto* label = new Label("Welcome to ltgui!");
    label->style().font = Font("Segoe UI", 18, FontWeight::Bold);

    auto* button = new Button("Click Me!");
    int count = 0;
    button->onClick([&]() {
        count++;
        button->setText("Clicked: " + std::to_string(count) + " times");
    });

    root->addChild(label);
    root->addChild(button);
    root->setLayout(layout);

    window.setCentralWidget(root);
    window.show();
    return Application::instance().run();
}
```

## Architecture

```
+-------------------------------------------------+
|                   Application                   |
+-------------------------------------------------+
|  Widget Tree  |  Layout    |  Style/Theme       |
|  (retained)   |  (HBox,    |  (colors, fonts,   |
|               |   VBox,    |   padding, themes) |
|               |   Grid)    |                    |
+-------------------------------------------------+
|               Event System                      |
|    (mouse, key, paint, resize, focus)           |
+-------------------------------------------------+
|         Animation & Canvas Layer                |
|    (AnimatedFloat, Easing, Canvas transforms)   |
+-------------------------------------------------+
|          Platform Abstraction Layer             |
|  +----------+----------+-------------------+    |
|  |  Win32   |   X11    |      Cocoa        |    |
|  |  (GDI+)  |  (Xft)   |     (stub)        |    |
|  +----------+----------+-------------------+    |
+-------------------------------------------------+
```

### Widget Tree

Each widget has a parent and children list. Layout negotiation follows the pattern:
`sizeHint()` → `setGeometry()` → `paint()`.

`sizeHint()` results are **cached** and only recomputed when content changes (e.g. `setText()`).

### Event Flow

Mouse events route from root to target widget through hit-testing in reverse z-order.
Keyboard events are delivered directly to the focus widget.

### Dirty-Rect Painting

When a widget calls `update()`, only its bounding rectangle is marked dirty.
On the next paint cycle, widgets that do not intersect the accumulated dirty region are
skipped entirely, reducing unnecessary draw calls.

### Coordinate System

Widget `geometry()` stores position relative to parent. `absoluteRect()` computes
window-absolute coordinates by walking the parent chain. The `Canvas` class adds
a translation stack (`save()`/`restore()`/`translate()`) for nested coordinate transforms.

## Widget Reference

### Core Widgets

| Widget | Description |
|--------|-------------|
| `Button` | Clickable button with text, hover/press state, and onClick callback |
| `Label` | Static text display with alignment |
| `TextBox` | Single-line UTF-8 text input with cursor, selection, and IME support |
| `CheckBox` | Toggleable check box with label and onToggled callback |
| `RadioButton` | Mutually-exclusive radio button (auto-unchecks siblings) |
| `Slider` | Draggable value slider with customizable range |
| `ListBox` | Scrollable item list with keyboard navigation and animated scrolling |
| `ScrollArea` | Scrollable content container with scrollbar |
| `ComboBox` | Drop-down selection list (auto-flips direction near window edge) |

### Extended Widgets

| Widget | Description |
|--------|-------------|
| `Image` | Image display with fit modes: contain, stretch, cover (fill-crop) |
| `ProgressBar` | Determinate bar and indeterminate sliding animation |
| `TabWidget` | Tabbed container with tab bar and per-tab content pane |
| `Tooltip` | Popup tooltip (static show helper, auto-cleans stale instances) |
| `TreeView` | Hierarchical tree with expand/collapse, selection, and scroll |

## Theme

```cpp
// Switch to dark theme — all existing windows repaint automatically
setTheme(Theme::Dark());

// Or set a custom theme
Theme t;
t.accent = Color(0, 140, 235);
t.bgPrimary = Color(240, 240, 240);
t.textPrimary = Color(30, 30, 30);
setTheme(t);
```

Predefined theme colors include: `bgPrimary`, `bgSecondary`, `bgTertiary`,
`textPrimary`, `textSecondary`, `textDisabled`, `accent`, `accentHover`,
`accentPressed`, `border`, `borderFocus`, `scrollbarTrack`, `scrollbarThumb`,
`selectionBg`.

## Styling

```cpp
Style style;
style.bgColor = Color::White;
style.fgColor = Color::Black;
style.borderColor = Color::Gray;
style.borderWidth = 1;
style.borderRadius = 3;
style.font = Font("Segoe UI", 14, FontWeight::Bold);
style.setPadding(8, 4);   // horizontal, vertical
style.setMargin(4);

widget->setStyle(style);
```

## Animation

```cpp
// AnimatedFloat — drives smooth value transitions
AnimatedFloat opacity(0.0f);
opacity.setTarget(1.0f, 300, Easing::EaseOut);  // 300ms ease-out

// In your paint/update loop:
float currentOpacity = opacity.value();

// Easing functions available: Linear, EaseIn, EaseOut, EaseInOut
```

The `AnimationManager` singleton automatically ticks each frame. When any animation
is active, the event loop runs at full speed; when idle, it sleeps efficiently.

## Image & Pixel Buffer

```cpp
// Load from file
auto* img = new Image();
img->load("picture.png");
img->setFitMode('c');  // 'c' = contain, 's' = stretch, 'f' = fill (cover)

// Draw raw pixel buffer (RGBA)
canvas->drawPixelBuffer(pixelData, width, height, targetRect);
```

Images loaded from disk are **cached** — repeated `drawImage()` calls to the same path
hit the in-memory cache and avoid disk I/O.

## Layout

```cpp
// Horizontal box layout
auto* hbox = new BoxLayout(BoxLayout::LeftToRight, spacing, margin);
hbox->addStretch(1);  // stretch factor for child 0

// Vertical box layout
auto* vbox = new BoxLayout(BoxLayout::TopToBottom, spacing, margin);

// Grid layout
auto* grid = new GridLayout(columns, rowSpacing, colSpacing, margin);
grid->setColumnStretch(0, 2);

container->setLayout(hbox);
```

## Project Structure

```
ltgui/
├── ltgui.py                    # Build script
├── include/ltgui/              # Public headers
│   ├── ltgui.h                 #   Umbrella header
│   ├── widget.h                #   Base widget class
│   ├── window.h                #   Window class
│   ├── layout.h                #   Layout engines
│   ├── style.h                 #   Style system
│   ├── theme.h                 #   Theme (Light/Dark)
│   ├── animation.h             #   Animation & easing
│   ├── app.h                   #   Application/event loop
│   ├── event.h                 #   Event types
│   ├── geometry.h              #   Point, Size, Rect
│   ├── color.h                 #   Color type
│   ├── font.h                  #   Font description
│   ├── utf8.h                  #   UTF-8 utilities
│   ├── canvas.h                #   Canvas with transforms
│   ├── platform/               #   Platform abstraction
│   │   ├── platform.h
│   │   ├── native_window.h
│   │   ├── native_canvas.h
│   │   ├── win32/              #   Windows (GDI+) backend
│   │   ├── x11/                #   Linux (X11+Xft) backend
│   │   └── cocoa/              #   macOS (Cocoa) stub
│   └── widgets/                #   Built-in widgets (17)
│       ├── button.h            ├── checkbox.h
│       ├── combobox.h          ├── image.h
│       ├── label.h             ├── listbox.h
│       ├── progressbar.h       ├── radiobutton.h
│       ├── scrollarea.h        ├── slider.h
│       ├── tabwidget.h         ├── textbox.h
│       ├── tooltip.h           └── treeview.h
├── src/                        # Implementation
│   ├── platform/
│   │   ├── win32/
│   │   │   ├── win32_window.cpp
│   │   │   └── win32_canvas.cpp
│   │   ├── x11/
│   │   │   ├── x11_window.cpp
│   │   │   └── x11_canvas.cpp
│   │   └── cocoa/
│   │       ├── cocoa_window.mm
│   │       └── cocoa_canvas.mm
│   └── widgets/                #   (one .cpp per widget)
└── examples/                   # Example programs
    ├── hello.cpp
    └── demo.cpp
```

## License

MIT

---
# 中文

一个从零构建的跨平台保留模式 GUI 框架，使用 C++17 编写。除平台 API 外零外部依赖。

## 特性

- **跨平台** — Windows (GDI+)，Linux/X11，macOS/Cocoa（桩）
- **17 个内置控件** — Button、Label、TextBox、CheckBox、RadioButton、Slider、ListBox、ScrollArea、ComboBox、Image、ProgressBar、TabWidget、Tooltip、TreeView
- **保留模式控件树** — 经典面向对象控件层级，父子关系
- **布局系统** — `BoxLayout`（水平/垂直）和 `GridLayout`，支持拉伸因子
- **样式系统** — 每个控件独立样式：颜色、字体、边框、内边距、外边距、圆角
- **主题系统** — 内置亮色/暗色主题，全局切换自动重绘
- **动画系统** — `AnimatedFloat` 支持 Linear/EaseIn/EaseOut/EaseInOut 缓动，自动驱动重绘循环
- **脏矩形绘制** — 仅重绘实际变化的区域，减少无效绘制调用
- **事件系统** — 通过命中测试进行事件路由，焦点管理处理键盘输入
- **Canvas 变换** — `Canvas` 类支持 save/restore、translate、clip-rect
- **UTF-8 支持** — 通过 GDI+ 实现完整 Unicode 文本渲染和输入，支持输入法
- **字体和图片缓存** — 字体和图片只加载一次，后续调用复用内存缓存
- **Python 构建系统** — 单个 `ltgui.py` 脚本编译，无需 CMake

## 快速开始

### 环境要求

- **clang++** (C++17) 或 MSVC
- **Python 3**
- Windows: 推荐使用 MSYS2 的 clang64 工具链

### 编译运行

```bash
# 调试编译
python ltgui.py build

# 发布编译
python ltgui.py build release

# 编译并运行示例
python ltgui.py run hello
python ltgui.py run demo

# 清理编译产物
python ltgui.py clean
```

### 最小示例

```cpp
#include "ltgui.h"
using namespace ltgui;

int main() {
    Window window;
    window.create(400, 300, "Hello ltgui");

    auto* root = new Widget();
    auto* layout = new BoxLayout(BoxLayout::TopToBottom, 8, 12);

    auto* label = new Label("Welcome to ltgui!");
    label->style().font = Font("Segoe UI", 18, FontWeight::Bold);

    auto* button = new Button("Click Me!");
    int count = 0;
    button->onClick([&]() {
        count++;
        button->setText("Clicked: " + std::to_string(count) + " times");
    });

    root->addChild(label);
    root->addChild(button);
    root->setLayout(layout);

    window.setCentralWidget(root);
    window.show();
    return Application::instance().run();
}
```

## 架构

```
+-------------------------------------------------+
|                   Application                   |
+-------------------------------------------------+
|  Widget Tree  |  Layout    |  Style/Theme       |
|  (retained)   |  (HBox,    |  (colors, fonts,   |
|               |   VBox,    |   padding, themes) |
|               |   Grid)    |                    |
+-------------------------------------------------+
|               Event System                      |
|    (mouse, key, paint, resize, focus)           |
+-------------------------------------------------+
|         Animation & Canvas Layer                |
|    (AnimatedFloat, Easing, Canvas transforms)   |
+-------------------------------------------------+
|          Platform Abstraction Layer             |
|  +----------+----------+-------------------+    |
|  |  Win32   |   X11    |      Cocoa        |    |
|  |  (GDI+)  |  (Xft)   |     (stub)        |    |
|  +----------+----------+-------------------+    |
+-------------------------------------------------+
```

### 控件树

每个控件有父控件和子控件列表。布局协商流程：`sizeHint()` → `setGeometry()` → `paint()`。

`sizeHint()` 结果已**缓存**，仅在内容变更时重新计算（例如 `setText()`）。

### 事件流

鼠标事件通过命中测试按逆 Z 序从根控件路由到目标控件。键盘事件直接发送到焦点控件。

### 脏矩形绘制

控件调用 `update()` 时，仅其边界矩形被标记为脏区域。
下一次绘制循环中，与累积脏区域不相交的控件会被完全跳过，减少不必要的绘制调用。

### 坐标系统

控件 `geometry()` 存储相对于父控件的位置。`absoluteRect()` 通过遍历父链计算窗口绝对坐标。
`Canvas` 类提供平移栈（`save()`/`restore()`/`translate()`）以支持嵌套坐标变换。

## 控件参考

### 基础控件

| 控件 | 描述 |
|------|------|
| `Button` | 可点击按钮，带文字、悬停/按下状态和 onClick 回调 |
| `Label` | 静态文本显示，支持对齐 |
| `TextBox` | 单行 UTF-8 文本输入，支持光标、选择和输入法 |
| `CheckBox` | 可切换复选框，带标签和 onToggled 回调 |
| `RadioButton` | 互斥单选按钮（自动取消同组其他） |
| `Slider` | 可拖动数值滑块，支持自定义范围 |
| `ListBox` | 可滚动列表，支持键盘导航和动画滚动 |
| `ScrollArea` | 可滚动内容容器，带滚动条 |
| `ComboBox` | 下拉选择列表（靠近窗口边缘时自动反向弹出） |

### 扩展控件

| 控件 | 描述 |
|------|------|
| `Image` | 图片显示，支持 contain/stretch/cover 适配模式 |
| `ProgressBar` | 确定进度条和不确定滑动动画 |
| `TabWidget` | 选项卡容器，带标签栏和每个标签的内容面板 |
| `Tooltip` | 弹出工具提示（静态 show 助手，自动清理旧实例） |
| `TreeView` | 层级树形视图，支持展开/折叠、选择和滚动 |

## 主题

```cpp
// 切换到暗色主题 — 所有已有窗口自动重绘
setTheme(Theme::Dark());

// 或自定义主题
Theme t;
t.accent = Color(0, 140, 235);
t.bgPrimary = Color(240, 240, 240);
t.textPrimary = Color(30, 30, 30);
setTheme(t);
```

预定义主题颜色：`bgPrimary`、`bgSecondary`、`bgTertiary`、`textPrimary`、`textSecondary`、
`textDisabled`、`accent`、`accentHover`、`accentPressed`、`border`、`borderFocus`、
`scrollbarTrack`、`scrollbarThumb`、`selectionBg`。

## 样式

```cpp
Style style;
style.bgColor = Color::White;
style.fgColor = Color::Black;
style.borderColor = Color::Gray;
style.borderWidth = 1;
style.borderRadius = 3;
style.font = Font("Segoe UI", 14, FontWeight::Bold);
style.setPadding(8, 4);   // 水平, 垂直
style.setMargin(4);

widget->setStyle(style);
```

## 动画

```cpp
// AnimatedFloat — 驱动平滑数值过渡
AnimatedFloat opacity(0.0f);
opacity.setTarget(1.0f, 300, Easing::EaseOut);  // 300ms 缓出

// 在绘制/更新循环中：
float currentOpacity = opacity.value();

// 可用的缓动函数：Linear, EaseIn, EaseOut, EaseInOut
```

`AnimationManager` 单例每帧自动 tick。有动画活跃时事件循环全速运行，空闲时高效休眠。

## 图片与像素缓冲

```cpp
// 从文件加载
auto* img = new Image();
img->load("picture.png");
img->setFitMode('c');  // 'c' = 适应, 's' = 拉伸, 'f' = 填充

// 绘制原始像素缓冲 (RGBA)
canvas->drawPixelBuffer(pixelData, width, height, targetRect);
```

从磁盘加载的图片已被**缓存** — 重复调用相同路径的 `drawImage()` 会命中内存缓存，避免磁盘 I/O。

## 布局

```cpp
// 水平盒子布局
auto* hbox = new BoxLayout(BoxLayout::LeftToRight, spacing, margin);
hbox->addStretch(1);  // 子控件 0 的拉伸因子

// 垂直盒子布局
auto* vbox = new BoxLayout(BoxLayout::TopToBottom, spacing, margin);

// 网格布局
auto* grid = new GridLayout(columns, rowSpacing, colSpacing, margin);
grid->setColumnStretch(0, 2);

container->setLayout(hbox);
```

## 项目结构

```
ltgui/
├── ltgui.py                    # 构建脚本
├── include/ltgui/              # 公开头文件
│   ├── ltgui.h                 #   总头文件
│   ├── widget.h                #   控件基类
│   ├── window.h                #   窗口类
│   ├── layout.h                #   布局引擎
│   ├── style.h                 #   样式系统
│   ├── app.h                   #   应用程序/事件循环
│   ├── event.h                 #   事件类型
│   ├── geometry.h              #   Point, Size, Rect
│   ├── color.h                 #   颜色类型
│   ├── font.h                  #   字体描述
│   ├── utf8.h                  #   UTF-8 工具函数
│   ├── canvas.h                #   画布抽象（含变换）
│   ├── theme.h                 #   主题（亮色/暗色）
│   ├── animation.h             #   动画和缓动
│   ├── platform/               #   平台抽象
│   │   ├── platform.h
│   │   ├── native_window.h
│   │   ├── native_canvas.h
│   │   ├── win32/              #   Windows (GDI+) 后端
│   │   ├── x11/                #   Linux (X11+Xft) 后端
│   │   └── cocoa/              #   macOS (Cocoa) 桩
│   └── widgets/                #   内置控件 (17 个)
│       ├── button.h            ├── checkbox.h
│       ├── combobox.h          ├── image.h
│       ├── label.h             ├── listbox.h
│       ├── progressbar.h       ├── radiobutton.h
│       ├── scrollarea.h        ├── slider.h
│       ├── tabwidget.h         ├── textbox.h
│       ├── tooltip.h           └── treeview.h
├── src/                        # 实现代码
│   ├── platform/
│   │   ├── win32/
│   │   │   ├── win32_window.cpp
│   │   │   └── win32_canvas.cpp
│   │   ├── x11/
│   │   │   ├── x11_window.cpp
│   │   │   └── x11_canvas.cpp
│   │   └── cocoa/
│   │       ├── cocoa_window.mm
│   │       └── cocoa_canvas.mm
│   └── widgets/                #   (每个控件一个 .cpp)
└── examples/                   # 示例程序
    ├── hello.cpp
    └── demo.cpp
```

## 许可证

MIT
