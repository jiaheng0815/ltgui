# Contributing to LTGUI

感谢你愿意为 LTGUI 添砖加瓦!这里是与维护者配合的约定。

## 开发环境

```bash
python ltgui.py build          # clang++ Debug(自动检测平台)
python ltgui.py test           # 全部单元测试(含 C23 SDK 测试)
python ltgui.py build --compiler msvc   # MSVC 通道(需 VS 开发者环境)
python ltgui.py bench          # 性能基准(release)
python ltgui.py fmt            # clang-format
```

CI 门禁(推送后自动):Linux(clang + **ASan/UBSan** + C23)、macOS(Cocoa
编译)、Windows(clang + GUI 冒烟 + `--dll`)、Windows(MSVC)。
**提交前本地至少跑通 `python ltgui.py test`。**

## 规范

- **C++23**:语言标准是 C++23,新代码直接用 C++23 特性
  (`std::to_underlying`、deduced-this 等);避免旧式写法
- **命名**:类型 PascalCase、方法 camelCase、成员尾下划线(`value_`);
  控件类用 `widgets/*.h + src/widgets/*.cpp` + `LTGUI_DECLARE_WIDGET_TYPE`
- **样式**:与周边代码一致,新文件过一遍 `python ltgui.py fmt`
- **事件**:直接消费则 `event.accepted = true;` 并返回;控件内部状态
  用 `lastClick`/`lastKey` 而非引入新事件字段(公共 Event 变更需审查)
- **回调**:一律 `Signal<T>`(公开成员),`connect()` 订阅;一次性 API
  才用 `std::function`;C API 的 C 回调 + userdata 走 `ltgui_signal_connect`
- **类型判断**:`widget->widgetType() == WidgetType::X`;不用 `dynamic_cast`
  判断控件类型(mixin 基类转换除外,如 `ListItems`/`Checkable` 必须
  dynamic_cast)
- **错误**:C API 边界统一 `LTGUI_ERR_*` + `ltgui_last_error()`;库内异常
  不得穿透 C 边界

## 测试

- 逻辑测试:`test/test_*.cpp`(doctest,自带 main)
- **C API 单测**:`test/test_c_api.cpp`;纯 C23 验证在 `test/c_api_test.c`
- 控件补测惯例:构造 + 事件注入用
  `static_cast<Widget&>(w).handleEvent(...)`(protected 虚分发)
- 大改动请附基准数据:`python ltgui.py bench`

## 提交信息

推荐 Conventional Commits + 项目主题头(少用多空行?保持一行摘要+正文):

```
@ feat(tableview): in-place cell editing

- 要点一…
- 要点二…
```

前缀:`feat/fix/refactor/style/docs/chore/build/perf/test`。

## 评审关注点

1. 是否有回归风险(测试覆盖不足)?
2. 是否与现有约定冲突(命名/样式/信号)?
3. 平台差异(Windows/Linux/macOS)是否考虑?
4. C API 是否需要同步暴露?

有问题先开 issue 讨论,再动手。
