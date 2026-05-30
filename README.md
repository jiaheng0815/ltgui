# ltgui

**[English](#english) | [中文](#中文)**

---

# English

A from-scratch, cross-platform retained-mode GUI framework written in C++17. Zero external dependencies beyond platform APIs.

## Features

- **Cross-platform** — Windows (GDI+), Linux/X11 (planned), macOS/Cocoa (planned)
- **Retained widget tree** — classical OOP widget hierarchy with parent-child relationships
- **Layout system** — `BoxLayout` (horizontal/vertical) and `GridLayout` with stretch factors
- **Style system** — per-widget styling: colors, fonts, borders, padding, margins
- **Event system** — event bubbling from root to target widget
- **9 built-in widgets** — Button, Label, TextBox, CheckBox, RadioButton, Slider, ListBox, ScrollArea, ComboBox
- **UTF-8 support** — full Unicode text rendering and input via GDI+ with proper IME handling
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
+---------------------------------------------+
|                Application                  |
+---------------------------------------------+
|  Widget Tree  |  Layout    |  Style/Theme   |
|  (retained)   |  (HBox,    |  (colors,      |
|               |   VBox,    |   fonts,       |
|               |   Grid)    |   padding)     |
+---------------------------------------------+
|             Event System                    |
|  (mouse, key, paint, resize, focus)         |
+---------------------------------------------+
|        Platform Abstraction Layer           |
|  +----------+----------+----------------+   |
|  |  Win32   |   X11    |     Cocoa      |   |
|  |  (GDI+)  | (planned)|   (planned)    |   |
|  +----------+----------+----------------+   |
+---------------------------------------------+
```

### Widget Tree

Each widget has a parent and children list. Layout negotiation follows the pattern:
`sizeHint()` → `setGeometry()` → `paint()`.

### Event Flow

Events bubble from the root widget down to the target through hit-testing.
Keyboard events are routed via focus management (`claimFocus()`).

### Coordinate System

Widget `geometry()` stores position relative to parent. `absoluteRect()` computes
window-absolute coordinates by walking the parent chain.

## Widget Reference

| Widget | Description |
|--------|-------------|
| `Button` | Clickable button with text and callback |
| `Label` | Static text display |
| `TextBox` | Single-line text input with UTF-8 support |
| `CheckBox` | Toggleable check box with label |
| `RadioButton` | Exclusive radio button (auto-unchecks siblings) |
| `Slider` | Draggable value slider |
| `ListBox` | Scrollable item list with selection |
| `ScrollArea` | Scrollable content container |
| `ComboBox` | Drop-down selection list |

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
│   ├── app.h                   #   Application/event loop
│   ├── event.h                 #   Event types
│   ├── geometry.h              #   Point, Size, Rect
│   ├── color.h                 #   Color type
│   ├── font.h                  #   Font description
│   ├── canvas.h                #   Canvas abstraction
│   ├── platform/               #   Platform abstraction
│   │   ├── platform.h
│   │   ├── native_window.h
│   │   ├── native_canvas.h
│   │   └── win32/              #   Windows backend
│   └── widgets/                #   Built-in widgets
│       ├── button.h
│       ├── label.h
│       ├── textbox.h
│       ├── checkbox.h
│       ├── radiobutton.h
│       ├── slider.h
│       ├── listbox.h
│       ├── scrollarea.h
│       └── combobox.h
├── src/                        # Implementation
│   ├── platform/win32/
│   │   ├── win32_window.cpp
│   │   └── win32_canvas.cpp
│   └── widgets/
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

- **跨平台** — Windows (GDI+)，Linux/X11（计划中），macOS/Cocoa（计划中）
- **保留模式控件树** — 经典面向对象控件层级，父子关系
- **布局系统** — `BoxLayout`（水平/垂直）和 `GridLayout`，支持拉伸因子
- **样式系统** — 每个控件独立样式：颜色、字体、边框、内边距、外边距
- **事件系统** — 从根控件到目标控件的事件冒泡
- **9 个内置控件** — Button、Label、TextBox、CheckBox、RadioButton、Slider、ListBox、ScrollArea、ComboBox
- **UTF-8 支持** — 通过 GDI+ 实现完整 Unicode 文本渲染和输入，支持输入法
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
+---------------------------------------------+
|                Application                  |
+---------------------------------------------+
|  Widget Tree  |  Layout    |  Style/Theme   |
|  (retained)   |  (HBox,    |  (colors,      |
|               |   VBox,    |   fonts,       |
|               |   Grid)    |   padding)     |
+---------------------------------------------+
|             Event System                    |
|  (mouse, key, paint, resize, focus)         |
+---------------------------------------------+
|        Platform Abstraction Layer           |
|  +----------+----------+----------------+   |
|  |  Win32   |   X11    |     Cocoa      |   |
|  |  (GDI+)  | (planned)|   (planned)    |   |
|  +----------+----------+----------------+   |
+---------------------------------------------+
```

### 控件树

每个控件有父控件和子控件列表。布局协商流程：`sizeHint()` → `setGeometry()` → `paint()`。

### 事件流

事件从根控件通过命中测试向下冒泡到目标控件。键盘事件通过焦点管理（`claimFocus()`）路由。

### 坐标系统

控件 `geometry()` 存储相对于父控件的位置。`absoluteRect()` 通过遍历父链计算窗口绝对坐标。

## 控件参考

| 控件 | 描述 |
|------|------|
| `Button` | 可点击按钮，带文字和回调 |
| `Label` | 静态文本显示 |
| `TextBox` | 单行文本输入，支持 UTF-8 |
| `CheckBox` | 可切换复选框，带标签 |
| `RadioButton` | 互斥单选按钮（自动取消同组其他） |
| `Slider` | 可拖动数值滑块 |
| `ListBox` | 可滚动列表，支持选择 |
| `ScrollArea` | 可滚动内容容器 |
| `ComboBox` | 下拉选择列表 |

## 样式

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
│   ├── canvas.h                #   画布抽象
│   ├── platform/               #   平台抽象
│   │   ├── platform.h
│   │   ├── native_window.h
│   │   ├── native_canvas.h
│   │   └── win32/              #   Windows 后端
│   └── widgets/                #   内置控件
│       ├── button.h
│       ├── label.h
│       ├── textbox.h
│       ├── checkbox.h
│       ├── radiobutton.h
│       ├── slider.h
│       ├── listbox.h
│       ├── scrollarea.h
│       └── combobox.h
├── src/                        # 实现
│   ├── platform/win32/
│   │   ├── win32_window.cpp
│   │   └── win32_canvas.cpp
│   └── widgets/
└── examples/                   # 示例程序
    ├── hello.cpp
    └── demo.cpp
```

## 许可证

MIT
