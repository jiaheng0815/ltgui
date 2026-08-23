---
---
# LTGUI

[![CI](https://github.com/jiaheng0815/ltgui/actions/workflows/ci.yml/badge.svg)](https://github.com/jiaheng0815/ltgui/actions/workflows/ci.yml)
[![license: MIT](https://img.shields.io/badge/license-MIT-blue.svg)](LICENSE)

一个从零构建的跨平台保留模式 C++20 GUI 框架，除平台 API 外零外部依赖。

[English](README.md)

---


## 特性

- **跨平台** — Windows (GDI+/D3D11)，Linux (X11+Xft/GLES3)，macOS (Cocoa)
- **GPU 加速** — D3D11 (Windows) / OpenGL ES 3.0 (Linux)，失败透明回退 CPU 渲染
- **21 个内置控件** — Button、Label、TextBox、CheckBox、RadioButton、Slider、ListBox、ScrollArea、ComboBox、Image、ProgressBar、TabWidget、Tooltip、TreeView、ContextMenu、MenuBar、**TableView**、**Dialog/MessageBox/InputDialog**、**FileDialog**
- **保留模式控件树** — `sizeHint()` → `setGeometry()` → `paint()` 管线，sizeHint 带缓存
- **布局系统** — `BoxLayout`（水平/垂直）、`GridLayout`，支持拉伸因子，`scheduleRelayout()` 自动重排
- **主题系统** — 6 种预设 (Light/Dark/DarkBlue/HighContrast/Solarized/Nord)，`ThemeManager` 带变更信号，28 色字段，支持自定义主题
- **动画** — `AnimatedFloat` 30+ 种 Robert Penner 缓动函数，`WidgetAnimation`、`KeyframeAnimation`，支持循环/往返
- **脏矩形绘制** — 仅重绘变化区域
- **事件系统** — MouseDown 定向投递，MouseMove/MouseUp 广播，焦点管理，拖放事件
- **控件类型枚举** — `widgetType()` 替代 `dynamic_cast`，O(1) 类型检查
- **日志** — `LOG_INFO/ERROR/WARN/DEBUG`，支持分类过滤，Release 编译仅输出 WARN/ERROR
- **GPU 字体图集** — 字形缓存，动态按需字号光栅化（同一 TTF 数据多字号复用）
- **UTF-8** — 编解码、码点导航、拒绝代理对
- **定时器** — `singleShot` / `interval`，集成事件循环
- **信号** — 类型安全回调，支持 `disconnect()` 和 `ScopedConnection` 自动清理
- **快捷键** — 窗口级键盘快捷键注册
- **光标** — 窗口级光标形状控制（Arrow、IBeam、Wait、Hand 等）
- **DPI 缩放** — 窗口级 DPI 检测 + 全局覆盖
- **焦点链** — Tab/Shift+Tab 按树序导航
- **国际化** — `I18n` 单例，20+ 语言复数规则，JSON 翻译文件
- **剪贴板** — 多格式 (Text/HTML/Image/Files)，平台抽象
- **拖放** — 应用内 MIME 类型数据传输，`DragSource`/`DropTarget`
- **CMake + Python** — 双构建系统

## 快速开始

### 环境要求

- **C++20 编译器** — clang++、MSVC 或 g++
- **Python 3**

### 编译

```bash
# 调试编译（Windows 自动检测 clang++ 或 MSVC）
python ltgui.py build

# 使用 MSVC（在 Visual Studio Developer Command Prompt 中）
python ltgui.py build --compiler msvc

# 使用 clang
python ltgui.py build --compiler clang

# 发布编译
python ltgui.py build release

# 运行测试（24 个测试套件）
python ltgui.py test

# CMake
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build
```

### 最小示例

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

等价的最小示例见 [`examples/hello.cpp`](examples/hello.cpp)。

---

## 控件参考

| 控件 | 描述 |
|------|------|
| `Button` | 可点击按钮，带悬停/按下状态，`onClicked` 信号 |
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
| `MenuBar` | 顶级菜单栏，下拉子菜单、键盘导航、快捷键、可选中项目 |
| `Dialog` | 模态对话框，半透明遮罩、淡入动画、Escape 关闭 |
| `MessageBox` | 预置对话框：Info/Warning/Error/Question 图标，OK/Cancel/Yes/No 按钮 |
| `InputDialog` | 文本输入对话框，OK/Cancel，Enter 确认 |
| `TableView` | 数据表格，可排序列、可调整表头、行选择、虚拟滚动 |
| `FileDialog` | 文件打开/保存对话框，目录浏览、文件过滤器 |

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
#include "signal.hpp"

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
// TabWidget, Image, TreeView, ContextMenu, MenuBar, Dialog,
// MessageBox, InputDialog, TableView, FileDialog
```

---

## 主题与样式

```cpp
// 切换主题 — 所有窗口自动重绘，发出 onThemeChanged 信号
ThemeManager::instance().setTheme(Theme::Dark());
// 或: setTheme(Theme::Dark());

// 可用的预设主题
ThemeManager::instance().setThemeByName("Nord");
ThemeManager::instance().setThemeByName("Solarized");
// 内置: Light, Dark, DarkBlue, HighContrast, Solarized, Nord

// 自定义主题（28 个颜色字段）
Theme t;
t.name = "MyTheme";
t.accent = Color(0, 140, 235);
t.bgPrimary = Color(240, 240, 240);
t.dialogBg = Color(255, 255, 255);
t.tableHeaderBg = Color(240, 240, 240);
ThemeManager::instance().registerTheme("MyTheme", t);

// 监听主题变更
ThemeManager::instance().onThemeChanged.connect([](const Theme& t) {
    LOG_INFO("UI", "主题变更为: %s", t.name.c_str());
});

// 单控件样式
Style s;
s.bgColor = Color::White;
s.borderWidth = 1;
s.borderRadius = 4;
s.setPadding(8, 4);
widget->setStyle(s);
```

### 状态样式（悬停 / 按下 / 聚焦 / 禁用）

每个控件按当前状态解析有效样式，优先级为 `状态补丁 > style > 主题默认`。
逐状态覆盖颜色：

```cpp
Style s = widget->style();
s.hovered.bgColor = Color(220, 230, 250);   // 悬停时
s.pressed.accent  = Color(0, 90, 175);      // 按下时
s.disabled.fgColor = Color(180, 180, 180);  // 禁用时
widget->setStyle(s);
```

悬停过渡默认带动画（150ms ease-out）——Button 背景与 Slider 滑块会在状态间平滑过渡。

### 渐变

```cpp
Style s = widget->style();
s.gradient = Gradient{Color(0, 120, 215), Color(0, 200, 255), /*vertical=*/true};
widget->setStyle(s);
```

### 从旧回调 API 迁移

所有控件回调均为 `Signal<T>` 成员——用 connect 而不是赋值：

| 旧写法                          | 新写法                                    |
|---------------------------------|-------------------------------------------|
| `btn.onClick([&]{ ... });`      | `btn.onClicked.connect([&]{ ... });`      |
| `slider.onValueChanged(cb);`    | `slider.onValueChanged.connect(cb);`      |
| `cb.onToggled(cb);`             | `cb.onToggled.connect(cb);`               |
| `box.onSelectionChanged(cb);`   | `box.onSelectionChanged.connect(cb);`     |
| `txt.onTextChanged(cb);`        | `txt.onTextChanged.connect(cb);`          |
| `tv.onRowSelected(cb);`         | `tv.onRowSelected.connect(cb);`           |
| `dlg.onFinished(cb);`           | `dlg.onFinished.connect(cb);`             |

其他改名：`selectedIndex()/setSelected()` → `currentIndex()/setCurrentIndex()`、
`TableView::selectedRow()/selectRow()` → `currentIndex()/setCurrentIndex()`、
`Window::getSize()` → `size()`、`Image::setFitMode(char)` → `setFitMode(FitMode)`。
`Style::setMargin()` 已移除。这些 deprecated 旧名已在 v1.0.1 中删除——请一律使用右列的新名。

## 动画

```cpp
// AnimatedFloat（30+ 种缓动函数）
AnimatedFloat opacity(0.0f);
opacity.setTarget(1.0f, 300, Easing::EaseOutBounce);
opacity.setLoop(true);
opacity.setYoyo(true);
opacity.onFinished.connect([]{ LOG_INFO("UI", "完成!"); });
float v = opacity.value();

// WidgetAnimation — 带动画回调
WidgetAnimation anim;
anim.setStartValue(0.0f);
anim.setEndValue(100.0f);
anim.setDuration(500);
anim.setEasing(Easing::EaseOutCubic);
anim.setValueCallback([&](float v) { widget->setX((int)v); });
anim.onFinished.connect([]{ LOG_INFO("UI", "动画完成"); });
anim.play();

// KeyframeAnimation — 关键帧动画
KeyframeAnimation kf;
kf.addKeyframe({0.0f, 0.0f, Easing::EaseOut});
kf.addKeyframe({0.5f, 1.0f, Easing::Linear});
kf.addKeyframe({1.0f, 0.0f, Easing::EaseIn});
kf.setDuration(1000);
kf.setLoop(true);
kf.setValueCallback([&](float v) { updateOpacity(v); });
kf.play();

// 缓动函数: Linear, EaseIn/Out/InOut, Quad, Cubic, Quart, Quint,
//         Sine, Expo, Circ, Back, Elastic, Bounce (每种×In/Out/InOut),
//         StepStart, StepEnd
```

## 对话框

```cpp
// MessageBox — 模态，返回 DialogResult
DialogResult r = MessageBox::show(parent, "保存更改?",
    "关闭前是否保存?",
    (int)(DialogButton::Yes | DialogButton::No | DialogButton::Cancel),
    MessageBox::Icon::Question);

// InputDialog — 模态，返回文本
std::string name = InputDialog::getText(parent,
    "输入名称", "请输入您的名字:", "默认值");

// 自定义 Dialog
class MyDialog : public Dialog {
    MyDialog(Widget* parent) : Dialog(parent) {
        setTitle("自定义对话框");
        panelW_ = 400; panelH_ = 200;
        auto* btn = panel_->makeChild<Button>("确定");
        btn->onClicked.connect([this]() { done(DialogResult::OK); });
    }
};
```

## 表格 TableView

```cpp
auto model = std::make_shared<SimpleTableModel>(0, 3);
model->addRow({"张三", "工程部", "2023"});
model->addRow({"李四", "设计部", "2024"});

auto* table = root->makeChild<TableView>();
table->addColumn({"姓名", 120});
table->addColumn({"部门", 120});
table->addColumn({"年份", 60, 40, true, false}); // minWidth=40, 不可排序
table->setModel(model);
table->setSortColumn(0, true);
model->sort(0, true);
table->onRowSelected.connect([](int row) { LOG_INFO("UI", "选中第 %d 行", row); });
```

## 国际化 (i18n)

```cpp
I18n::instance().loadTableFromFile(Locale{"zh", "CN", ""}, "lang/zh-CN.json");
I18n::instance().setLocale(Locale{"zh", "CN", ""});

std::string t = I18n::instance().tr("dialog.ok");        // "确定"
std::string p = I18n::instance().tr("files.count", 5);   // "5 个文件"

// JSON 翻译文件格式:
// {"ok":"确定","files":["零个","一个","两个","几个","很多","%d 个文件"]}
```

## 剪贴板

```cpp
Clipboard::setText("Hello World");
ClipboardData data;
data.setText("Hello");
data.setHtml("<b>Hello</b>");
data.setImage(rgbaPixels, 64, 64);
Clipboard::setData(data);
auto read = Clipboard::getData();
if (read.hasFormat(ClipboardFormat::Image)) { /* 处理图片 */ }
```

## 文件对话框

```cpp
FileDialog dlg(parent);
dlg.setTitle("打开图片");
dlg.addFilter({"图片文件", "*.png;*.jpg"});
if (dlg.exec()) { loadImage(dlg.selectedPath()); }
```

## 拖放

```cpp
auto* source = root->makeChild<Label>("拖我");
DragSource dragSrc(source);
dragSrc.addMimeType("text/plain");
auto data = std::make_shared<DragData>();
data->setText("被拖动的文本");
dragSrc.setDragData(data);

auto* target = root->makeChild<Label>("放这里");
DropTarget dropTgt(target);
dropTgt.setAcceptedMimeTypes({"text/plain"});
dropTgt.onDrop([](const DragData& d) {
    LOG_INFO("UI", "收到: %s", d.text().c_str());
});
```

## 布局

```cpp
// 盒子布局（通过 unique_ptr 转移所有权）
auto layout = std::make_unique<BoxLayout>(BoxLayout::TopToBottom, 8, 12);
layout->addStretch(1);
container->setLayout(std::move(layout));

// 网格布局
auto grid = std::make_unique<GridLayout>(2, 4, 4, 8);
grid->setColumnStretch(0, 2);
container2->setLayout(std::move(grid));
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
├── CHANGELOG.md
├── docs/                             # 审计与设计文档
├── include/ltgui/
│   ├── ltgui.h                       # 总头文件
│   ├── widget.h, window.h, app.h     # 核心
│   ├── layout.h, style.h, theme.h    # 布局和样式
│   ├── event.h, shortcut.h           # 输入
│   ├── animation.h, timer.h          # 时间
│   ├── signal.hpp, log.h               # 工具
│   ├── geometry.h, color.h, font.h   # 基础类型
│   ├── utf8.h                        # UTF-8
│   ├── i18n.h                        # 国际化
│   ├── clipboard.h                   # 剪贴板
│   ├── dragdrop.h                    # 拖放
│   ├── canvas.h                      # 绘制
│   ├── platform/{native_window,native_canvas,platform}.h
│   ├── platform/win32/x11/cocoa/     # CPU 后端
│   ├── platform/gpu/                 # GPU 渲染器
│   └── widgets/                      # 21 个控件
├── src/
│   ├── i18n.cpp, clipboard.cpp, dragdrop.cpp
│   ├── platform/{win32,x11,cocoa,gpu}/
│   ├── widgets/                      # 每个控件一个 .cpp
│   ├── widget.cpp, window.cpp, app.cpp, ... 
│   └── timer.cpp
├── test/                             # 24 个 doctest 测试套件
├── examples/                         # hello, demo, tableview, dialogs, ...
└── app/                              # main
```

## 构建、安装、打包与 SDK

### 静态库（默认）

`python ltgui.py build` 产出静态库 `build/lib/ltgui.lib`(Windows)或
`build/lib/libltgui.a`(Linux/macOS),连同 app 与全部示例一起构建。
**消费者必须定义 `LTGUI_STATIC`**,否则 Windows 上公共头会假定
`__declspec(dllimport)`:

```bash
clang++ -std=c++20 -DLTGUI_STATIC -I <prefix>/include/ltgui my_app.cpp \
    <prefix>/lib/ltgui.lib -lgdi32 -lgdiplus -luser32 -lcomctl32 -lole32 \
    -limm32 -ld3d11 -ldxgi -ld3dcompiler
```

### 共享库 SDK(--dll)

```bash
python ltgui.py build --dll ./sdk
```

`sdk/` 目录内容:

| 文件 | 用途 |
|------|------|
| `ltgui.h` | 融合单头 SDK(生成后经冒烟编译验证) |
| `ltgui.dll` | 共享库(Windows) |
| `libltgui.dll.a` | 链接 DLL 用的导入库 |
| `stb_truetype.h` | GPU 字体图集依赖(随 SDK 分发) |

消费者 include `sdk/ltgui.h` **不要**定义 `LTGUI_STATIC`,链接导入库即可;
Windows 上 SDK 头自动带上 `__declspec(dllimport)` 修饰。Linux 下头文件与
链接方式无关(-lltgui 链 `libltgui.so`);macOS 内部走 `-dynamiclib`。

> MSVC(`cl`)不支持 `--dll`(共享构建需要逐符号导出标记或 .def);共享构建
> 请用 `--compiler clang`,或直接构建静态库。

### 安装与打包

```bash
python ltgui.py install --prefix /path           # 复制库 + 全部头文件
python ltgui.py package                          # build/ltgui-1.0.0-sdk.zip
```

`package` 打包静态库、融合头 `ltgui.h` 与 `stb_truetype.h`,版本名取自
`include/ltgui/version.h`(唯一事实源;git tag 不一致仅告警)。

CMake 消费方在 `cmake --install` 后可以直接 `find_package`:

```bash
cmake --install build-cmake --prefix /path/prefix
# 消费方 CMakeLists.txt:
#   find_package(ltgui REQUIRED)
#   target_link_libraries(app PRIVATE ltgui::ltgui)   # 自动带上 LTGUI_STATIC
```

### C SDK(C23 绑定)

纯 C 绑定覆盖整个框架:不透明句柄(`ltgui_widget*`、`ltgui_window*`)、
`xxx_create()/xxx_destroy()` 生命周期、int 错误码(`ltgui_last_error()` 读取
消息)、带 userdata 的 C 回调(`ltgui_signal_connect`)。C 代码按 **C23**
编译,并用 C++ 驱动链接(库本体是 C++):

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
# Windows / Linux / macOS(使用 clang):
clang -std=c23 -I sdk -c my_app.c
clang++ my_app.o -lltgui       # 用 C++ 驱动链接
```

注意:MSVC 的 C 编译器只实现 C11,不支 C23——C 绑定请用 clang/gcc
(MSVC 仍可在 C++ 翻译单元中消费该头)。`examples/c_hello.c` 与
`test/c_api_test.c` 是可运行的示例;SDK 包内 C 头名为 `ltgui_c.h`。

## 许可证

MIT
