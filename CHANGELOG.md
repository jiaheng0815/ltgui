# Changelog

本项目遵循 [Keep a Changelog](https://keepachangelog.com/zh-CN/1.1.0/) 与
[semver](https://semver.org/lang/zh-CN/)。

## [Unreleased]

### Removed
- 公共 API 冗余清理(v1.0.1):`Window::getSize()`、`ListItems::selectedIndex()/setSelected()`、
  `Image::setFitMode(char)`、`TableView::selectedRow()/selectRow()` 别名删除——一律使用
  `currentIndex()/setCurrentIndex()`、`size()`、`setFitMode(FitMode)`;
  死类 `Canvas`(canvas.h)与 `ListBox::currentScrollOffset()` 私有转发移除。
- 示例样板行清理(setRange(0,100) 等重复默认值、no-op addStretch(0)),行为不变。

## [1.0.0] - 2026-08-23

### Added
- `--dll` 共享库 SDK 完整可用:Windows 生成导入库 `libltgui.dll.a`,
  SDK 含 `ltgui.dll` + 融合头 + `stb_truetype.h`。
- `include/ltgui/version.h` 版本号单一事实源;`package` 产物名
  `ltgui-<version>-sdk.zip`;CMake 与 ltgui.py 均校验版本一致性。
- 融合头生成后冒烟编译验证(`_smoke_compile_header`),防头文件顺序失同步。
- `LTGUI_API` 导出装饰(`include/ltgui/api.h`),核心类
  (Window/Application/Widget/ThemeManager)支持 DLL 导出/导入。

### Changed
- 文档全面校正:README/BUILD_GUIDE 的 C++17→C++20、测试套件 17→24、
  "最小示例"重写为现行 API、控件表/WidgetType 枚举同步、中文版补齐
  状态样式/渐变/迁移三小节;新增"构建、安装、打包与 SDK"使用文档。
- `generate_amalgamated_header` 修复:不再剥离
  `#include "stb_truetype.h"`(此前 SDK 融合头缺少字体库,GPU 文字静默降级)。
- `export_sdk` 不再猜测导入库路径(此前会把 27MB 静态库当导入库拷入 SDK)。

### Fixed
- 一次 114 条审计波次(112 修复 + 1 误报),详见
  `docs/bug-audit-2026-08-23.md`。覆盖:Signal 重入安全、动画计数泄漏、
  UTF-8 光标/越界、Win32 关闭/DPI/光标/剪贴板、GPU 渲染与泄漏、
  Dialog 模态体系、MenuBar/ContextMenu 弹层、i18n 复数规则等。
- CMake 收尾:`/utf-8` 提到项目级、GLOB 加 `CONFIGURE_DEPENDS`、
  平台宏 PUBLIC 传播(见 bug 109/110)。

### Removed
- 过期的 `test_sdk/` 本地产物;`_strip_function_bodies` 死代码;
  README 中已删除的 `Style::setMargin` 文档残留。
