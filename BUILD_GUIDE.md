# ltgui 构建与调试完全指南

从零开始——编译器安装、源码下载、编译、VSCode/VS 调试、常见问题排查。

---

## 目录

- [前置知识](#前置知识)
- [方案 A：clang++（推荐，零配置）](#方案-aclang推荐零配置)
  - [1. 下载安装 LLVM](#1-下载安装-llvm)
  - [2. 验证安装](#2-验证安装)
  - [3. 下载 ltgui 源码](#3-下载-ltgui-源码)
  - [4. 编译](#4-编译)
  - [5. VSCode 调试配置](#5-vscode-调试配置)
- [方案 B：MSVC](#方案-bmsvc)
  - [1. 安装 Visual Studio](#1-安装-visual-studio)
  - [2. 打开开发者命令行](#2-打开开发者命令行)
  - [3. 编译](#3-编译)
  - [4. Visual Studio 调试配置](#4-visual-studio-调试配置)
- [CMake 构建（替代方案）](#cmake-构建替代方案)
- [编译产物说明](#编译产物说明)
- [测试](#测试)
- [运行示例](#运行示例)
- [常见问题排查](#常见问题排查)
- [项目扩展指南](#项目扩展指南)

---

## 前置知识

ltgui 支持三种编译器：

| 编译器 | Windows | Linux | macOS |
|--------|---------|-------|-------|
| **clang++** | ✅ 推荐 | ✅ 推荐 | ✅ 推荐 |
| **MSVC (cl.exe)** | ✅ | — | — |
| **g++** | ✅ | ✅ | ✅ |

Python 构建脚本 `ltgui.py` 自动检测平台和编译器。你只需要 Python 3 和一个 C++17 编译器。

---

## 方案 A：clang++（推荐，零配置）

clang++ 是 LLVM 项目的 C++ 编译器，在 Windows 上无需 Visual Studio 即可独立使用（通过 MinGW/GNU 工具链或 MSVC 工具链）。

### 1. 下载安装 LLVM

**方式一：官方 GitHub Release（最快）**

打开浏览器，访问：
```
https://github.com/llvm/llvm-project/releases
```

找到最新的稳定版本（例如 LLVM 19.1.x），下载 Windows 安装器：

| 版本 | 文件名 |
|------|--------|
| 64 位 | `LLVM-19.1.x-win64.exe` |
| 32 位 | `LLVM-19.1.x-win32.exe` |

**运行安装器，关键步骤：**
1. 安装路径用默认 `C:\Program Files\LLVM`
2. **勾选** "Add LLVM to the system PATH for all users"
   - 这一步至关重要！不勾选的话 PowerShell 里找不到 `clang++`
3. 其余选项保持默认，一路 Next

**方式二：winget（命令行）**

在 PowerShell 中运行：
```powershell
winget install LLVM
```

winget 自动完成下载、安装、PATH 配置。安装完成后**重新打开 PowerShell**。

**方式三：scoop（开发者推荐）**

```powershell
# 先安装 scoop（如果还没有）
Set-ExecutionPolicy RemoteSigned -Scope CurrentUser
irm get.scoop.sh | iex

# 安装 LLVM
scoop install llvm
```

### 2. 验证安装

**重新打开一个新的 PowerShell 窗口**，运行：

```powershell
clang++ --version
```

预期输出类似：
```
clang version 19.1.0
Target: x86_64-pc-windows-msvc
Thread model: posix
```

再确认 Python：
```powershell
python --version
```
预期输出 `Python 3.x.x`。如果没有 Python，从 https://www.python.org/downloads/ 下载安装，勾选 "Add Python to PATH"。

### 3. 下载 ltgui 源码

```powershell
# 克隆仓库
git clone https://github.com/jiaheng0815/ltgui.git
cd ltgui
```

如果还没有 git：
```powershell
winget install Git.Git
```

### 4. 编译

```powershell
# 进入项目目录
cd D:\code\ltgui

# Debug 构建（默认，含调试符号 -g -O0）
python ltgui.py build

# Release 构建（优化 -O2 -DNDEBUG）
python ltgui.py build release

# 指定 LLVM 路径（如果 clang++ 不在 PATH 里）
python ltgui.py build --compiler "C:\Program Files\LLVM\bin\clang++.exe"

# 只编译某个示例
python ltgui.py build --example hello

# 构建 DLL + SDK header
python ltgui.py build --dll ./sdk
```

**编译过程说明：**

```
Building ltgui (debug) for windows with clang++...
Found 37 source files.
  Compiling animation.cpp...
  Compiling app.cpp...
  ...
  Compiling window.cpp...
  Creating ltgui.lib...         ← 静态库
  Created D:\code\ltgui\build\lib\ltgui.lib
Building apps...
  Building app: main...
  Created D:\code\ltgui\build\main.exe
Building examples...
  Building example: demo...
  Building example: hello...

Build complete.
```

增量编译：`ltgui.py` 自动追踪 `.cpp` 和所有 `#include` 头文件的修改时间。只改了一个文件就只重编译一个文件。

### 5. VSCode 调试配置

#### 5.1 安装 VSCode 扩展

打开 VSCode，按 `Ctrl+Shift+X`，搜索安装：

| 扩展 | 用途 |
|------|------|
| **C/C++** (微软) | IntelliSense、调试器支持 |
| **clangd** (LLVM) | 更好的代码补全和诊断（可选） |

#### 5.2 创建调试配置

在项目根目录创建 `.vscode/launch.json`：

```json
{
    "version": "0.2.0",
    "configurations": [
        {
            "name": "ltgui: Debug demo.exe",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/demo.exe",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "miDebuggerPath": "C:\\Program Files\\LLVM\\bin\\lldb.exe",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for lldb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                }
            ],
            "preLaunchTask": "ltgui: build debug"
        },
        {
            "name": "ltgui: Debug hello.exe",
            "type": "cppdbg",
            "request": "launch",
            "program": "${workspaceFolder}/build/hello.exe",
            "args": [],
            "stopAtEntry": false,
            "cwd": "${workspaceFolder}",
            "environment": [],
            "externalConsole": false,
            "MIMode": "gdb",
            "miDebuggerPath": "C:\\Program Files\\LLVM\\bin\\lldb.exe",
            "setupCommands": [
                {
                    "description": "Enable pretty-printing for lldb",
                    "text": "-enable-pretty-printing",
                    "ignoreFailures": true
                }
            ],
            "preLaunchTask": "ltgui: build debug"
        }
    ]
}
```

#### 5.3 创建编译任务

创建 `.vscode/tasks.json`：

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "ltgui: build debug",
            "type": "shell",
            "command": "python",
            "args": [
                "ltgui.py",
                "build"
            ],
            "group": {
                "kind": "build",
                "isDefault": true
            },
            "presentation": {
                "reveal": "always",
                "panel": "shared"
            },
            "problemMatcher": {
                "owner": "cpp",
                "fileLocation": ["absolute"],
                "pattern": {
                    "regexp": "^(.+\\.(?:cpp|h|mm)):(\\d+):(\\d+):\\s+(error|warning|note):\\s+(.*)$",
                    "file": 1,
                    "line": 2,
                    "column": 3,
                    "severity": 4,
                    "message": 5
                }
            }
        },
        {
            "label": "ltgui: test",
            "type": "shell",
            "command": "python",
            "args": [
                "ltgui.py",
                "test"
            ],
            "group": {
                "kind": "test",
                "isDefault": true
            },
            "presentation": {
                "reveal": "always",
                "panel": "shared"
            }
        }
    ]
}
```

#### 5.4 调试流程

1. 在 `demo.cpp` 或任何 `.cpp` 文件中设断点（按 `F9`）
2. 按 `F5` 启动调试
3. VSCode 自动执行 preLaunchTask 编译 → 启动 lldb → 附加到进程
4. 断点命中时检查变量、调用栈、表达式

**调试器选择：**

| 调试器 | 安装方式 | 优点 |
|--------|---------|------|
| **lldb** | LLVM 自带 | 与 clang 生成的 DWARF 调试信息天然兼容 |
| **gdb** | `winget install gdb` 或 MinGW | 传统选择，但有时与 clang 的调试信息不完全兼容 |

如果 `lldb` 路径不是 `C:\Program Files\LLVM\bin\lldb.exe`，用以下命令查找：
```powershell
where lldb
```

#### 5.5 clangd 配置（可选，更好的 IntelliSense）

如果用 clangd 替代微软 C++ 扩展，创建 `.vscode/settings.json`：

```json
{
    "clangd.path": "C:\\Program Files\\LLVM\\bin\\clangd.exe",
    "clangd.arguments": [
        "--compile-commands-dir=${workspaceFolder}/build",
        "--header-insertion=never",
        "--background-index"
    ],
    "C_Cpp.intelliSenseEngine": "disabled"
}
```

---

## 方案 B：MSVC

MSVC 是微软的 C++ 编译器（`cl.exe`）。它和 Visual Studio 绑定，但也提供免费的 Build Tools 版本。

### 1. 安装 Visual Studio

**方式一：Visual Studio 2022 Community（免费，推荐个人开发）**

1. 打开 https://visualstudio.microsoft.com/zh-hans/downloads/
2. 下载 **Visual Studio 2022 Community** 安装器
3. 运行安装器，在"工作负荷"标签页勾选：

   | 工作负荷 | 说明 |
   |---------|------|
   | **使用 C++ 的桌面开发** | 包含 MSVC 编译器、Windows SDK、调试器 |

4. 在右侧"安装详细信息"面板确认以下组件已勾选：
   - MSVC v143 - VS 2022 C++ x64/x86 生成工具
   - Windows 11 SDK (10.0.22621.0 或更新)
   - 适用于 Windows 的 C++ CMake 工具（可选）
   - C++ AddressSanitizer（可选，用于检测内存错误）

5. 点击"安装"，等待完成（约 6-10 GB）。

**方式二：仅 Build Tools（无 IDE，仅命令行）**

如果不需要 Visual Studio IDE，只装编译工具链：

1. 打开 https://visualstudio.microsoft.com/zh-hans/downloads/#build-tools-for-visual-studio-2022
2. 下载 **Build Tools for Visual Studio 2022**
3. 运行安装器，勾选"使用 C++ 的桌面开发"，安装

### 2. 打开开发者命令行

**MSVC 的 `cl.exe` 不在系统 PATH 里！** 必须从专门的命令行启动。

**方法一：开始菜单**

按 `Win` 键，搜索以下任一：

| 名称 | 用途 |
|------|------|
| **Developer Command Prompt for VS 2022** | 经典 cmd 窗口 |
| **Developer PowerShell for VS 2022** | PowerShell 窗口（推荐） |
| **x64 Native Tools Command Prompt for VS 2022** | 纯编译环境，无多余 PATH |

点击打开，你会看到开头打印：
```
**********************************************************************
** Visual Studio 2022 Developer Command Prompt v17.x
** Copyright (c) 2022 Microsoft Corporation
**********************************************************************
```

此时 `cl` 已在 PATH 里。验证：
```cmd
cl
```
应输出 Microsoft C/C++ 编译器版本信息。

**方法二：在任何 PowerShell 中导入环境**

```powershell
# 找到 VS 安装路径下的 vcvarsall.bat 并执行
& "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
```

这个命令把 MSVC 工具链临时注入当前 PowerShell 会话的 PATH。

**方法三：Windows Terminal 配置（一劳永逸）**

在 Windows Terminal 设置中添加一个新配置文件：

```json
{
    "name": "VS 2022 Dev PS",
    "commandline": "powershell.exe -NoExit -Command \"& 'C:\\Program Files\\Microsoft Visual Studio\\2022\\Community\\Common7\\Tools\\Launch-VsDevShell.ps1'\"",
    "startingDirectory": "D:\\code\\ltgui",
    "icon": "ms-appx:///ProfileIcons/{GUID}.png"
}
```

### 3. 编译

在 Developer PowerShell 中：

```powershell
cd D:\code\ltgui

# Debug 构建
python ltgui.py build --compiler msvc

# Release 构建
python ltgui.py build release --compiler msvc

# 运行测试
python ltgui.py test --compiler msvc

# 运行示例
python ltgui.py run demo --compiler msvc
```

如果提示 `Error: MSVC compiler 'cl' not found in PATH`，说明**你不是在 Developer PowerShell 里**。关掉当前窗口，从开始菜单重新打开。

### 4. Visual Studio 调试配置

#### 4.1 从 VS 打开项目

ltgui 默认没有 `.sln` 文件，但可以用 CMake 生成（见下文）或直接用 VS 的"打开文件夹"功能。

**创建 CMake 配置后生成 VS 工程：**
```powershell
cmake -B build_vs -G "Visual Studio 17 2022" -A x64
```

然后在 Visual Studio 中打开生成的 `.sln` 文件。

#### 4.2 使用 VS 的 CMake 集成

Visual Studio 2022 原生支持 CMake。直接从 VS 菜单：**文件 → 打开 → CMake**，选择项目根目录的 `CMakeLists.txt`。

#### 4.3 附加到已有进程调试

如果程序是通过 `python ltgui.py run demo` 启动的：

1. 打开 Visual Studio
2. **调试 → 附加到进程** (`Ctrl+Alt+P`)
3. 找到 `demo.exe`，点击附加
4. 在源码中设断点，程序命中时 VS 会中断

#### 4.4 调试器要点

| MSVC 调试功能 | 快捷键 |
|-------------|--------|
| 设断点 | `F9` |
| 启动调试 | `F5` |
| 逐语句 | `F11` |
| 逐过程 | `F10` |
| 跳出 | `Shift+F11` |
| 查看变量 | 鼠标悬停或"局部变量"窗口 |
| 条件断点 | 右键断点 → 条件 |
| 内存窗口 | 调试 → 窗口 → 内存 |

---

## CMake 构建（替代方案）

Python 脚本是主力构建工具，但 CMake 也可用：

```powershell
# clang++ (Debug)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++
cmake --build build

# MSVC (Release)
cmake -B build_msvc -G "Visual Studio 17 2022" -A x64
cmake --build build_msvc --config Release

# 运行测试
ctest --test-dir build
ctest --test-dir build_msvc -C Debug
```

CMake 输出的可执行文件在 `build/` 下（clang）或 `build_msvc/Debug/` 下（MSVC）。

---

## 编译产物说明

```
build/
├── lib/
│   └── ltgui.lib          ← 静态库（所有编译器的输出）
├── obj/
│   ├── animation.cpp.o    ← 中间目标文件
│   ├── ...
│   └── window.cpp.o
├── demo.exe               ← 示例程序
├── hello.exe              ← 最小示例
├── main.exe               ← app/ 下的主程序
├── test_animation.exe     ← 测试可执行文件
├── ...
└── .platform              ← 平台/编译器指纹（切换编译器时自动 clean）
```

**重要文件：**
- `vendor/stb_truetype.h` — 字体光栅化库（GPU 文字渲染依赖）
- `font/Deng.ttf` — 默认中文字体（灯虹体系列）
- `ltgui.py` — Python 构建脚本（主入口）
- `CLAUDE.md` — AI 助手的项目上下文

---

## 测试

```powershell
# 构建并运行所有测试
python ltgui.py test

# 使用 MSVC
python ltgui.py test --compiler msvc

# 运行单个测试
python ltgui.py build              # 先构建
.\build\test_color.exe             # 直接运行测试可执行文件

# CMake 方式
ctest --test-dir build --output-on-failure
```

15 个测试套件覆盖：
- 几何运算（相交、合并、包含）
- 颜色编码（ARGB/ABGR 字节序）
- UTF-8 编解码
- 事件分发路由
- 布局系统
- Widget 树操作
- 样式边界情况
- 动画边界情况

---

## 运行示例

```powershell
# 一步构建+运行
python ltgui.py run hello    # 最小示例：两个按钮
python ltgui.py run demo     # 完整示例：14 种 widget 展示

# 或两步：先 build 再直接运行
python ltgui.py build
.\build\demo.exe

# 启用 GPU 加速（默认开启，GPU 不可用时自动回退到 GDI+）
# 查看控制台日志确认 GPU 状态：
# [Window][INFO ] GPU acceleration enabled: D3D11
# 或
# [Window][INFO ] Using software renderer (GDI+/X11)
```

---

## 常见问题排查

### Q1: `python ltgui.py build --compiler msvc` 报错找不到 cl

```
Error: MSVC compiler 'cl' not found in PATH.
```

**原因**：MSVC 编译器只能在 Visual Studio Developer Command Prompt 中使用。

**解决**：从开始菜单打开 "Developer PowerShell for VS 2022" 或 "Developer Command Prompt for VS 2022"，在那个窗口里运行命令。

### Q2: `python ltgui.py build` 报错找不到 clang++

```
FileNotFoundError: clang++
```

**原因**：安装 LLVM 时没有勾选 "Add to PATH"。

**解决**：
```powershell
# 方法1：重新运行 LLVM 安装器，勾选 "Add to PATH"

# 方法2：手动添加到当前会话
$env:Path += ";C:\Program Files\LLVM\bin"

# 方法3：指定完整路径
python ltgui.py build --compiler "C:\Program Files\LLVM\bin\clang++.exe"
```

### Q3: 编译成功但文字不显示

确认字体文件存在：
```powershell
ls D:\code\ltgui\font\Deng.ttf
```

控制台应输出：
```
[Window][INFO ] GPU acceleration enabled: D3D11
[GPU][INFO ] Loaded default font: D:/code/ltgui/font/Deng.ttf
```

如果看到 `No system font found`，检查 `font/Deng.ttf` 是否在仓库中。首次 clone 后该文件应该在。

### Q4: GPU 模式画面空白/全白

这是 D3D11 初始化失败回退的信号。检查控制台日志：
```
[GPU][INFO ] No GPU found, falling back to CPU rendering.
```
如果看到这条，说明：
- DirectX 11 运行时损坏 → 运行 `dxdiag` 检查
- 或者 GPU 驱动过期 → 更新显卡驱动

### Q5: 编译警告 "LF will be replaced by CRLF"

Git 在 Windows 上的默认行尾转换。不影响编译，可以忽略。如果想消除：
```powershell
git config core.autocrlf true
```

### Q6: Python 报错 `capture_output=True` 不存在

你的 Python 版本太旧。`capture_output` 参数需要 Python 3.7+。
```powershell
python --version  # 确认 ≥ 3.7
```

### Q7: fork 后字体文件过大（Deng.ttf 16MB）

这是正常的。字体内嵌了完整的中文、日文、韩文字形集。
Git LFS 可以在 `.gitattributes` 中配置：
```
*.ttf filter=lfs diff=lfs merge=lfs -text
```

### Q8: VSCode 调试时 lldb 报错 "Unable to find executable"

检查 `launch.json` 中的 `program` 路径是否正确：
```json
"program": "${workspaceFolder}/build/demo.exe"
```
确保先执行过 `python ltgui.py build` 生成可执行文件。

---

## 项目扩展指南

### 添加新的 Widget

**1. 创建头文件 `include/ltgui/widgets/newwidget.h`：**
```cpp
#pragma once
#include "widget.h"
#include <string>

namespace ltgui {

class NewWidget : public Widget {
public:
    explicit NewWidget(Widget* parent = nullptr);

    WidgetType widgetType() const override { return WidgetType::NewWidget; }
    Size sizeHint() const override;
    // canAcceptFocus() → override if this widget can't receive focus

protected:
    void paintSelf(NativeCanvas* canvas) override;
    bool handleEvent(Event& event) override;

private:
    std::string data_;
};

} // namespace ltgui
```

**2. 创建实现文件 `src/widgets/newwidget.cpp`：**
```cpp
#include "widgets/newwidget.h"
#include "theme.h"
#include "platform/native_canvas.h"

namespace ltgui {

NewWidget::NewWidget(Widget* parent) : Widget(parent) {
    style().bgColor = currentTheme().bgSecondary;
    // ...
}

Size NewWidget::sizeHint() const {
    if (!sizeHintDirty()) return cachedSizeHint();
    setCachedSizeHint({100, 24});
    return cachedSizeHint();
}

void NewWidget::paintSelf(NativeCanvas* canvas) {
    Rect r = absoluteRect();
    // ... 绘制代码 ...
}

bool NewWidget::handleEvent(Event& event) {
    // ... 事件处理 ...
    return false;
}

} // namespace ltgui
```

**3. 注册 WidgetType 枚举：**

在 `include/ltgui/widget.h` 的 `enum class WidgetType` 中添加 `NewWidget,`。

**4. 添加 header 排序：**

如果导出的 SDK 头文件需要这个 widget，在 `ltgui.py` 的 `_HEADER_ORDER` 列表中添加 `"widgets/newwidget.h"`。

**5. 编译：** `python ltgui.py build` — 构建脚本自动扫描 `src/` 下所有 `.cpp` 文件。

### 添加新的编译选项

编辑 `ltgui.py` 的 `compile_source()` 函数，在 `flags` 列表中添加。

### IntelliSense 配置

如果 VSCode 报 "file not found"（`widget.h` 等），创建 `.vscode/c_cpp_properties.json`：

```json
{
    "configurations": [
        {
            "name": "ltgui",
            "includePath": [
                "${workspaceFolder}/include",
                "${workspaceFolder}/vendor"
            ],
            "cppStandard": "c++17",
            "intelliSenseMode": "windows-clang-x64",
            "compilerPath": "C:/Program Files/LLVM/bin/clang++.exe"
        }
    ],
    "version": 4
}
```

---

## 编译器详细对比

| 特性 | clang++ | MSVC |
|------|---------|------|
| 安装大小 | ~2 GB | ~6 GB（含 Visual Studio） |
| 编译速度 | 极快 | 中等 |
| 错误信息 | 极其清晰（彩色、箭头指示） | 传统格式 |
| 标准合规度 | 最接近标准 | 有少量 Microsoft 扩展 |
| AddressSanitizer | ✅ 原生支持 | ✅ VS 2022 支持 |
| 调试器 | lldb / gdb | VS Debugger（最强大） |
| PATH 配置 | 安装时可自动添加 | 必须从 Developer Prompt 启动 |
| 跨平台 | ✅ | ❌ Windows only |
| RTTI | 默认开启 | 默认开启 |

**推荐组合：** 用 `clang++` 编译，用 Visual Studio 的调试器（附加到进程）。编译快 + 调试强。

---

## 版本历史

| 日期 | 说明 |
|------|------|
| 2026-06 | 初始版本，覆盖 clang++/MSVC 双编译器完整流程 |

---

*最后更新：2026-06-04*
