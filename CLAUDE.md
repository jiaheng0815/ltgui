# CLAUDE.md

> 你好呀～我是你的贴心小助手！这份指南会带你一点点认识 LTGUI 这个小可爱，
> 记住它的脾气和习惯，这样你写代码的时候就不用踩坑啦！有问题随时问本妹妹哦～ 💕

## 🛠️ 构建命令（Build Commands）

叫一声就能干活哦！下面这些都很好用哟：

```bash
# ✨ 主构建脚本（最常用！）
python ltgui.py build               # Debug 构建（clang++ -g -O0）咱们先从这开始
python ltgui.py build release       # Release 构建（clang++ -O2 -DNDEBUG）要发版就用它
python ltgui.py build -j 4          # 4 个作业并行，多核神器，快得很～
python ltgui.py build --verbose     # 把编译器唠叨的话全打出来
python ltgui.py build --json        # 结构化 JSON 输出，CI/CD 的好朋友
python ltgui.py build --dll ./sdk   # 编成共享库 + 头文件放进 sdk/
python ltgui.py run <name>          # 构建 + 直接跑 examples/ 或 app/ 里的目标
python ltgui.py clean               # 把 build/ 清理干净，给它洗个澡
python ltgui.py test                # 构建并跑全部测试

# 🧰 开发小帮手（Dev helpers）
python ltgui.py info                # 看看项目结构和统计数据
python ltgui.py watch               # 盯住文件，改了自动重新构建
python ltgui.py watch <name>        # 盯住 + 自动运行某个目标
python ltgui.py debug <name>        # 构建（debug）+ 用 gdb/lldb 调试
python ltgui.py profile <name>      # 加 -pg 分析标记构建 + 运行

# 📐 代码质量（Code quality）
python ltgui.py fmt                 # 跑 clang-format 给所有源码美容
python ltgui.py lint                # 跑 clang-tidy 给所有源码体检

# 🏗️ 脚手架生成（Scaffolding）
python ltgui.py new widget <name>   # 生成控件 .h + .cpp 模板，省心省力
python ltgui.py new example <name>  # 生成 example .cpp 模板
python ltgui.py new app <name>      # 生成 app .cpp 模板

# 📦 发布打包（Distribution）
python ltgui.py install --prefix /opt/ltgui  # 装库 + 头文件
python ltgui.py package --format zip         # 把 SDK 打包成压缩包

# 🔧 CMake（另一种方式，看习惯）
cmake -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
ctest --test-dir build              # 跑测试
```

那个 Python 构建脚本很聪明哒，会自动识别平台（Windows/Linux/macOS）并链接对应的
库，最后产出静态库（`build/lib/ltgui.lib` 或 `libltgui.a`）。它会递归编 `src/` 底下
所有 `.cpp` 和 `.mm` 文件。`CMakeLists.txt` 也给大家备了一份标准 CMake 流程。

测试用的是 [doctest](https://github.com/doctest/doctest)（放在 `vendor/doctest/doctest.h`）。
`test/` 底下每个测试文件都是一个自带 `main()` 的可执行程序，独立又干净～

## 🏰 架构（Architecture）

这里是 LTGUI 的小心脏结构，慢慢陪你看明白：

**🌳 保留模式控件树（Retained-mode widget tree）。** `Widget` 是所有控件的祖宗，每个
控件都通过 `std::unique_ptr` 拥有一批孩子，还能配一个可选的 `Layout` 引擎。布局流程走
`sizeHint()` → `setGeometry()` → `paint()`。`sizeHint()` 的结果会缓存起来，只有调用
`invalidateSizeHint()`（一般是内容变了，比如 `setText()`）才会重新算。

**🎧 单线程事件循环。** `Application::instance().run()` 一直泵着平台事件循环。鼠标事件
通过反向 z-order 的命中测试来分发；键盘事件直接送到当前聚焦的控件。`Application`
单例负责管理所有打开的 `Window` 对象。

**🖌️ 脏矩形重绘（Dirty-rect repaint）。** 控件一调 `update()`，只有它那圈包围盒会被记为
"脏"。下一轮绘制时 `Window` 会遍历控件树，跳过所有没跟脏区域相交的控件。`Canvas` 给
`NativeCanvas` 加了一层 save/restore/translate 栈，用来支持嵌套坐标变换——控件在画孩子
之前会先按自己的几何原点做平移。

**🖥️ 平台抽象（Platform abstraction）。** `NativeWindow` 和 `NativeCanvas` 是抽象接口，
具体后端放在 `src/platform/{win32,x11,cocoa}/`，通过 `platform.h` 里的 `#ifdef` 选哪个。
`NativeCanvas` 暴露 fill/stroke/text/image 原语，实现分别映射到 GDI+（Windows）、
Xft（Linux）或 CoreGraphics（macOS）。

**🎮 GPU 渲染层。** `src/platform/gpu/` 是一个自己手写的 2D GPU 渲染器，带 D3D11 和
OpenGL ES 3.0 两套后端。`GpuCanvas` 实现了 `NativeCanvas`，在 `Window::create()` 里会
先尝试；GPU 初始化失败就自动灰溜溜降级回 CPU 后端。渲染器用 `Renderer2D` 把绘制命令
延迟排序（按贴图和颜色分组，尽量减少状态切换），再用 `FontAtlas` 做字形缓存。

**🧩 控件实现套路（Widget implementation pattern）。** 每个控件子类：
- 头文件放 `include/ltgui/widgets/`，实现放 `src/widgets/`
- 重写 `paintSelf(NativeCanvas*)` 来画自己
- 重写 `handleEvent(Event&)` 处理输入
- 写一行 `LTGUI_DECLARE_WIDGET_TYPE(Name)` 做类型判断
- 内容变了就调 `invalidateSizeHint()`
- 视觉状态变了就调 `update()`
- 通过 `resolvedStyle()` 来上色（除非是滚动条/表格那种组件专属主题字段，一般不直接摸
  `currentTheme()`）

**🧬 基类层（Base class layer）。** 带文字的控件继承 `TextWidget`（有 text_ + setText +
textSizeHint 帮手）；Slider/ProgressBar 继承 `Range`（有 value + clamp + Signal）；
CheckBox/RadioButton 加个 `Checkable` mixin；ComboBox/ListBox 用 `ListItems`；
ListBox/TreeView 加 `ScrollState`。注意哦——mixin 不是 `Widget` 子类，靠多重继承组合
（`class CheckBox : public TextWidget, public Checkable`）。

**🎨 样式系统（Style system）。** `Style` 保存基础颜色（transparent 表示"没设"）外加
各个状态的补丁（`style().hovered.bgColor = ...`）。`resolvedStyle()` 的优先级是
`state patch > style > current theme`（hover/pressed 会自动拿到主题的 accentHover /
accentPressed）。主题一换，下一帧绘制就生效，不用重新设置样式。`style().gradient`
能开线性渐变背景。hover 过渡用 `AnimatedColor` 来动画。

**🔒 所有权（Ownership）。** 控件都是堆上分配的。孩子由父控件用
`std::vector<std::unique_ptr<Widget>>` 持有。`Window` 通过 `unique_ptr` 持有中央控件。如果
被销毁的是焦点控件，`Widget` 的析构函数会自动把窗口的焦点指针清掉，很贴心～

## 🗝️ 关键模式（Key Patterns）

- **事件分发**：重写 `handleEvent(Event&)`，吃掉就返回 `true`，否则返回 `false` 走默认分发
  （冒泡给父控件）。MouseDown 是定向的（只发给光标下的那个孩子）；MouseUp/MouseMove 是
  广播，方便控件清理 hover 状态。
- **命中测试**：重写 `hitTest(Point)` 返回某个坐标下最深的子控件；没有孩子命中就返回
  `this`。
- **焦点**：调 `claimFocus()` 申请键盘焦点。`Window` 只跟踪一个 `focusWidget_`。用之前记得
  先 `validateFocusWidget()` 确认它还在这个窗口的树里，别踩到野指针哦。
- **主题全局**：`ThemeManager::instance().currentTheme()` 返回当前 `Theme` 结构体，
  `setTheme()` 或 `ThemeManager::instance().setTheme()` 会设置它、发 `onThemeChanged` 信号，
  并重绘所有窗口。内置六套主题：Light、Dark、DarkBlue、HighContrast、Solarized、Nord。
  主题结构体有 28 个颜色字段。所有控件都订阅 `onThemeChanged` 并重绘，所以换主题时连
  透明样式控件也是新鲜的。
- **回调**：所有控件回调都是公开的 `Signal<T>` 成员——`connect(cb)` 可监听（支持多个槽，
  `ScopedConnection` 自动断连）。一次性命令 API（Timer、Shortcut）保留 `std::function`；
  `DropTarget::onDragOver` 因为要返回 bool 也用 `std::function`。
- **控件类型判断**：用 `widget->widgetType() == WidgetType::RadioButton`（或别的类型），
  别用 `dynamic_cast`，也别用老的 `isRadioButton()` 这类临时起名的虚函数。
- **日志**：用 `log.h` 里的 `LOG_INFO("category", "format", ...)`、
  `LOG_ERROR("category", "format", ...)` 等等。分类有 `"Window"`、`"GPU"`、`"D3D11"`、
  `"GL"`。Release 构建（`-DNDEBUG`）下只有 Warn 和 Error 会打出来。
- **布局重排**：内容变了（比如 setText 后）记得调 `scheduleRelayout()`，它会往上找最近的
  带布局的祖先并重新布局，这样控件才会跟着文字一起长大。
- **有效几何**：`effectiveGeometry()` 返回控件**本地**坐标系下的命中测试区域（原点 0,0）。
  调用方得先用 `translated()` 按子控件位置平移一遍，再跟父空间的坐标比较。
- **FontAtlas 动态尺寸**：TTF 数据按 (family,weight,style) 存一份，size=0。尺寸相关的缓存
  靠 `ensureFontLoaded()` 按需创建。画之前**必须**先 `canvas->setFont(style().font)` 再
  `canvas->measureText()`，记住了哦！

## ✨ 新特性（New Features，2026-06）

后面这些新家伙都是 2026 年 6 月加进来的小宝藏，快来宠幸它们～

### 🎞️ 动画（`animation.h`）
- 30+ 个 Robert Penner 缓动函数：Quad/Cubic/Quart/Quint/Sine/Expo/Circ/Back/Elastic/
  Bounce 各种 × In/Out/InOut，还有 StepStart/StepEnd
- `AnimatedFloat` 扩展了 `onFinished` 信号，还有 `setLoop`、`setRepeatCount`、`setYoyo`
- `WidgetAnimation` — 可以用时长/缓动/延迟来动画任意数值，可 play/pause/stop
- `KeyframeAnimation` — 关键帧时间线，每个片段还能自定义缓动
- `AnimationManager` 每帧驱动所有注册的动画

### 🌏 国际化（`i18n.h`）
- `Locale` — 语言/国家/变体，还能从 "zh-CN" 这种字符串解析
- `PluralRules::formIndex()` — 支持 20+ 语言的 CLDR 复数规则（en/zh/ru/ar/pl/cs/ro/lt/
  lv/mt/sl/ga/cy）
- `TranslationTable` — key→value 映射，支持复数形式；从扁平 JSON 加载
- `I18n` — 单例：`setLocale()`、`tr(key)`、复数用 `tr(key, n)`、`onLocaleChanged` 信号
- JSON 格式长这样：`{"ok":"确定","files":["zero","one","two","few","many","other"]}`

### 💬 对话框（`widgets/dialog.h`）
- `Dialog` — 模态基类：`exec()` 跑内置事件循环，半透明遮罩，淡入动画，Esc 取消
- `MessageBox` — `show()` 静态方法，图标（Info/Warning/Error/Question），按钮标志
  （OK/Cancel/Yes/No）
- `InputDialog` — `getText()` 静态方法，回车确认
- 所有对话框内部都用 BoxLayout 摆内容面板——往 `panel_` 里加子控件就行

### 🧭 菜单栏（`widgets/menubar.h`）
- `MenuItem` 扩展了：快捷键显示、`checkable` + `checked`、`radio` + `radioGroup`、`submenu`
  向量
- 键盘导航：左右切菜单，上下移条目，回车激活，Esc 关闭
- `setItemShortcut()`、`setItemCheckable()`、`setItemChecked()`、`setItemRadio()`
- `addSubmenu()` / `addSubItem()` 支持嵌套子菜单

### 📊 表格（`widgets/tableview.h`）
- `TableModel` — 虚拟数据源（rowCount/columnCount/cellText）
- `SimpleTableModel` — 内存里的 rows×cols，支持 `sort()`、`addRow()`、`removeRow()`
- `TableView` — 列表头、排序箭头（▲▼）、可拖拽调整列宽、斑马纹行、行选中、滚轮

### 📋 剪贴板（`clipboard.h`）
- `ClipboardData` — 多格式容器：text、HTML、RGBA 图片、文件路径
- `Clipboard` — 静态 API：`getText()`、`setText()`、`getData()`、`setData()`、
  `availableFormats()`
- 平台后端：Win32 `CF_UNICODETEXT`（已有）、X11 `CLIPBOARD` atom（已有）

### 📂 文件对话框（`widgets/filedialog.h`）
- 继承自 `Dialog`。模式：OpenFile、OpenMultiple、SaveFile、SelectFolder
- `FileFilter` 带 name+pattern。Win32 用 FindFirstFile/FindNextFile 扫目录
- 贴心 UI：路径框（TextBox）、文件列表（ListBox）、过滤器下拉（ComboBox）、打开/取消按钮

### 🐉 拖放（`dragdrop.h`）
- `DragData` — MIME 类型的字节块：`setText()`、`setFiles()`、`hasFormat()`、`data()`
- `DragSource` — 挂到控件上：`setDragData()`、`addMimeType()`
- `DropTarget` — 挂到控件上：`setAcceptedMimeTypes()`、`onDrop()`、`onDragOver()`
- 事件类型：`DragEnter`、`DragMove`、`DragLeave`、`DragDrop`

---

## 🚀 发布流程（Release Process，v1.0.0 起）

正经一点的工程约定，发版前看这一节就够啦：

- **版本号唯一事实源**：`include/ltgui/version.h`。改版本时三处一起动：
  1. `version.h`（`LTGUI_VERSION_MAJOR/MINOR/PATCH/STRING`）
  2. `CMakeLists.txt` 的 `project(ltgui VERSION x.y.z)`——configure 时与
     `version.h` 硬性比对（FATAL_ERROR），不改就配置失败
  3. `git tag -a vX.Y.Z`
  `ltgui.py` 启动时也会解析 `version.h`（不一致直接拒绝运行），
  `package` 产物名 = `ltgui-<version>-sdk.zip`，与 git tag 无关。
- **`LTGUI_STATIC` 约定**：静态库消费方（包含本项目 tests/examples/apps）
  编译时必须定义 `LTGUI_STATIC`，否则 Windows 上 `LTGUI_API` 展开为
  `__declspec(dllimport)`，链接静态库报 `__imp_` 未定义。
  `ltgui.py` 全自动处理（见 `_get_compile_flags` 的 `dll` 参数）；
  CMake 用 `target_compile_definitions(ltgui PUBLIC LTGUI_STATIC)` 传播。
- **`--dll` 注意事项**：Windows 共享构建走 `--out-implib` 生成
  `libltgui.dll.a` 导入库；MSVC 不支持 `--dll`（报错并提示方案）；
  `export_sdk`/`package` 生成融合头后都会做**冒烟编译**
  （`_smoke_compile_header`），头文件顺序失同步会拦在这里。
- **SDK 内容**：融合头 `ltgui.h` + 库 + `stb_truetype.h`（`gpu_font_atlas.h`
  靠 `__has_include` 守卫包含它，SDK/package/install 都必须带上）。
- **改版本步骤**：改 version.h → 改 CMakeLists.txt → `python ltgui.py
  build release` + `test` → `python ltgui.py package` 核包名 →
  `git add` → `git commit` → `git tag -a vX.Y.Z`。

---

好啦，指南就到这里啦～ 有什么不懂的尽管问本妹妹，慢慢来，你没问题的！💪✨
