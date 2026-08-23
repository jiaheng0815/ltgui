# ltgui Build & Debug Guide / 构建与调试指南

From-scratch compiler setup, build, and debug walkthrough.
从零开始：编译器安装、源码下载、编译、VSCode/VS 调试配置。

[English](#english) | [中文](#中文)

---

# English

## Table of Contents

- [Prerequisites](#prerequisites)
- [Option A: clang++ (Recommended)](#option-a-clang-recommended)
- [Option B: MSVC](#option-b-msvc)
- [CMake Build (Alternative)](#cmake-build-alternative)
- [Build Output](#build-output)
- [Tests](#tests)
- [Run Examples](#run-examples)
- [Troubleshooting](#troubleshooting)
- [Project Extension](#project-extension)
- [Compiler Comparison](#compiler-comparison)

---

## Prerequisites

You need Python 3 and a C++20 compiler. ltgui's build script (`ltgui.py`) auto-detects your platform and picks the right compiler.

| Compiler | Windows | Linux | macOS |
|----------|---------|-------|-------|
| clang++ | ✅ Recommended | ✅ Recommended | ✅ Recommended |
| MSVC (cl.exe) | ✅ | — | — |
| g++ | ✅ | ✅ | ✅ |

---

## Option A: clang++ (Recommended)

clang++ is the LLVM C++ compiler. On Windows it works standalone — no Visual Studio required.

### 1. Install LLVM

**Method 1: Official GitHub Release (fastest)**

Open https://github.com/llvm/llvm-project/releases, find the latest stable release (e.g. LLVM 19.1.x), download:

| Arch | Filename |
|------|----------|
| 64-bit | `LLVM-19.1.x-win64.exe` |
| 32-bit | `LLVM-19.1.x-win32.exe` |

Run the installer. **Critical step:** check "Add LLVM to the system PATH for all users". Skipping this means PowerShell won't find `clang++`.

**Method 2: winget (command line)**

```powershell
winget install LLVM
```

Restart PowerShell after install.

**Method 3: scoop (devs' choice)**

```powershell
Set-ExecutionPolicy RemoteSigned -Scope CurrentUser
irm get.scoop.sh | iex
scoop install llvm
```

### 2. Verify

Open a **new** PowerShell window:

```powershell
clang++ --version
# Expected: clang version 19.1.0

python --version
# Expected: Python 3.x.x
```

If Python is missing, download from https://www.python.org/downloads/, check "Add Python to PATH".

### 3. Clone

```powershell
git clone https://github.com/jiaheng0815/ltgui.git
cd ltgui
```

If you don't have git: `winget install Git.Git`

### 4. Build

```powershell
# Debug build (with -g -O0)
python ltgui.py build

# Release build (-O2 -DNDEBUG)
python ltgui.py build release

# Custom LLVM path (if clang++ isn't in PATH)
python ltgui.py build --compiler "C:\Program Files\LLVM\bin\clang++.exe"

# Build a single example
python ltgui.py build --example hello

# Build DLL + SDK header
python ltgui.py build --dll ./sdk
```

Incremental builds: the script tracks modification times of `.cpp` files and all transitive `#include` dependencies. Only changed files recompile.

### 5. VSCode Debugging

#### 5.1 Install Extensions

Press `Ctrl+Shift+X`, search and install:

| Extension | Purpose |
|-----------|---------|
| **C/C++** (Microsoft) | IntelliSense, debugger |
| **clangd** (LLVM) | Better code completion (optional) |

#### 5.2 Debug Configuration

Create `.vscode/launch.json`:

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
            "MIMode": "gdb",
            "miDebuggerPath": "C:\\Program Files\\LLVM\\bin\\lldb.exe",
            "setupCommands": [
                { "description": "Enable pretty-printing", "text": "-enable-pretty-printing", "ignoreFailures": true }
            ],
            "preLaunchTask": "ltgui: build debug"
        }
    ]
}
```

Create `.vscode/tasks.json`:

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "ltgui: build debug",
            "type": "shell",
            "command": "python",
            "args": ["ltgui.py", "build"],
            "group": { "kind": "build", "isDefault": true },
            "presentation": { "reveal": "always", "panel": "shared" },
            "problemMatcher": {
                "owner": "cpp",
                "fileLocation": ["absolute"],
                "pattern": {
                    "regexp": "^(.+\\.(?:cpp|h|mm)):(\\d+):(\\d+):\\s+(error|warning|note):\\s+(.*)$",
                    "file": 1, "line": 2, "column": 3, "severity": 4, "message": 5
                }
            }
        }
    ]
}
```

#### 5.3 Debug Flow

1. Set breakpoints with `F9`
2. Press `F5` — VSCode builds via preLaunchTask, then attaches lldb
3. Inspect variables, call stack, and expressions at breakpoints

If `lldb` path differs, find it with `where lldb`.

#### 5.4 IntelliSense Fix (if needed)

Create `.vscode/c_cpp_properties.json`:

```json
{
    "configurations": [{
        "name": "ltgui",
        "includePath": ["${workspaceFolder}/include", "${workspaceFolder}/vendor"],
        "cppStandard": "c++20",
        "intelliSenseMode": "windows-clang-x64",
        "compilerPath": "C:/Program Files/LLVM/bin/clang++.exe"
    }],
    "version": 4
}
```

---

## Option B: MSVC

MSVC is Microsoft's C++ compiler (`cl.exe`). It ships with Visual Studio or the free Build Tools.

### 1. Install Visual Studio

**Option 1: Visual Studio 2022 Community (free, includes IDE)**

1. Open https://visualstudio.microsoft.com/downloads/
2. Download **Visual Studio 2022 Community** installer
3. In the installer, on the "Workloads" tab, check:

   | Workload | Contents |
   |----------|----------|
   | **Desktop development with C++** | MSVC compiler, Windows SDK, debugger |

4. In the right-side "Installation details" panel, verify these are checked:
   - MSVC v143 - VS 2022 C++ x64/x86 build tools
   - Windows 11 SDK (10.0.22621.0 or newer)
   - C++ CMake tools for Windows (optional)

5. Click "Install" — wait for ~6-10 GB download.

**Option 2: Build Tools only (no IDE, CLI only)**

1. Open https://visualstudio.microsoft.com/downloads/#build-tools-for-visual-studio-2022
2. Download **Build Tools for Visual Studio 2022**
3. Run the installer, check "Desktop development with C++", install.

### 2. Open Developer Command Prompt

**`cl.exe` is NOT in the system PATH!** You must launch a special prompt.

| Name | How to open |
|------|------------|
| **Developer PowerShell for VS 2022** | Start menu search |
| **Developer Command Prompt for VS 2022** | Start menu search |
| **x64 Native Tools Command Prompt** | Start menu search |

Once opened, verify:
```cmd
cl
```
Should print Microsoft C/C++ compiler version info.

**Alternative — import into any PowerShell:**

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
```

### 3. Build

In the Developer PowerShell:

```powershell
cd D:\code\ltgui

# Debug build
python ltgui.py build --compiler msvc

# Release build
python ltgui.py build release --compiler msvc

# Run tests
python ltgui.py test --compiler msvc

# Run an example
python ltgui.py run demo --compiler msvc
```

If you see `Error: MSVC compiler 'cl' not found in PATH`, you're not in a Developer Prompt. Close this window and re-open from Start menu.

### 4. Visual Studio Debugging

Open project in VS via CMake:
```powershell
cmake -B build_vs -G "Visual Studio 17 2022" -A x64
```
Then open the generated `.sln` in Visual Studio.

Or use **Debug → Attach to Process** (`Ctrl+Alt+P`), select the running `demo.exe`, then set breakpoints in source files.

| MSVC Debug Shortcut | Key |
|--------------------|-----|
| Set breakpoint | `F9` |
| Start debugging | `F5` |
| Step into | `F11` |
| Step over | `F10` |
| Step out | `Shift+F11` |
| Inspect variable | Hover or "Locals" window |

---

## CMake Build (Alternative)

```powershell
# clang++ (Debug)
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++
cmake --build build

# MSVC (Release)
cmake -B build_msvc -G "Visual Studio 17 2022" -A x64
cmake --build build_msvc --config Release

# Run tests
ctest --test-dir build
```

---

## Build Output

```
build/
├── lib/
│   ├── ltgui.lib          ← static library
│   ├── ltgui.dll          ← shared library (only with --dll)
│   └── libltgui.dll.a     ← import library (only with --dll)
├── obj/
│   ├── animation.cpp.o    ← intermediate object files
│   └── ...
├── demo.exe               ← full widget showcase
├── hello.exe              ← minimal example
├── main.exe               ← app/ entry point
└── .platform              ← compiler fingerprint (auto-cleans on switch)
```

`python ltgui.py build --dll ./sdk` produces the shared-library SDK instead
(ltgui.dll + libltgui.dll.a + amalgamated ltgui.h + stb_truetype.h in `sdk/`).
Consumers link the static library with `-DLTGUI_STATIC` defined (Windows);
SDK consumers omit it and link the import library. `install --prefix` and
`package` cover the static path and are documented in README.md.

Key files:
- `vendor/stb_truetype.h` — font rasterization library (GPU text rendering)
- `font/Deng.ttf` — default CJK font (Deng family, 16 MB)
- `ltgui.py` — Python build script (main entry)
- `CLAUDE.md` — AI assistant project context

---

## Tests

```powershell
# Build and run all 24 test suites
python ltgui.py test

# Run a single test binary
python ltgui.py build
.\build\test_color.exe

# CMake
ctest --test-dir build --output-on-failure
```

Coverage: geometry, color encoding, UTF-8, event routing, layout, widget tree, style edge cases, animation edge cases.

---

## Run Examples

```powershell
# One-shot build + run
python ltgui.py run hello    # 2 buttons, counter
python ltgui.py run demo     # 21 widgets showcase

# Or two steps
python ltgui.py build
.\build\demo.exe
```

GPU acceleration is on by default. Check console output:
```
[Window][INFO ] GPU acceleration enabled: D3D11
```
If GPU init fails, falls back to GDI+/X11 CPU rendering automatically.

---

## Troubleshooting

### Q1: `--compiler msvc` says `cl not found`

You're in a regular PowerShell. Open **Developer PowerShell for VS 2022** from the Start menu and retry there.

### Q2: `clang++` not found

LLVM was installed without adding to PATH. Fix:
```powershell
# Temporary
$env:Path += ";C:\Program Files\LLVM\bin"

# Permanent: re-run LLVM installer, check "Add to PATH"

# Or use full path
python ltgui.py build --compiler "C:\Program Files\LLVM\bin\clang++.exe"
```

### Q3: Text not showing / blank window

Verify the font file exists: `ls D:\code\ltgui\font\Deng.ttf`

Console should log:
```
[GPU][INFO ] Loaded default font: D:/code/ltgui/font/Deng.ttf
```
If you see `No system font found`, the font file is missing or corrupted. Re-clone the repo.

### Q4: GPU mode shows all-white screen

D3D11 init failed. Check console:
```
[GPU][INFO ] No GPU found, falling back to CPU rendering.
```
Causes: corrupted DirectX runtime → run `dxdiag`. Or outdated GPU drivers → update.

### Q5: "LF will be replaced by CRLF" warnings

Git line-ending normalization on Windows. Harmless. To silence:
```powershell
git config core.autocrlf true
```

### Q6: Python `capture_output=True` not supported

Python too old. `capture_output` needs Python 3.7+. Upgrade at https://www.python.org/downloads/.

### Q7: Deng.ttf is 16 MB

Normal — full CJK glyph set embedded. Use Git LFS if needed:
```
*.ttf filter=lfs diff=lfs merge=lfs -text
```

### Q8: lldb says "Unable to find executable"

You haven't built yet. Run `python ltgui.py build` before launching debug.

---

## Project Extension

### Adding a New Widget

**Header `include/ltgui/widgets/newwidget.h`:**
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

protected:
    void paintSelf(NativeCanvas* canvas) override;
    bool handleEvent(Event& event) override;

private:
    std::string data_;
};

} // namespace ltgui
```

**Impl `src/widgets/newwidget.cpp`:**
```cpp
#include "widgets/newwidget.h"
#include "theme.h"
#include "platform/native_canvas.h"

namespace ltgui {

NewWidget::NewWidget(Widget* parent) : Widget(parent) {
    style().bgColor = currentTheme().bgSecondary;
}

Size NewWidget::sizeHint() const {
    if (!sizeHintDirty()) return cachedSizeHint();
    setCachedSizeHint({100, 24});
    return cachedSizeHint();
}

void NewWidget::paintSelf(NativeCanvas* canvas) {
    Rect r = absoluteRect();
    // ... draw code ...
}

bool NewWidget::handleEvent(Event& event) {
    // ... event handling ...
    return false;
}

} // namespace ltgui
```

**Register enum:** Add `NewWidget` to `enum class WidgetType` in `include/ltgui/widget.h`.

**SDK header order:** Add `"widgets/newwidget.h"` to `_HEADER_ORDER` in `ltgui.py`.

Then `python ltgui.py build` — the build script auto-scans all `.cpp` files under `src/`.

---

## Compiler Comparison

| Feature | clang++ | MSVC |
|---------|---------|------|
| Install size | ~2 GB | ~6 GB (with VS) |
| Compile speed | Very fast | Moderate |
| Error messages | Excellent (color, arrows) | Traditional format |
| Standards compliance | Closest to standard | Some MS extensions |
| AddressSanitizer | ✅ Native | ✅ VS 2022 |
| Debugger | lldb / gdb | VS Debugger (best) |
| PATH config | Auto on install | Developer Prompt only |
| Cross-platform | ✅ | ❌ Windows only |
| RTTI | Default on | Default on |

**Recommended combo:** Compile with `clang++`, debug with Visual Studio (attach-to-process). Fast compile + best debugger.

---

# 中文

## 目录

- [前置条件](#前置条件)
- [方案 A：clang++（推荐）](#方案-aclang推荐)
- [方案 B：MSVC](#方案-bmsvc)
- [CMake 构建（替代方案）](#cmake-构建替代方案)
- [编译产物](#编译产物)
- [测试](#测试)
- [运行示例](#运行示例)
- [常见问题排查](#常见问题排查)
- [项目扩展](#项目扩展)
- [编译器对比](#编译器对比)

---

## 前置条件

需要 Python 3 和一个 C++20 编译器。ltgui 的构建脚本（`ltgui.py`）自动检测平台并选择合适的编译器。

| 编译器 | Windows | Linux | macOS |
|--------|---------|-------|-------|
| clang++ | ✅ 推荐 | ✅ 推荐 | ✅ 推荐 |
| MSVC (cl.exe) | ✅ | — | — |
| g++ | ✅ | ✅ | ✅ |

---

## 方案 A：clang++（推荐）

clang++ 是 LLVM 项目的 C++ 编译器。在 Windows 上无需 Visual Studio，可独立使用。

### 1. 安装 LLVM

**方式一：官方 GitHub Release（最快）**

打开 https://github.com/llvm/llvm-project/releases ，找到最新稳定版（如 LLVM 19.1.x），下载：

| 架构 | 文件名 |
|------|--------|
| 64 位 | `LLVM-19.1.x-win64.exe` |
| 32 位 | `LLVM-19.1.x-win32.exe` |

运行安装器。**关键步骤：勾选** "Add LLVM to the system PATH for all users"。不勾选的话 PowerShell 找不到 `clang++`。

**方式二：winget（命令行）**

```powershell
winget install LLVM
```

安装后重新打开 PowerShell。

**方式三：scoop（开发者推荐）**

```powershell
Set-ExecutionPolicy RemoteSigned -Scope CurrentUser
irm get.scoop.sh | iex
scoop install llvm
```

### 2. 验证安装

打开**新的** PowerShell 窗口：

```powershell
clang++ --version
# 预期输出：clang version 19.1.0

python --version
# 预期输出：Python 3.x.x
```

如果没有 Python，从 https://www.python.org/downloads/ 下载安装，勾选 "Add Python to PATH"。

### 3. 克隆仓库

```powershell
git clone https://github.com/jiaheng0815/ltgui.git
cd ltgui
```

如果没有 git：`winget install Git.Git`

### 4. 编译

```powershell
# Debug 构建（含 -g -O0）
python ltgui.py build

# Release 构建（-O2 -DNDEBUG）
python ltgui.py build release

# 指定 LLVM 路径（clang++ 不在 PATH 里时）
python ltgui.py build --compiler "C:\Program Files\LLVM\bin\clang++.exe"

# 只编译某个示例
python ltgui.py build --example hello

# 构建 DLL + SDK 头文件
python ltgui.py build --dll ./sdk
```

增量编译：脚本自动追踪 `.cpp` 文件和所有传递依赖的 `#include` 头文件的修改时间。只改了一个文件就只重编译一个文件。

### 5. VSCode 调试

#### 5.1 安装扩展

按 `Ctrl+Shift+X`，搜索安装：

| 扩展 | 用途 |
|------|------|
| **C/C++**（微软） | IntelliSense、调试器 |
| **clangd**（LLVM） | 更好的代码补全（可选） |

#### 5.2 调试配置

创建 `.vscode/launch.json`：

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
            "MIMode": "gdb",
            "miDebuggerPath": "C:\\Program Files\\LLVM\\bin\\lldb.exe",
            "setupCommands": [
                { "description": "启用美化打印", "text": "-enable-pretty-printing", "ignoreFailures": true }
            ],
            "preLaunchTask": "ltgui: build debug"
        }
    ]
}
```

创建 `.vscode/tasks.json`：

```json
{
    "version": "2.0.0",
    "tasks": [
        {
            "label": "ltgui: build debug",
            "type": "shell",
            "command": "python",
            "args": ["ltgui.py", "build"],
            "group": { "kind": "build", "isDefault": true },
            "presentation": { "reveal": "always", "panel": "shared" },
            "problemMatcher": {
                "owner": "cpp",
                "fileLocation": ["absolute"],
                "pattern": {
                    "regexp": "^(.+\\.(?:cpp|h|mm)):(\\d+):(\\d+):\\s+(error|warning|note):\\s+(.*)$",
                    "file": 1, "line": 2, "column": 3, "severity": 4, "message": 5
                }
            }
        }
    ]
}
```

#### 5.3 调试流程

1. 按 `F9` 设断点
2. 按 `F5` —— VSCode 先通过 preLaunchTask 编译，然后附加 lldb
3. 断点命中后检查变量、调用栈、表达式

如果 `lldb` 路径不对，用 `where lldb` 查找实际路径。

#### 5.4 IntelliSense 修复

如果报 "file not found"，创建 `.vscode/c_cpp_properties.json`：

```json
{
    "configurations": [{
        "name": "ltgui",
        "includePath": ["${workspaceFolder}/include", "${workspaceFolder}/vendor"],
        "cppStandard": "c++20",
        "intelliSenseMode": "windows-clang-x64",
        "compilerPath": "C:/Program Files/LLVM/bin/clang++.exe"
    }],
    "version": 4
}
```

---

## 方案 B：MSVC

MSVC 是微软的 C++ 编译器（`cl.exe`）。它随 Visual Studio 发布，也有免费的 Build Tools 版本。

### 1. 安装 Visual Studio

**方式一：Visual Studio 2022 Community（免费，含 IDE）**

1. 打开 https://visualstudio.microsoft.com/zh-hans/downloads/
2. 下载 **Visual Studio 2022 Community** 安装器
3. 在安装器的"工作负荷"标签页勾选：

   | 工作负荷 | 内容 |
   |---------|------|
   | **使用 C++ 的桌面开发** | MSVC 编译器、Windows SDK、调试器 |

4. 在右侧"安装详细信息"面板确认以下组件已勾选：
   - MSVC v143 - VS 2022 C++ x64/x86 生成工具
   - Windows 11 SDK（10.0.22621.0 或更新）
   - 适用于 Windows 的 C++ CMake 工具（可选）

5. 点击"安装"，等待完成（约 6-10 GB）。

**方式二：仅 Build Tools（无 IDE，纯命令行）**

1. 打开 https://visualstudio.microsoft.com/zh-hans/downloads/#build-tools-for-visual-studio-2022
2. 下载 **Build Tools for Visual Studio 2022**
3. 运行安装器，勾选"使用 C++ 的桌面开发"，安装。

### 2. 打开开发者命令行

**`cl.exe` 不在系统 PATH 里！** 必须从专门命令行启动。

| 名称 | 打开方式 |
|------|---------|
| **Developer PowerShell for VS 2022** | 开始菜单搜索 |
| **Developer Command Prompt for VS 2022** | 开始菜单搜索 |
| **x64 Native Tools Command Prompt** | 开始菜单搜索 |

打开后验证：
```cmd
cl
```
应输出 Microsoft C/C++ 编译器版本信息。

**备用方案——在任何 PowerShell 中导入环境：**

```powershell
& "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat"
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

如果提示 `Error: MSVC compiler 'cl' not found in PATH`，说明你不是在 Developer PowerShell 里。关掉当前窗口，从开始菜单重新打开。

### 4. Visual Studio 调试

通过 CMake 在 VS 中打开项目：
```powershell
cmake -B build_vs -G "Visual Studio 17 2022" -A x64
```
然后用 VS 打开生成的 `.sln` 文件。

或使用 **调试 → 附加到进程**（`Ctrl+Alt+P`），选择运行中的 `demo.exe`，在源码中设断点。

| MSVC 调试快捷键 | 按键 |
|----------------|------|
| 设断点 | `F9` |
| 启动调试 | `F5` |
| 逐语句 | `F11` |
| 逐过程 | `F10` |
| 跳出 | `Shift+F11` |
| 查看变量 | 鼠标悬停或"局部变量"窗口 |

---

## CMake 构建（替代方案）

```powershell
# clang++（Debug）
cmake -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_CXX_COMPILER=clang++
cmake --build build

# MSVC（Release）
cmake -B build_msvc -G "Visual Studio 17 2022" -A x64
cmake --build build_msvc --config Release

# 运行测试
ctest --test-dir build
```

---

## 编译产物

```
build/
├── lib/
│   └── ltgui.lib          ← 静态库
├── obj/
│   ├── animation.cpp.o    ← 中间目标文件
│   └── ...
├── demo.exe               ← 完整 widget 展示
├── hello.exe              ← 最小示例
├── main.exe               ← app/ 入口
└── .platform              ← 编译器指纹（切换时自动 clean）
```

关键文件：
- `vendor/stb_truetype.h` — 字体光栅化库（GPU 文字渲染依赖）
- `font/Deng.ttf` — 默认中日韩字体（灯虹体系列，16 MB）
- `ltgui.py` — Python 构建脚本（主入口）
- `CLAUDE.md` — AI 助手的项目上下文

---

## 测试

```powershell
# 构建并运行全部 24 个测试套件
python ltgui.py test

# 运行单个测试
python ltgui.py build
.\build\test_color.exe

# CMake 方式
ctest --test-dir build --output-on-failure
```

覆盖范围：几何运算、颜色编码、UTF-8 编解码、事件路由、布局系统、Widget 树操作、样式边界、动画边界。

---

## 运行示例

```powershell
# 一步构建+运行
python ltgui.py run hello    # 最小示例：两个按钮，计数器
python ltgui.py run demo     # 完整示例：14 种 widget 展示

# 或两步操作
python ltgui.py build
.\build\demo.exe
```

GPU 加速默认开启。控制台输出：
```
[Window][INFO ] GPU acceleration enabled: D3D11
```
GPU 初始化失败时自动回退到 GDI+/X11 CPU 渲染。

---

## 常见问题排查

### Q1：`--compiler msvc` 报找不到 cl

你在普通 PowerShell 里。从开始菜单打开 **Developer PowerShell for VS 2022**，在那里面重试。

### Q2：找不到 clang++

安装 LLVM 时没勾选 "Add to PATH"。修复：
```powershell
# 临时
$env:Path += ";C:\Program Files\LLVM\bin"

# 永久：重新运行 LLVM 安装器，勾选 "Add to PATH"

# 或用完整路径
python ltgui.py build --compiler "C:\Program Files\LLVM\bin\clang++.exe"
```

### Q3：文字不显示 / 窗口空白

确认字体文件存在：`ls D:\code\ltgui\font\Deng.ttf`

控制台应输出：
```
[GPU][INFO ] Loaded default font: D:/code/ltgui/font/Deng.ttf
```
如果看到 `No system font found`，字体文件缺失或损坏，重新 clone 仓库。

### Q4：GPU 模式全白屏

D3D11 初始化失败。检查控制台：
```
[GPU][INFO ] No GPU found, falling back to CPU rendering.
```
原因：DirectX 运行时损坏 → 运行 `dxdiag` 检查；或显卡驱动过旧 → 更新驱动。

### Q5："LF will be replaced by CRLF" 警告

Git 在 Windows 上的行尾转换，不影响编译。消除方式：
```powershell
git config core.autocrlf true
```

### Q6：Python 报 `capture_output=True` 不支持

Python 版本太旧，需要 3.7+。在 https://www.python.org/downloads/ 升级。

### Q7：Deng.ttf 有 16 MB

正常——内嵌了完整的中日韩字形集。如需 Git LFS：
```
*.ttf filter=lfs diff=lfs merge=lfs -text
```

### Q8：lldb 报 "Unable to find executable"

还没有编译。先执行 `python ltgui.py build` 生成可执行文件，再启动调试。

---

## 项目扩展

### 添加新 Widget

**头文件 `include/ltgui/widgets/newwidget.h`：**
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

protected:
    void paintSelf(NativeCanvas* canvas) override;
    bool handleEvent(Event& event) override;

private:
    std::string data_;
};

} // namespace ltgui
```

**实现文件 `src/widgets/newwidget.cpp`：**
```cpp
#include "widgets/newwidget.h"
#include "theme.h"
#include "platform/native_canvas.h"

namespace ltgui {

NewWidget::NewWidget(Widget* parent) : Widget(parent) {
    style().bgColor = currentTheme().bgSecondary;
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

**注册枚举：** 在 `include/ltgui/widget.h` 的 `enum class WidgetType` 中添加 `NewWidget`。

**SDK header 排序：** 在 `ltgui.py` 的 `_HEADER_ORDER` 列表中添加 `"widgets/newwidget.h"`。

然后执行 `python ltgui.py build`——构建脚本自动扫描 `src/` 下所有 `.cpp` 文件。

---

## 编译器对比

| 特性 | clang++ | MSVC |
|------|---------|------|
| 安装大小 | ~2 GB | ~6 GB（含 VS） |
| 编译速度 | 极快 | 中等 |
| 错误信息 | 极其清晰（彩色、箭头指示） | 传统格式 |
| 标准合规度 | 最接近标准 | 有少量 MS 扩展 |
| AddressSanitizer | ✅ 原生支持 | ✅ VS 2022 |
| 调试器 | lldb / gdb | VS 调试器（最强） |
| PATH 配置 | 安装时自动添加 | 必须从 Developer Prompt 启动 |
| 跨平台 | ✅ | ❌ 仅 Windows |
| RTTI | 默认开启 | 默认开启 |

**推荐组合：** 用 `clang++` 编译，用 Visual Studio 调试（附加到进程）。编译快 + 调试强。

---

*最后更新 / Last updated: 2026-06-04*
