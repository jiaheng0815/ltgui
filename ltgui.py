#!/usr/bin/env python3
"""ltgui build system — python ltgui.py [command] [options]"""

import os
import sys
import subprocess
import shutil

# --- Color support ---
if sys.platform == "win32":
    try:
        import ctypes
        ctypes.windll.kernel32.SetConsoleMode(ctypes.windll.kernel32.GetStdHandle(-11), 7)
    except Exception:
        pass

class Color:
    RED     = "\033[91m"
    GREEN   = "\033[92m"
    YELLOW  = "\033[93m"
    BLUE    = "\033[94m"
    MAGENTA = "\033[95m"
    CYAN    = "\033[96m"
    WHITE   = "\033[97m"
    BOLD    = "\033[1m"
    RESET   = "\033[0m"

def cprint(msg, color=Color.WHITE, bold=False, end="\n"):
    prefix = Color.BOLD if bold else ""
    print(f"{prefix}{color}{msg}{Color.RESET}", end=end)

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_DIR = os.path.join(SCRIPT_DIR, "src")
INCLUDE_DIR = os.path.join(SCRIPT_DIR, "include", "ltgui")
VENDOR_DIR = os.path.join(SCRIPT_DIR, "vendor")
EXAMPLES_DIR = os.path.join(SCRIPT_DIR, "examples")
APP_DIR      = os.path.join(SCRIPT_DIR, "app")
BUILD_DIR = os.path.join(SCRIPT_DIR, "build")
OBJ_DIR = os.path.join(BUILD_DIR, "obj")
LIB_DIR = os.path.join(BUILD_DIR, "lib")
TEST_DIR = os.path.join(SCRIPT_DIR, "test")

# --- Platform helpers ---
def detect_platform():
    if sys.platform == "win32":
        return "windows"
    elif sys.platform.startswith("linux"):
        return "linux"
    elif sys.platform == "darwin":
        return "macos"
    else:
        raise RuntimeError(f"Unsupported platform: {sys.platform}")

def check_platform_clean():
    """Auto-clean if platform changed since last build."""
    fingerprint = os.path.join(BUILD_DIR, ".platform")
    current = detect_platform()
    if os.path.exists(fingerprint):
        with open(fingerprint, "r") as f:
            previous = f.read().strip()
        if previous != current:
            cprint(f"Platform changed ({previous} -> {current}), cleaning...", Color.YELLOW)
            if os.path.exists(BUILD_DIR):
                shutil.rmtree(BUILD_DIR)
    os.makedirs(BUILD_DIR, exist_ok=True)
    with open(fingerprint, "w") as f:
        f.write(current)

# --- Compiler resolution ---
def resolve_compiler(compiler_arg):
    """Resolve --compiler flag to the C++ compiler binary name."""
    if compiler_arg is None:
        return "clang++"
    c = compiler_arg.lower()
    if c in ("clang", "clang++"):
        return "clang++"
    if c in ("gcc", "g++", "gcc++"):
        return "g++"
    return compiler_arg  # custom path

# --- Flag parsing ---
def parse_flags(args):
    """Parse --key value flags from args.
    Returns (positional_args, flags_dict).
    Boolean flags (no value) get the value True.
    """
    positional = []
    flags = {}
    i = 0
    while i < len(args):
        if args[i].startswith("--"):
            key = args[i][2:]
            if i + 1 < len(args) and not args[i + 1].startswith("--"):
                flags[key] = args[i + 1]
                i += 2
            else:
                flags[key] = True
                i += 1
        else:
            positional.append(args[i])
            i += 1
    return positional, flags

# --- Platform libs / flags ---
def get_platform_libs(platform):
    if platform == "windows":
        return ["-lgdi32", "-lgdiplus", "-luser32", "-lcomctl32", "-lole32", "-limm32",
                "-ld3d11", "-ldxgi", "-ld3dcompiler"]
    elif platform == "linux":
        return ["-lX11", "-lXft", "-lXrender", "-lfontconfig", "-lEGL", "-lGLESv2"]
    elif platform == "macos":
        return ["-framework", "Cocoa", "-framework", "CoreGraphics", "-framework", "CoreText"]
    return []

def get_platform_flags(platform):
    if platform == "windows":
        return ["-D_UNICODE", "-DUNICODE"]
    elif platform == "linux":
        return ["-I/usr/include/freetype2"]
    return []

def get_source_files():
    sources = []
    for root, dirs, files in os.walk(SRC_DIR):
        for f in files:
            if f.endswith(".cpp") or f.endswith(".mm"):
                sources.append(os.path.join(root, f))
    return sorted(sources)

# --- Build steps ---
def _get_direct_includes(filepath):
    """Parse #include \"...\" directives from a source/header file. Returns list of resolved paths."""
    includes = []
    try:
        with open(filepath, "r", encoding="utf-8", errors="ignore") as f:
            for line in f:
                line = line.strip()
                if line.startswith("#include \""):
                    # Extract the quoted path
                    start = line.index('"') + 1
                    end = line.index('"', start)
                    inc = line[start:end]
                    # Try relative to the file's directory first, then INCLUDE_DIR
                    candidates = [
                        os.path.join(os.path.dirname(filepath), inc),
                        os.path.join(INCLUDE_DIR, inc),
                    ]
                    for c in candidates:
                        if os.path.exists(c):
                            includes.append(os.path.normpath(c))
                            break
    except Exception:
        pass
    return includes

def _get_all_dependencies(src_path, visited=None):
    """Recursively collect all header dependencies for a source file."""
    if visited is None:
        visited = set()
    src_norm = os.path.normpath(src_path)
    if src_norm in visited:
        return set()
    visited.add(src_norm)
    deps = {src_norm}
    for inc in _get_direct_includes(src_norm):
        deps.update(_get_all_dependencies(inc, visited))
    return deps

def _object_is_fresh(obj_path, src_path):
    """Check if .o is newer than .cpp AND all transitive header dependencies."""
    if not os.path.exists(obj_path):
        return False
    obj_mtime = os.path.getmtime(obj_path)
    deps = _get_all_dependencies(src_path)
    for dep in deps:
        if os.path.getmtime(dep) > obj_mtime:
            return False
    return True

def compile_source(src_path, platform, is_release, compiler, pic=False):
    rel = os.path.relpath(src_path, SRC_DIR)
    obj_path = os.path.join(OBJ_DIR, rel + ".o")
    os.makedirs(os.path.dirname(obj_path), exist_ok=True)

    if _object_is_fresh(obj_path, src_path):
        return obj_path

    flags = [compiler, "-c", "-std=c++17", "-fexec-charset=UTF-8"]
    if pic:
        flags.append("-fPIC")
    if is_release:
        flags += ["-O2", "-DNDEBUG"]
    else:
        flags += ["-g", "-O0"]

    flags += ["-Wall", "-Wextra", "-Wpedantic"]
    # Not errors in ltgui, but noisy from vendor/system headers:
    flags += ["-Wno-unused-parameter", "-Wno-missing-field-initializers"]

    flags += ["-I", INCLUDE_DIR, "-I", VENDOR_DIR]
    flags += get_platform_flags(platform)
    flags += [src_path, "-o", obj_path]

    cprint(f"  Compiling {rel}...", Color.CYAN)
    result = subprocess.run(flags, capture_output=True, text=True)
    if result.returncode != 0:
        cprint(f"  Error compiling {rel}:", Color.RED, bold=True)
        cprint(result.stderr, Color.RED)
        return None
    return obj_path

def build_shared_lib(platform, is_release, compiler):
    """Build a dynamic/shared library (.dll / .so / .dylib)."""
    sources = get_source_files()
    if not sources:
        cprint("No source files found in src/", Color.RED)
        return None

    build_type = "release" if is_release else "debug"
    cprint(f"Building ltgui shared library ({build_type}) for {platform} with {compiler}...", Color.BLUE, bold=True)
    cprint(f"Found {len(sources)} source files.", Color.WHITE)

    object_files = []
    for src in sources:
        obj = compile_source(src, platform, is_release, compiler, pic=True)
        if obj:
            object_files.append(obj)
        else:
            return None

    os.makedirs(LIB_DIR, exist_ok=True)

    if platform == "windows":
        dll_name = "ltgui.dll"
    elif platform == "macos":
        dll_name = "libltgui.dylib"
    else:
        dll_name = "libltgui.so"
    dll_path = os.path.join(LIB_DIR, dll_name)

    cprint(f"  Linking {dll_name}...", Color.CYAN)

    flags = [compiler, "-shared", "-std=c++17"]
    if platform != "macos":
        flags.append("-static")
    if platform == "windows":
        flags.append("-Wl,--export-all-symbols")
    if is_release:
        flags += ["-O2", "-DNDEBUG"]
    else:
        flags += ["-g", "-O0"]
    flags += get_platform_flags(platform)
    flags += object_files
    flags += get_platform_libs(platform)
    flags += ["-o", dll_path]

    result = subprocess.run(flags, capture_output=True, text=True)
    if result.returncode != 0:
        cprint(f"  Error linking shared library: {result.stderr}", Color.RED)
        return None

    cprint(f"  Created {dll_path}", Color.GREEN)
    return dll_path

def build_lib(platform, is_release, compiler):
    sources = get_source_files()
    if not sources:
        cprint("No source files found in src/", Color.RED)
        return None

    build_type = "release" if is_release else "debug"
    cprint(f"Building ltgui ({build_type}) for {platform} with {compiler}...", Color.BLUE, bold=True)
    cprint(f"Found {len(sources)} source files.", Color.WHITE)

    object_files = []
    for src in sources:
        obj = compile_source(src, platform, is_release, compiler)
        if obj:
            object_files.append(obj)
        else:
            return None

    os.makedirs(LIB_DIR, exist_ok=True)
    lib_name = "ltgui.lib" if platform == "windows" else "libltgui.a"
    lib_path = os.path.join(LIB_DIR, lib_name)

    cprint(f"  Creating {lib_name}...", Color.CYAN)

    if platform == "windows" and "clang" in compiler:
        # llvm-lib for clang toolchain on Windows
        result = subprocess.run(
            ["llvm-lib"] + object_files + ["/OUT:" + lib_path],
            capture_output=True, text=True)
        if result.returncode != 0:
            result = subprocess.run(
                ["ar", "rcs", lib_path] + object_files,
                capture_output=True, text=True)
    else:
        result = subprocess.run(
            ["ar", "rcs", lib_path] + object_files,
            capture_output=True, text=True)

    if result.returncode != 0:
        cprint(f"  Error creating library: {result.stderr}", Color.RED)
        return None

    cprint(f"  Created {lib_path}", Color.GREEN)
    return lib_path

def build_example(name, platform, is_release, lib_path, compiler):
    src = os.path.join(EXAMPLES_DIR, name + ".cpp")
    if not os.path.exists(src):
        cprint(f"Example '{name}' not found at {src}", Color.RED)
        return False

    exe_path = os.path.join(BUILD_DIR, name + (".exe" if platform == "windows" else ""))

    flags = [compiler, "-std=c++17", "-fexec-charset=UTF-8"]
    if is_release:
        flags += ["-O2", "-DNDEBUG"]
    else:
        flags += ["-g", "-O0"]

    flags += ["-I", INCLUDE_DIR, "-I", VENDOR_DIR]
    flags += get_platform_flags(platform)
    flags += [src, lib_path]
    flags += get_platform_libs(platform)
    flags += ["-o", exe_path]

    cprint(f"  Building example: {name}...", Color.CYAN)
    result = subprocess.run(flags, capture_output=True, text=True)
    if result.returncode != 0:
        cprint(f"  Error building {name}:", Color.RED, bold=True)
        cprint(result.stderr, Color.RED)
        return False

    cprint(f"  Created {exe_path}", Color.GREEN)
    return True

def build_app(name, platform, is_release, lib_path, compiler):
    src = os.path.join(APP_DIR, name + ".cpp")
    if not os.path.exists(src):
        cprint(f"App '{name}' not found at {src}", Color.RED)
        return False

    exe_path = os.path.join(BUILD_DIR, name + (".exe" if platform == "windows" else ""))

    flags = [compiler, "-std=c++17", "-fexec-charset=UTF-8"]
    if is_release:
        flags += ["-O2", "-DNDEBUG"]
    else:
        flags += ["-g", "-O0"]

    flags += ["-I", INCLUDE_DIR, "-I", VENDOR_DIR]
    flags += get_platform_flags(platform)
    flags += [src, lib_path]
    flags += get_platform_libs(platform)
    flags += ["-o", exe_path]

    cprint(f"  Building app: {name}...", Color.CYAN)
    result = subprocess.run(flags, capture_output=True, text=True)
    if result.returncode != 0:
        cprint(f"  Error building {name}:", Color.RED, bold=True)
        cprint(result.stderr, Color.RED)
        return False

    cprint(f"  Created {exe_path}", Color.GREEN)
    return True

# --- SDK export ---

def _find_matching_brace(text, start):
    """Return index of matching '}' for '{' at text[start], or -1."""
    depth = 0
    for i in range(start, len(text)):
        if text[i] == '{':
            depth += 1
        elif text[i] == '}':
            depth -= 1
            if depth == 0:
                return i
    return -1

def _strip_function_bodies(text):
    """Replace function bodies { ... } with ; for inline/in-class definitions.
    Preserves class/struct/enum/namespace bodies and control-flow blocks.
    """
    # Keywords whose opening brace is NOT a function body
    control = {'if', 'for', 'while', 'switch', 'catch', 'return',
               'namespace', 'class', 'struct', 'enum', 'union',
               'extern', 'static_cast', 'dynamic_cast', 'const_cast',
               'reinterpret_cast', 'sizeof', 'decltype', 'alignof',
               'static_assert', 'noexcept', 'try', 'thread_local'}

    out = []
    pos = 0
    n = len(text)
    while pos < n:
        # Find next opening brace
        brace = text.find('{', pos)
        if brace < 0:
            out.append(text[pos:])
            break

        # Look backwards from '{' to find what precedes it
        before = text[pos:brace].rstrip()

        # Only examine the immediate context before '{' to determine if it starts
        # a function body. Scan backwards from '{' to the nearest blank line,
        # '}', ';', or '{' — that's the function-signature window.
        sig_start = max(pos, brace - 1)
        while sig_start > pos:
            c = text[sig_start - 1]
            if c in ('}', ';', '{'):
                break
            if c == '\n':
                if sig_start > pos + 1 and text[sig_start - 2] == '\n':
                    break
            sig_start -= 1
        sig = text[sig_start:brace]
        bad = {';', '}', 'public:', 'protected:', 'private:'}
        if any(kw in sig for kw in bad):
            # Non-function brace — skip the entire { ... } block so the next
            # function's signature window won't include this block's closing }.
            end_brace = _find_matching_brace(text, brace)
            if end_brace >= 0:
                out.append(text[pos:end_brace + 1])
                pos = end_brace + 1
                while pos < n and text[pos] in ' \t\r\n':
                    pos += 1
            else:
                out.append(text[pos:brace + 1])
                pos = brace + 1
            continue

        # Determine if this brace starts a function body: preceded by ')' or ')' + qualifiers
        is_func_body = False
        if before.endswith(')'):
            # "){" or ") {" or ")\n{" — direct function body
            is_func_body = True
        else:
            # Check for ) followed by qualifiers then {
            # Supported: const, override, final, noexcept, volatile, &, &&
            qual_check = before
            while True:
                stripped = qual_check.rstrip()
                matched = False
                for q in ['const', 'override', 'final', 'noexcept', 'volatile', '&', '&&']:
                    if stripped.endswith(q):
                        qual_check = stripped[:-len(q)]
                        matched = True
                        break
                if not matched:
                    break
            if qual_check.rstrip().endswith(')'):
                is_func_body = True
            else:
                # Check for constructor initializer list: ) : ...  { ...
                # After skipping qualifiers, check if there's a ':' between ) and {
                ctor_check = before
                for q in ['const', 'override', 'final', 'noexcept']:
                    if ctor_check.rstrip().endswith(q):
                        ctor_check = ctor_check[:ctor_check.rstrip().rfind(q)]
                if ctor_check.rstrip().endswith(')') or ':' in ctor_check[ctor_check.rfind(')') if ')' in ctor_check else 0:]:
                    # Could be ) : init_list { — check more carefully
                    pass  # For now, simple case: if it ends with ) it's a func

        if is_func_body:
            # Verify it's not a control structure: check word before matching '('
            # Find the matching '(' for the last ')'
            paren_pos = len(before) - 1
            depth = 0
            found_paren = False
            for k in range(paren_pos, -1, -1):
                if before[k] == ')':
                    depth += 1
                elif before[k] == '(':
                    depth -= 1
                    if depth < 0:
                        found_paren = True
                        break
                elif before[k] == ';':
                    # This is not a function call/decl, likely a lambda or expression
                    break

            if found_paren:
                # Get the word before '('
                word_start = k - 1
                while word_start >= 0 and (before[word_start].isalnum() or before[word_start] == '_'):
                    word_start -= 1
                before_word = before[word_start+1:k].strip()
                # Also check for destructor: ~ClassName
                if before_word.startswith('~'):
                    before_word = before_word[1:]

                if before_word in control:
                    is_func_body = False

        if is_func_body:
            end_brace = _find_matching_brace(text, brace)
            if end_brace >= 0:
                decl = text[pos:brace]

                # Strip constructor initializer list: find ':' at nesting depth 0
                # (not inside parens, not part of ::). "Foo(int x) : x_(x) " → "Foo(int x)"
                depth = 0
                colon_idx = -1
                for i in range(len(decl)):
                    if decl[i] == '(':
                        depth += 1
                    elif decl[i] == ')':
                        depth -= 1
                    elif decl[i] == ':' and depth == 0:
                        # Skip both colons of :: (continue skips current, next iter
                        # checks i-1 to detect this is the second colon).
                        if i > 0 and decl[i - 1] == ':':
                            continue
                        if i + 1 < len(decl) and decl[i + 1] == ':':
                            continue
                        colon_idx = i
                        break
                if colon_idx >= 0:
                    decl = decl[:colon_idx].rstrip()

                # Strip 'inline' keyword
                decl = decl.replace('inline ', '').replace('inline\n', '\n')

                # Skip out-of-line member defaults: :: appears BEFORE the first '('
                # meaning "ReturnType ClassName::method(...)". In-class methods may
                # have :: inside parameter types (after '('), those are kept.
                paren_pos = decl.find('(')
                if paren_pos >= 0 and '::' in decl[:paren_pos]:
                    pos = end_brace + 1
                    while pos < n and text[pos] in ' \t\r\n':
                        pos += 1
                    continue

                out.append(decl)
                out.append(';\n')
                pos = end_brace + 1
                while pos < n and text[pos] in ' \t\r\n':
                    pos += 1
                continue

        out.append(text[pos:brace + 1])
        pos = brace + 1

    return ''.join(out)

# Header ordering: topological sort so dependencies appear before dependents.
# Order follows the umbrella ltgui.h include list plus all transitive headers.
# Concrete platform backends (win32/x11/cocoa) are excluded — they are internal
# implementation details, not part of the public SDK API.
_HEADER_ORDER = [
    "geometry.h",
    "color.h",
    "font.h",
    "utf8.h",
    "platform/platform.h",
    "event.h",
    "style.h",
    "theme.h",
    "platform/native_canvas.h",
    "platform/native_window.h",
    "animation.h",
    "platform/gpu/gpu_device.h",
    "platform/gpu/gpu_font_atlas.h",
    "platform/gpu/gpu_detect.h",
    "platform/gpu/gpu_renderer.h",
    "platform/gpu/gpu_canvas.h",
    "app.h",
    "widget.h",
    "canvas.h",
    "layout.h",
    "window.h",
    "widgets/button.h",
    "widgets/label.h",
    "widgets/textbox.h",
    "widgets/checkbox.h",
    "widgets/radiobutton.h",
    "widgets/slider.h",
    "widgets/listbox.h",
    "widgets/scrollarea.h",
    "widgets/combobox.h",
    "widgets/progressbar.h",
    "widgets/tooltip.h",
    "widgets/tabwidget.h",
    "widgets/image.h",
    "widgets/treeview.h",
    "widgets/contextmenu.h",
]

def generate_amalgamated_header(target_path):
    """Generate a single ltgui.h by merging all public headers."""
    cprint("Generating amalgamated ltgui.h...", Color.CYAN)

    preamble = """\
// ltgui — single-header SDK generated by ltgui.py build --dll
// Include this header and link against ltgui.dll / libltgui.so / libltgui.dylib.
//
// Usage:
//   #include "ltgui.h"
//   #pragma comment(lib, "ltgui.lib")  // MSVC only
//   // or link with: -lltgui

#pragma once

// --- DLL import/export ---
#ifdef LTGUI_STATIC
#  define LTGUI_API
#elif defined(_WIN32) || defined(_WIN64)
#  ifdef LTGUI_EXPORTS
#    define LTGUI_API __declspec(dllexport)
#  else
#    define LTGUI_API __declspec(dllimport)
#  endif
#else
#  define LTGUI_API
#endif

"""

    system_includes = set()
    parts = []

    for rel_path in _HEADER_ORDER:
        h_path = os.path.join(INCLUDE_DIR, rel_path)
        if not os.path.exists(h_path):
            continue

        with open(h_path, "r", encoding="utf-8") as f:
            content = f.read()

        lines = content.split('\n')
        filtered = []
        for line in lines:
            s = line.strip()
            # Skip #pragma once
            if s.startswith('#pragma once'):
                continue
            # Skip internal includes (they're being amalgamated)
            if s.startswith('#include "') and ('ltgui/' in s or s.endswith('.h"') or 'stb_' in s):
                continue
            # Collect system includes for deduplication.
            # Skip stb_truetype — it's guarded by __has_include and must stay in place.
            if s.startswith('#include <') and 'stb_' not in s:
                system_includes.add(s)
                continue
            filtered.append(line)

        text = '\n'.join(filtered)

        parts.append(f"// --- {rel_path} ---\n")
        parts.append(text)
        parts.append('\n')

    # Assemble: preamble, then system includes (deduped, sorted), then body
    ordered_includes = sorted(system_includes)
    include_block = '\n'.join(ordered_includes)

    full = preamble + include_block + '\n\n' + ''.join(parts)

    os.makedirs(os.path.dirname(target_path), exist_ok=True)
    with open(target_path, "w", encoding="utf-8") as f:
        f.write(full)

    cprint(f"  Generated {target_path}", Color.GREEN)

def export_sdk(lib_path, target_dir, platform):
    """Build SDK: shared library + single declaration-only ltgui.h."""
    target_dir = os.path.abspath(target_dir)

    cprint(f"Exporting SDK to {target_dir}...", Color.BLUE, bold=True)

    # Generate amalgamated ltgui.h (declarations only)
    generate_amalgamated_header(os.path.join(target_dir, "ltgui.h"))

    # Copy shared library
    os.makedirs(target_dir, exist_ok=True)
    lib_name = os.path.basename(lib_path)
    shutil.copy2(lib_path, os.path.join(target_dir, lib_name))
    cprint(f"  {lib_name} -> {target_dir}", Color.CYAN)

    cprint("SDK export complete.", Color.GREEN, bold=True)

# --- Commands ---
def cmd_build(positional, flags):
    is_release = len(positional) >= 2 and positional[1] == "release"

    compiler = resolve_compiler(flags.get("compiler"))
    single_example = flags.get("example")
    single_app = flags.get("app")

    check_platform_clean()
    platform = detect_platform()

    # SDK export (--dll) — build shared lib only, skip examples/apps
    dll_dir = flags.get("dll")
    if dll_dir:
        lib_path = build_shared_lib(platform, is_release, compiler)
        if not lib_path:
            return
        export_sdk(lib_path, dll_dir, platform)
        cprint("\nBuild complete.", Color.GREEN, bold=True)
        return

    lib_path = build_lib(platform, is_release, compiler)
    if not lib_path:
        return

    if single_example:
        build_example(single_example, platform, is_release, lib_path, compiler)
    elif single_app:
        build_app(single_app, platform, is_release, lib_path, compiler)
    else:
        build_apps(platform, is_release, lib_path, compiler)
        build_examples(platform, is_release, lib_path, compiler)

    cprint("\nBuild complete.", Color.GREEN, bold=True)

def build_apps(platform, is_release, lib_path, compiler):
    if not os.path.exists(APP_DIR):
        return True

    apps = sorted([f[:-4] for f in os.listdir(APP_DIR) if f.endswith(".cpp")])
    if not apps:
        return True

    cprint("Building apps...", Color.BLUE, bold=True)
    for app in apps:
        if not build_app(app, platform, is_release, lib_path, compiler):
            return False
    return True

def build_examples(platform, is_release, lib_path, compiler):
    if not os.path.exists(EXAMPLES_DIR):
        return True

    examples = sorted([f[:-4] for f in os.listdir(EXAMPLES_DIR) if f.endswith(".cpp")])
    if not examples:
        return True

    cprint("Building examples...", Color.BLUE, bold=True)
    for ex in examples:
        if not build_example(ex, platform, is_release, lib_path, compiler):
            return False
    return True

def cmd_clean(positional, flags):
    if os.path.exists(BUILD_DIR):
        shutil.rmtree(BUILD_DIR)
        cprint("Cleaned build directory.", Color.GREEN)
    else:
        cprint("Nothing to clean.", Color.YELLOW)

def cmd_run(positional, flags):
    if len(positional) < 2:
        cprint("Usage: python ltgui.py run <name> [--compiler clang|gcc|...]", Color.YELLOW)
        apps = sorted([f[:-4] for f in os.listdir(APP_DIR) if f.endswith(".cpp")]) if os.path.exists(APP_DIR) else []
        examples = sorted([f[:-4] for f in os.listdir(EXAMPLES_DIR) if f.endswith(".cpp")]) if os.path.exists(EXAMPLES_DIR) else []
        if apps:
            cprint("Apps:", Color.WHITE)
            for a in apps:
                print(f"  {a}")
        if examples:
            cprint("Examples:", Color.WHITE)
            for e in examples:
                if e not in apps:
                    print(f"  {e}")
        return

    name = positional[1]
    compiler = resolve_compiler(flags.get("compiler"))

    # Determine if it's an app or example
    app_src = os.path.join(APP_DIR, name + ".cpp")
    ex_src = os.path.join(EXAMPLES_DIR, name + ".cpp")
    is_app = os.path.exists(app_src)

    if not is_app and not os.path.exists(ex_src):
        cprint(f"Program '{name}' not found in app/ or examples/", Color.RED)
        return

    check_platform_clean()
    platform = detect_platform()

    lib_path = build_lib(platform, is_release=False, compiler=compiler)
    if not lib_path:
        return

    if is_app:
        if not build_app(name, platform, False, lib_path, compiler):
            return
    else:
        if not build_example(name, platform, False, lib_path, compiler):
            return

    exe_name = name + (".exe" if platform == "windows" else "")
    exe_path = os.path.join(BUILD_DIR, exe_name)

    cprint(f"\nRunning {exe_name}...\n", Color.MAGENTA, bold=True)
    subprocess.run([exe_path])

def cmd_test(positional, flags):
    """Build and run all tests in test/."""
    compiler = resolve_compiler(flags.get("compiler"))

    check_platform_clean()
    platform = detect_platform()

    test_files = sorted([f for f in os.listdir(TEST_DIR) if f.endswith(".cpp")])
    if not test_files:
        cprint("No test files found in test/", Color.YELLOW)
        return

    lib_path = build_lib(platform, is_release=False, compiler=compiler)
    if not lib_path:
        cprint("Library build failed, cannot run tests.", Color.RED)
        return

    cprint(f"Building and running {len(test_files)} test(s)...\n", Color.BLUE, bold=True)

    passed = 0
    failed = 0

    for tf in test_files:
        src = os.path.join(TEST_DIR, tf)
        exe_name = tf[:-4] + (".exe" if platform == "windows" else "")
        exe_path = os.path.join(BUILD_DIR, exe_name)

        flags_list = [compiler, "-std=c++17", "-g", "-O0"]
        if platform == "windows":
            flags_list.append("-mconsole")
        flags_list += ["-I", INCLUDE_DIR, "-I", os.path.join(SCRIPT_DIR, "vendor")]
        flags_list += get_platform_flags(platform)
        flags_list += [src, lib_path]
        flags_list += get_platform_libs(platform)
        flags_list += ["-o", exe_path]

        cprint(f"  {tf}... ", Color.CYAN, end="")
        result = subprocess.run(flags_list, capture_output=True, text=True)
        if result.returncode != 0:
            cprint("COMPILE ERROR", Color.RED)
            for line in result.stderr.strip().split('\n')[-4:]:
                cprint(f"    {line.strip()}", Color.RED)
            failed += 1
            continue

        result = subprocess.run([exe_path], capture_output=True, text=True)
        if result.returncode == 0:
            cprint("PASS", Color.GREEN)
            passed += 1
        else:
            cprint("FAIL", Color.RED)
            for line in result.stdout.split('\n'):
                stripped = line.strip()
                if stripped and ('FAILED' in stripped or 'ERROR' in stripped or
                                 'assertion' in stripped.lower()):
                    cprint(f"    {stripped}", Color.RED)
            failed += 1

    print()
    total = passed + failed
    if failed == 0:
        cprint(f"  {passed}/{total} tests passed", Color.GREEN, bold=True)
    else:
        cprint(f"  {passed}/{total} passed, {failed} FAILED", Color.RED, bold=True)

def print_usage():
    cprint("ltgui build system", Color.BLUE, bold=True)
    print("Usage: python ltgui.py <command> [options]")
    print()
    print("Commands:")
    print("  build [release]     Build static library, apps, and examples")
    print("  clean               Remove build/ directory")
    print("  run <name>          Build and run an app or example")
    print("  test                Build and run all tests")
    print()
    print("Options:")
    print("  --compiler <val>    clang (default), gcc, or custom path")
    print("  --example <name>    Build only this example (with 'build')")
    print("  --app <name>        Build only this app (with 'build')")
    print("  --dll <dir>         Build shared library (.dll/.so/.dylib) + headers to dir")
    print()
    print("Examples:")
    print("  python ltgui.py build")
    print("  python ltgui.py build release --compiler gcc")
    print("  python ltgui.py build --compiler /usr/bin/g++-13 --example hello")
    print("  python ltgui.py build --dll ./sdk")
    print("  python ltgui.py run demo --compiler gcc")

def main():
    args = sys.argv[1:]
    positional, flags = parse_flags(args)

    if not positional:
        print_usage()
        return

    cmd = positional[0]
    if cmd == "build":
        cmd_build(positional, flags)
    elif cmd == "clean":
        cmd_clean(positional, flags)
    elif cmd == "run":
        cmd_run(positional, flags)
    elif cmd == "test":
        cmd_test(positional, flags)
    elif cmd in ("-h", "--help", "help"):
        print_usage()
    else:
        cprint(f"Unknown command: {cmd}", Color.RED)
        print_usage()

if __name__ == "__main__":
    main()
