#!/usr/bin/env python3
"""ltgui build system — python ltgui.py [command] [options]"""

import os
import sys
import time
import json
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
    if _JSON_MODE:
        return
    prefix = Color.BOLD if bold else ""
    print(f"{prefix}{color}{msg}{Color.RESET}", end=end)

def json_event(event_type, **data):
    """Emit a structured event as a JSON line. Used for CI with --json flag."""
    if _JSON_MODE:
        import json as _json
        print(_json.dumps({"event": event_type, **data}))

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

# --- Module-level state ---
_VERBOSE = False
_JSON_MODE = False
_JOBS = os.cpu_count() or 1
_COMPILE_COMMANDS = []
_PROFILE_BUILD = False

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

def check_platform_clean(compiler_id="clang++"):
    """Auto-clean if platform or compiler changed since last build."""
    fingerprint = os.path.join(BUILD_DIR, ".platform")
    current = f"{detect_platform()}:{compiler_id}"
    if os.path.exists(fingerprint):
        with open(fingerprint, "r") as f:
            previous = f.read().strip()
        if previous != current:
            cprint(f"Build config changed ({previous} -> {current}), cleaning...", Color.YELLOW)
            if os.path.exists(BUILD_DIR):
                shutil.rmtree(BUILD_DIR)
    os.makedirs(BUILD_DIR, exist_ok=True)
    with open(fingerprint, "w") as f:
        f.write(current)

# --- Compiler resolution ---
def resolve_compiler(compiler_arg):
    """Resolve --compiler flag to compiler ID."""
    if compiler_arg is None:
        # Auto-detect: prefer clang++, then MSVC on Windows, then g++
        if sys.platform == "win32":
            if shutil.which("clang++"):
                return ("clang++", False)
            if shutil.which("cl"):
                return ("cl", True)  # MSVC
        return ("clang++", False)
    c = compiler_arg.lower()
    if c in ("clang", "clang++"):
        return ("clang++", False)
    if c in ("gcc", "g++", "gcc++"):
        return ("g++", False)
    if c in ("msvc", "cl", "msbuild"):
        if not shutil.which("cl"):
            cprint("Error: MSVC compiler 'cl' not found in PATH.", Color.RED, bold=True)
            cprint("", Color.WHITE)
            cprint("You must run this command from a Visual Studio Developer Command Prompt,", Color.YELLOW)
            cprint("or use the 'Developer PowerShell for VS' terminal.", Color.YELLOW)
            cprint("", Color.WHITE)
            cprint("Alternative: install clang and use --compiler clang", Color.WHITE)
            cprint("  python ltgui.py build --compiler clang", Color.CYAN)
            sys.exit(1)
        return ("cl", True)
    if not shutil.which(compiler_arg):
        cprint(f"Error: compiler '{compiler_arg}' not found in PATH.", Color.RED, bold=True)
        cprint("Make sure the compiler is installed and available on your PATH.", Color.YELLOW)
        sys.exit(1)
    return (compiler_arg, False)  # custom path

# --- Flag parsing ---
def parse_flags(args):
    """Parse --key value and -k value flags from args.
    Returns (positional_args, flags_dict).
    Boolean flags (no value) get the value True.
    """
    positional = []
    flags = {}
    i = 0
    while i < len(args):
        if args[i].startswith("--"):
            key = args[i][2:]
            if i + 1 < len(args) and not args[i + 1].startswith("-"):
                flags[key] = args[i + 1]
                i += 2
            else:
                flags[key] = True
                i += 1
        elif args[i].startswith("-") and len(args[i]) > 2 and args[i][1].isalpha() and args[i][2:].isdigit():
            # Combined short flag: -j4, -O2 (letter + digits)
            key = args[i][1]
            flags[key] = args[i][2:]
            i += 1
        elif args[i].startswith("-") and len(args[i]) == 2 and args[i][1].isalpha():
            # Short flag: -j 4, -O2 (single letter, value in next arg)
            key = args[i][1]
            if i + 1 < len(args) and not args[i + 1].startswith("-"):
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
def get_platform_libs(platform, is_msvc=False):
    if platform == "windows":
        if is_msvc:
            return ["gdi32.lib", "gdiplus.lib", "user32.lib", "comctl32.lib",
                    "ole32.lib", "imm32.lib", "d3d11.lib", "dxgi.lib", "d3dcompiler.lib"]
        return ["-lgdi32", "-lgdiplus", "-luser32", "-lcomctl32", "-lole32", "-limm32",
                "-ld3d11", "-ldxgi", "-ld3dcompiler"]
    elif platform == "linux":
        return ["-lX11", "-lXft", "-lXrender", "-lfontconfig", "-lEGL", "-lGLESv2"]
    elif platform == "macos":
        return ["-framework", "Cocoa", "-framework", "CoreGraphics", "-framework", "CoreText"]
    return []

def get_platform_flags(platform, is_msvc=False):
    if platform == "windows":
        if is_msvc:
            # /utf-8 fixes C4819 warnings on Chinese locale systems
            # LTGUI_PLATFORM_WINDOWS is also defined in platform.h; dup is harmless
            return ["/D_UNICODE", "/DUNICODE", "/DLTGUI_PLATFORM_WINDOWS", "/utf-8"]
        return ["-D_UNICODE", "-DUNICODE", "-DLTGUI_PLATFORM_WINDOWS"]
    elif platform == "linux":
        return ["-I/usr/include/freetype2"]
    return []

def get_source_files(is_msvc=False):
    sources = []
    for root, dirs, files in os.walk(SRC_DIR):
        for f in files:
            if f.endswith(".cpp"):
                sources.append(os.path.join(root, f))
            elif f.endswith(".mm") and not is_msvc:
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

def _record_compile_command(src_path, flags):
    """Accumulate a compile command for compile_commands.json generation."""
    import json as _json
    _COMPILE_COMMANDS.append({
        "directory": SCRIPT_DIR,
        "command": " ".join(flags),
        "file": src_path,
    })

def _write_compile_commands():
    """Write accumulated compile commands to build/compile_commands.json."""
    if not _COMPILE_COMMANDS:
        return
    path = os.path.join(BUILD_DIR, "compile_commands.json")
    with open(path, "w") as f:
        json.dump(_COMPILE_COMMANDS, f, indent=2)
    cprint(f"  Generated {path}", Color.GREEN)

def _get_compile_flags(src_path, platform, is_release, compiler, is_msvc, pic=False):
    """Build the compiler flags list for a single source file.
    Returns (flags_list, obj_path, rel_path).
    """
    rel = os.path.relpath(src_path, SRC_DIR)
    ext = ".obj" if is_msvc else ".o"
    obj_path = os.path.join(OBJ_DIR, rel + ext)

    if is_msvc:
        flags = [compiler, "/nologo", "/c", "/std:c++17", "/EHsc"]
        if is_release:
            flags += ["/O2", "/DNDEBUG"]
        else:
            flags += ["/Z7", "/Od"]  # /Z7 embeds debug info in .obj (parallel-safe)
        flags += ["/W3"]
        flags += ["/I", INCLUDE_DIR, "/I", VENDOR_DIR]
        flags += get_platform_flags(platform, is_msvc=True)
        flags += [src_path, f"/Fo{obj_path}"]
    else:
        flags = [compiler, "-c", "-std=c++17", "-fexec-charset=UTF-8"]
        if pic:
            flags.append("-fPIC")
        if is_release:
            flags += ["-O2", "-DNDEBUG"]
        else:
            flags += ["-g", "-O0"]
        # Profile build: add gprof instrumentation
        if _PROFILE_BUILD and not is_release:
            if platform != "macos":
                flags += ["-pg", "-fno-omit-frame-pointer"]
        flags += ["-Wall", "-Wextra", "-Wpedantic"]
        flags += ["-Wno-unused-parameter", "-Wno-missing-field-initializers"]
        flags += ["-I", INCLUDE_DIR, "-I", VENDOR_DIR]
        flags += get_platform_flags(platform)
        flags += [src_path, "-o", obj_path]
    return flags, obj_path, rel

def compile_source(src_path, platform, is_release, compiler, is_msvc, pic=False):
    flags, obj_path, rel = _get_compile_flags(src_path, platform, is_release, compiler, is_msvc, pic)
    os.makedirs(os.path.dirname(obj_path), exist_ok=True)

    if _object_is_fresh(obj_path, src_path):
        _record_compile_command(src_path, flags)
        return obj_path

    cprint(f"  Compiling {rel}...", Color.CYAN)
    if _VERBOSE:
        cprint(f"    {' '.join(flags)}", Color.YELLOW)
        result = subprocess.run(flags)
    else:
        result = subprocess.run(flags, capture_output=True, text=True)
    if result.returncode != 0:
        cprint(f"  Error compiling {rel}:", Color.RED, bold=True)
        if not _VERBOSE:
            cprint(result.stderr, Color.RED)
        return None

    _record_compile_command(src_path, flags)
    return obj_path

def _compile_sources_parallel(sources, platform, is_release, compiler, is_msvc, pic=False):
    """Compile multiple sources in parallel using a thread pool."""
    from concurrent.futures import ThreadPoolExecutor, as_completed
    object_files = []
    errors = []

    with ThreadPoolExecutor(max_workers=_JOBS) as executor:
        futures = {
            executor.submit(compile_source, src, platform, is_release, compiler, is_msvc, pic): src
            for src in sources
        }
        for future in as_completed(futures):
            src = futures[future]
            try:
                obj = future.result()
                if obj:
                    object_files.append(obj)
                else:
                    errors.append(os.path.relpath(src, SRC_DIR))
            except Exception as e:
                rel = os.path.relpath(src, SRC_DIR)
                cprint(f"  Compilation exception for {rel}: {e}", Color.RED)
                errors.append(rel)

    if errors:
        cprint(f"  {len(errors)} file(s) failed to compile.", Color.RED)
        return None
    return object_files

def build_shared_lib(platform, is_release, compiler):
    """Build a dynamic/shared library (.dll / .so / .dylib)."""
    sources = get_source_files()
    if not sources:
        cprint("No source files found in src/", Color.RED)
        return None

    build_type = "release" if is_release else "debug"
    cprint(f"Building ltgui shared library ({build_type}) for {platform} with {compiler}...", Color.BLUE, bold=True)
    cprint(f"Found {len(sources)} source files.", Color.WHITE)

    object_files = _compile_sources_parallel(sources, platform, is_release, compiler, False, pic=True)
    if not object_files:
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

def build_lib(platform, is_release, compiler, is_msvc):
    sources = get_source_files(is_msvc)
    if not sources:
        cprint("No source files found in src/", Color.RED)
        return None

    build_type = "release" if is_release else "debug"
    cprint(f"Building ltgui ({build_type}) for {platform} with {compiler}...", Color.BLUE, bold=True)
    cprint(f"Found {len(sources)} source files.", Color.WHITE)

    object_files = _compile_sources_parallel(sources, platform, is_release, compiler, is_msvc)
    if not object_files:
        return None

    os.makedirs(LIB_DIR, exist_ok=True)
    lib_name = "ltgui.lib" if platform == "windows" else "libltgui.a"
    lib_path = os.path.join(LIB_DIR, lib_name)

    cprint(f"  Creating {lib_name}...", Color.CYAN)

    if is_msvc:
        result = subprocess.run(
            ["link", "/LIB", "/nologo", f"/OUT:{lib_path}"] + object_files,
            capture_output=True, text=True)
    elif platform == "windows" and "clang" in compiler:
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

def build_example(name, platform, is_release, lib_path, compiler, is_msvc):
    src = os.path.join(EXAMPLES_DIR, name + ".cpp")
    if not os.path.exists(src):
        cprint(f"Example '{name}' not found at {src}", Color.RED)
        return False

    exe_path = os.path.join(BUILD_DIR, name + (".exe" if platform == "windows" else ""))

    if is_msvc:
        flags = [compiler, "/nologo", "/std:c++17", "/EHsc"]
        if is_release:
            flags += ["/O2", "/DNDEBUG"]
        else:
            flags += ["/Zi", "/Od"]
        flags += ["/I", INCLUDE_DIR, "/I", VENDOR_DIR]
        flags += get_platform_flags(platform, is_msvc=True)
        flags += [f"/Fe:{exe_path}"]
        flags += [src]
        flags += ["/link", lib_path]
        flags += get_platform_libs(platform, is_msvc=True)
        flags += ["/SUBSYSTEM:CONSOLE"]
    else:
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
        cprint(result.stderr or result.stdout or "(no output)", Color.RED)
        return False

    cprint(f"  Created {exe_path}", Color.GREEN)
    return True

def build_app(name, platform, is_release, lib_path, compiler, is_msvc):
    src = os.path.join(APP_DIR, name + ".cpp")
    if not os.path.exists(src):
        cprint(f"App '{name}' not found at {src}", Color.RED)
        return False

    exe_path = os.path.join(BUILD_DIR, name + (".exe" if platform == "windows" else ""))

    if is_msvc:
        flags = [compiler, "/nologo", "/std:c++17", "/EHsc"]
        if is_release:
            flags += ["/O2", "/DNDEBUG"]
        else:
            flags += ["/Zi", "/Od"]
        flags += ["/I", INCLUDE_DIR, "/I", VENDOR_DIR]
        flags += get_platform_flags(platform, is_msvc=True)
        flags += [f"/Fe:{exe_path}"]
        flags += [src]
        flags += ["/link", lib_path]
        flags += get_platform_libs(platform, is_msvc=True)
        flags += ["/SUBSYSTEM:CONSOLE"]
    else:
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
        cprint(result.stderr or result.stdout or "(no output)", Color.RED)
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

    KNOWN LIMITATIONS:
    - String literals containing '{' or '}' may still confuse the parser
      in edge cases (multi-line strings, raw string literals with R\"(...)\").
    - Template arguments with '>' are handled for simple cases but complex
      nested templates may still cause false positives.
    - Lambda expressions inside function signatures are not fully supported.
    - #ifdef-wrapped function definitions may produce invalid syntax output
      if the #ifdef splits a function signature across branches.
    - For production use, consider replacing this with a libclang-based
      approach or using a pre-built SDK header instead.
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
        # Skip string and char literals to avoid false brace matches inside them
        if pos < n and text[pos] in ('"', "'"):
            quote = text[pos]
            # Check for raw string literal prefix: R"delim(...)delim"
            if pos > 0 and text[pos-1] == 'R':
                # Raw string: R"delim( ... )delim"
                delim_start = pos + 1
                paren = text.find('(', delim_start)
                if paren >= 0:
                    delim = text[delim_start:paren]
                    end_marker = ')' + delim + '"'
                    end_pos = text.find(end_marker, paren + 1)
                    if end_pos >= 0:
                        out.append(text[pos:end_pos + len(end_marker)])
                        pos = end_pos + len(end_marker)
                        continue
            # Regular string/char literal
            out.append(quote)
            i = pos + 1
            while i < n:
                if text[i] == '\\':
                    out.append(text[i:i+2])
                    i += 2
                    continue
                if text[i] == quote:
                    out.append(quote)
                    pos = i + 1
                    break
                out.append(text[i])
                i += 1
            else:
                pos = i  # unterminated string, move past it
            continue

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

    compiler, is_msvc = resolve_compiler(flags.get("compiler"))
    single_example = flags.get("example")
    single_app = flags.get("app")

    check_platform_clean(compiler)
    platform = detect_platform()

    # SDK export (--dll) — build shared lib only, skip examples/apps
    dll_dir = flags.get("dll")
    if dll_dir and isinstance(dll_dir, str):
        lib_path = build_shared_lib(platform, is_release, compiler)
        if not lib_path:
            return
        export_sdk(lib_path, dll_dir, platform)
        _write_compile_commands()
        cprint("\nBuild complete.", Color.GREEN, bold=True)
        return

    lib_path = build_lib(platform, is_release, compiler, is_msvc)
    if not lib_path:
        return

    if single_example:
        build_example(single_example, platform, is_release, lib_path, compiler, is_msvc)
    elif single_app:
        build_app(single_app, platform, is_release, lib_path, compiler, is_msvc)
    else:
        build_apps(platform, is_release, lib_path, compiler, is_msvc)
        build_examples(platform, is_release, lib_path, compiler, is_msvc)

    _write_compile_commands()
    cprint("\nBuild complete.", Color.GREEN, bold=True)

def build_apps(platform, is_release, lib_path, compiler, is_msvc):
    if not os.path.exists(APP_DIR):
        return True

    apps = sorted([f[:-4] for f in os.listdir(APP_DIR) if f.endswith(".cpp")])
    if not apps:
        return True

    cprint("Building apps...", Color.BLUE, bold=True)
    for app in apps:
        if not build_app(app, platform, is_release, lib_path, compiler, is_msvc):
            return False
    return True

def build_examples(platform, is_release, lib_path, compiler, is_msvc):
    if not os.path.exists(EXAMPLES_DIR):
        return True

    examples = sorted([f[:-4] for f in os.listdir(EXAMPLES_DIR) if f.endswith(".cpp")])
    if not examples:
        return True

    cprint("Building examples...", Color.BLUE, bold=True)
    for ex in examples:
        if not build_example(ex, platform, is_release, lib_path, compiler, is_msvc):
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
    compiler, is_msvc = resolve_compiler(flags.get("compiler"))

    # Determine if it's an app or example
    app_src = os.path.join(APP_DIR, name + ".cpp")
    ex_src = os.path.join(EXAMPLES_DIR, name + ".cpp")
    is_app = os.path.exists(app_src)

    if not is_app and not os.path.exists(ex_src):
        cprint(f"Program '{name}' not found in app/ or examples/", Color.RED)
        return

    check_platform_clean(compiler)
    platform = detect_platform()

    lib_path = build_lib(platform, is_release=False, compiler=compiler, is_msvc=is_msvc)
    if not lib_path:
        return

    if is_app:
        if not build_app(name, platform, False, lib_path, compiler, is_msvc):
            return
    else:
        if not build_example(name, platform, False, lib_path, compiler, is_msvc):
            return

    _write_compile_commands()

    exe_name = name + (".exe" if platform == "windows" else "")
    exe_path = os.path.join(BUILD_DIR, exe_name)

    cprint(f"\nRunning {exe_name}...\n", Color.MAGENTA, bold=True)
    subprocess.run([exe_path])

def cmd_test(positional, flags):
    """Build and run all tests in test/."""
    compiler, is_msvc = resolve_compiler(flags.get("compiler"))

    check_platform_clean(compiler)
    platform = detect_platform()

    test_files = sorted([f for f in os.listdir(TEST_DIR) if f.endswith(".cpp")])
    if not test_files:
        cprint("No test files found in test/", Color.YELLOW)
        return

    lib_path = build_lib(platform, is_release=False, compiler=compiler, is_msvc=is_msvc)
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

        if is_msvc:
            flags_list = [compiler, "/nologo", "/std:c++17", "/EHsc",
                          "/Zi", "/Od"]
            flags_list += ["/I", INCLUDE_DIR, "/I", os.path.join(SCRIPT_DIR, "vendor")]
            flags_list += get_platform_flags(platform, is_msvc=True)
            flags_list += [f"/Fe:{exe_path}"]
            flags_list += [src]
            flags_list += ["/link", lib_path]
            flags_list += get_platform_libs(platform, is_msvc=True)
            flags_list += ["/SUBSYSTEM:CONSOLE"]
        else:
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

# --- cmd_info ---
def cmd_info(positional, flags):
    """Display project structure statistics."""
    platform = detect_platform()

    src_files = get_source_files()
    header_files = []
    if os.path.exists(INCLUDE_DIR):
        for root, dirs, files in os.walk(INCLUDE_DIR):
            for f in files:
                if f.endswith(".h"):
                    header_files.append(os.path.join(root, f))

    examples = sorted([f[:-4] for f in os.listdir(EXAMPLES_DIR) if f.endswith(".cpp")]) if os.path.exists(EXAMPLES_DIR) else []
    apps = sorted([f[:-4] for f in os.listdir(APP_DIR) if f.endswith(".cpp")]) if os.path.exists(APP_DIR) else []
    tests = sorted([f[:-4] for f in os.listdir(TEST_DIR) if f.endswith(".cpp")]) if os.path.exists(TEST_DIR) else []

    compiler, is_msvc = resolve_compiler(flags.get("compiler"))

    cprint("ltgui Project Info", Color.BLUE, bold=True)
    print(f"  Platform:      {platform} ({sys.platform})")
    print(f"  Compiler:      {compiler}")
    print(f"  Python:        {sys.version.split()[0]}")
    print(f"  CPU cores:     {os.cpu_count() or 'unknown'}")
    print()
    cprint("File counts:", Color.CYAN)
    print(f"  Sources:       {len(src_files)}")
    print(f"  Headers:       {len(header_files)}")
    print(f"  Examples:      {len(examples)}")
    print(f"  Apps:          {len(apps)}")
    print(f"  Tests:         {len(tests)}")

    if src_files:
        dirs = {}
        for f in src_files:
            d = os.path.relpath(os.path.dirname(f), SRC_DIR)
            dirs[d] = dirs.get(d, 0) + 1
        print()
        cprint("Source structure:", Color.CYAN)
        for d, count in sorted(dirs.items()):
            label = d if d != "." else "(root)"
            print(f"  {label}/: {count} files")

    if examples:
        print()
        cprint("Examples:", Color.CYAN)
        for e in examples:
            print(f"  {e}")
    if apps:
        print()
        cprint("Apps:", Color.CYAN)
        for a in apps:
            print(f"  {a}")

# --- cmd_fmt ---
def _find_clang_format():
    """Locate clang-format or return None."""
    exe = shutil.which("clang-format")
    if exe:
        return exe
    for candidate in ["clang-format-18", "clang-format-17", "clang-format-16", "clang-format-15"]:
        exe = shutil.which(candidate)
        if exe:
            return exe
    return None

def _get_project_source_files():
    """Collect all .cpp, .h, .mm files for formatting/linting."""
    patterns = [
        (SRC_DIR, [".cpp", ".mm"]),
        (INCLUDE_DIR, [".h"]),
        (EXAMPLES_DIR, [".cpp"]),
        (APP_DIR, [".cpp"]),
        (TEST_DIR, [".cpp"]),
    ]
    files = []
    for base_dir, extensions in patterns:
        if not os.path.exists(base_dir):
            continue
        for root, dirs, filenames in os.walk(base_dir):
            dirs[:] = [d for d in dirs if d not in (".git", "__pycache__")]
            for f in filenames:
                if any(f.endswith(ext) for ext in extensions):
                    files.append(os.path.join(root, f))
    return sorted(files)

def cmd_fmt(positional, flags):
    """Run clang-format on all source files."""
    cf = _find_clang_format()
    if not cf:
        cprint("Error: clang-format not found. Install clang-format and add it to PATH.", Color.RED)
        return

    config_path = os.path.join(SCRIPT_DIR, ".clang-format")
    if not os.path.exists(config_path):
        cprint("Warning: No .clang-format found in project root.", Color.YELLOW)
        cprint("  Using LLVM style as default. Create a .clang-format to customize.", Color.YELLOW)

    files = _get_project_source_files()
    cprint(f"Formatting {len(files)} files...", Color.BLUE, bold=True)

    cmd = [cf, "-i"]
    if os.path.exists(config_path):
        cmd += ["--style=file"]
    else:
        cmd += ["--style=LLVM"]

    success = 0
    failed = 0
    for f in files:
        rel = os.path.relpath(f, SCRIPT_DIR)
        cprint(f"  {rel}...", Color.CYAN, end="")
        result = subprocess.run(cmd + [f], capture_output=True, text=True)
        if result.returncode == 0:
            cprint(" OK", Color.GREEN)
            success += 1
        else:
            cprint(" ERROR", Color.RED)
            if not _VERBOSE:
                cprint(f"    {result.stderr.strip()}", Color.RED)
            failed += 1

    if failed:
        cprint(f"{success}/{success + failed} files formatted, {failed} failed", Color.RED)
    else:
        cprint(f"{success} files formatted.", Color.GREEN, bold=True)

# --- cmd_lint ---
def cmd_lint(positional, flags):
    """Run clang-tidy on all source files."""
    ct = shutil.which("clang-tidy")
    if not ct:
        cprint("Error: clang-tidy not found. Install clang-tidy and add it to PATH.", Color.RED)
        return

    config_path = os.path.join(SCRIPT_DIR, ".clang-tidy")
    if not os.path.exists(config_path):
        cprint("Warning: No .clang-tidy found in project root.", Color.YELLOW)
        cprint("  Using built-in default checks. Create a .clang-tidy to customize.", Color.YELLOW)

    files = sorted([
        os.path.join(root, f)
        for root, dirs, files in os.walk(SRC_DIR)
        for f in files if f.endswith(".cpp")
    ])
    if not files:
        cprint("No source files found in src/", Color.RED)
        return

    cprint(f"Running clang-tidy on {len(files)} files...", Color.BLUE, bold=True)

    compile_db = os.path.join(BUILD_DIR, "compile_commands.json")
    issues = 0
    for f in files:
        rel = os.path.relpath(f, SCRIPT_DIR)
        cprint(f"  {rel}...", Color.CYAN, end="")

        cmd = [ct, f]
        if os.path.exists(compile_db):
            cmd += [f"-p={BUILD_DIR}"]
        else:
            cmd += ["--", f"-std=c++17", f"-I{INCLUDE_DIR}", f"-I{VENDOR_DIR}"]

        result = subprocess.run(cmd, capture_output=True, text=True)
        output = (result.stdout + result.stderr).strip()

        w_count = output.count("warning:")
        e_count = output.count("error:")
        if w_count or e_count:
            issues += w_count + e_count
            cprint(f" {w_count} warnings, {e_count} errors", Color.YELLOW)
            if _VERBOSE:
                print(output)
        else:
            cprint(" PASS", Color.GREEN)

    if issues:
        cprint(f"Found {issues} lint issue(s).", Color.YELLOW)
    else:
        cprint("No lint issues detected.", Color.GREEN, bold=True)

# --- cmd_new ---
def _to_snake_case(name):
    """Convert PascalCase to snake_case.  'MyButton' -> 'my_button'"""
    result = []
    for i, c in enumerate(name):
        if c.isupper() and i > 0:
            result.append('_')
            result.append(c.lower())
        else:
            result.append(c.lower())
    return ''.join(result)

def _to_pascal_case(name):
    """Convert 'my_button' or 'MyButton' to 'MyButton'."""
    return ''.join(w[0].upper() + w[1:] if w else '' for w in name.replace('-', '_').split('_'))

def _scaffold_widget(name):
    snake = _to_snake_case(name)
    header_path = os.path.join(INCLUDE_DIR, "widgets", snake + ".h")
    src_path = os.path.join(SRC_DIR, "widgets", snake + ".cpp")

    if os.path.exists(header_path) or os.path.exists(src_path):
        cprint(f"Widget '{name}' already exists.", Color.RED)
        return

    header_content = f"""#pragma once
#include "ltgui/widget.h"

namespace ltgui {{

class {name} : public Widget {{
public:
    explicit {name}(Widget* parent = nullptr);

    void paintSelf(NativeCanvas* canvas) override;
    Size sizeHint() const override;
    WidgetType widgetType() const override {{ return WidgetType::{name}; }}

private:
    // TODO: add private members
}};

}} // namespace ltgui
"""
    src_content = f"""#include "widgets/{snake}.h"

namespace ltgui {{

{name}::{name}(Widget* parent)
    : Widget(parent)
{{
    // TODO: initialize default style and children
}}

void {name}::paintSelf(NativeCanvas* canvas)
{{
    // TODO: implement custom painting
    Widget::paintSelf(canvas);
}}

Size {name}::sizeHint() const
{{
    // TODO: return preferred size
    return Size{{100, 30}};
}}

}} // namespace ltgui
"""
    with open(header_path, "w") as f:
        f.write(header_content)
    with open(src_path, "w") as f:
        f.write(src_content)

    cprint(f"Created widget '{name}':", Color.GREEN, bold=True)
    cprint(f"  {header_path}", Color.CYAN)
    cprint(f"  {src_path}", Color.CYAN)
    cprint("", Color.WHITE)
    cprint("Don't forget to include in:", Color.YELLOW)
    cprint("  1. The ltgui.h umbrella header", Color.YELLOW)
    cprint(f"  2. Add '{snake}.h' to _HEADER_ORDER in ltgui.py", Color.YELLOW)

def _scaffold_example(name):
    snake = _to_snake_case(name)
    ex_path = os.path.join(EXAMPLES_DIR, snake + ".cpp")
    if os.path.exists(ex_path):
        cprint(f"Example '{name}' already exists at {ex_path}", Color.RED)
        return

    content = f"""#include "ltgui.h"
#include <iostream>

using namespace ltgui;

int main() {{
    Window window;
    if (!window.create(400, 300, "{name}")) {{
        std::cerr << "Failed to create window." << std::endl;
        return 1;
    }}

    auto root = std::make_unique<Widget>();
    root->setStyle(Style::defaultStyle());

    // TODO: add widgets and layout

    window.setCentralWidget(std::move(root));
    window.show();

    return Application::instance().run();
}}
"""
    with open(ex_path, "w") as f:
        f.write(content)
    cprint(f"Created example '{name}' at {ex_path}", Color.GREEN, bold=True)

def _scaffold_app(name):
    snake = _to_snake_case(name)
    app_path = os.path.join(APP_DIR, snake + ".cpp")
    if os.path.exists(app_path):
        cprint(f"App '{name}' already exists at {app_path}", Color.RED)
        return

    content = f"""#include "ltgui.h"
#include <iostream>

using namespace ltgui;

int main() {{
    Window window;
    if (!window.create(800, 600, "{name}")) {{
        std::cerr << "Failed to create window." << std::endl;
        return 1;
    }}

    auto root = std::make_unique<Widget>();
    root->style().bgColor = currentTheme().bgPrimary;

    // TODO: build app UI

    window.setCentralWidget(std::move(root));
    window.show();

    return Application::instance().run();
}}
"""
    with open(app_path, "w") as f:
        f.write(content)
    cprint(f"Created app '{name}' at {app_path}", Color.GREEN, bold=True)

def cmd_new(positional, flags):
    if len(positional) < 3:
        cprint("Usage: python ltgui.py new widget|example|app <name>", Color.YELLOW)
        cprint("  python ltgui.py new widget MyButton", Color.CYAN)
        cprint("  python ltgui.py new example particles", Color.CYAN)
        cprint("  python ltgui.py new app myapp", Color.CYAN)
        return

    kind = positional[1].lower()
    raw_name = positional[2]
    name = _to_pascal_case(raw_name)

    if kind == "widget":
        _scaffold_widget(name)
    elif kind == "example":
        _scaffold_example(name)
    elif kind == "app":
        _scaffold_app(name)
    else:
        cprint(f"Unknown type: '{kind}'. Use 'widget', 'example', or 'app'.", Color.RED)

# --- cmd_install ---
def cmd_install(positional, flags):
    """Install library + headers to system directories."""
    platform = detect_platform()
    compiler, is_msvc = resolve_compiler(flags.get("compiler"))

    if platform == "windows":
        prefix = flags.get("prefix", os.path.join(os.environ.get("ProgramFiles", "C:\\Program Files"), "ltgui"))
        lib_dir = os.path.join(prefix, "lib")
        include_dir = os.path.join(prefix, "include")
    else:
        prefix = flags.get("prefix", "/usr/local")
        lib_dir = os.path.join(prefix, "lib")
        include_dir = os.path.join(prefix, "include")

    lib_name = "ltgui.lib" if platform == "windows" else "libltgui.a"
    lib_path = os.path.join(LIB_DIR, lib_name)
    if not os.path.exists(lib_path):
        cprint(f"Library not found at {lib_path}. Build first.", Color.RED)
        cprint("  Run: python ltgui.py build", Color.YELLOW)
        return

    cprint(f"Installing to {prefix}...", Color.BLUE, bold=True)

    try:
        os.makedirs(lib_dir, exist_ok=True)
        os.makedirs(include_dir, exist_ok=True)

        shutil.copy2(lib_path, os.path.join(lib_dir, lib_name))
        cprint(f"  Library: {lib_name}", Color.GREEN)

        header_count = 0
        for root, dirs, files in os.walk(INCLUDE_DIR):
            for f in files:
                if f.endswith(".h"):
                    rel = os.path.relpath(root, INCLUDE_DIR)
                    target_dir = os.path.join(include_dir, "ltgui", rel) if rel != "." else os.path.join(include_dir, "ltgui")
                    os.makedirs(target_dir, exist_ok=True)
                    shutil.copy2(os.path.join(root, f), os.path.join(target_dir, f))
                    header_count += 1

        cprint(f"  Headers: {header_count} copied", Color.GREEN)
        cprint("Install complete.", Color.GREEN, bold=True)
    except PermissionError:
        cprint(f"Permission denied writing to {prefix}. Try running with elevated privileges.", Color.RED)

# --- cmd_package ---
def cmd_package(positional, flags):
    """Package the SDK into a distributable archive."""
    fmt = flags.get("format")
    platform = detect_platform()
    if fmt is None:
        fmt = "zip" if platform == "windows" else "tar.gz"

    if fmt not in ("zip", "tar.gz"):
        cprint(f"Unsupported format: '{fmt}'. Use 'zip' or 'tar.gz'.", Color.RED)
        return

    lib_name = "ltgui.lib" if platform == "windows" else "libltgui.a"
    lib_path = os.path.join(LIB_DIR, lib_name)
    if not os.path.exists(lib_path):
        cprint(f"Library not found at {lib_path}. Build first.", Color.RED)
        cprint("  Run: python ltgui.py build", Color.YELLOW)
        return

    # Auto-generate amalgamated header if not already done
    amalgamated = os.path.join(BUILD_DIR, "ltgui.h")
    if not os.path.exists(amalgamated):
        generate_amalgamated_header(amalgamated)

    # Create temp package directory
    sdk_dir = os.path.join(BUILD_DIR, "sdk-pkg")
    if os.path.exists(sdk_dir):
        shutil.rmtree(sdk_dir)
    os.makedirs(sdk_dir)

    shutil.copy2(lib_path, sdk_dir)
    cprint(f"  Packaged: {lib_name}", Color.CYAN)
    if os.path.exists(amalgamated):
        shutil.copy2(amalgamated, sdk_dir)
        cprint(f"  Packaged: ltgui.h", Color.CYAN)

    # Version from git
    version = subprocess.run(
        ["git", "describe", "--tags", "--always"],
        capture_output=True, text=True, cwd=SCRIPT_DIR
    ).stdout.strip()
    if not version:
        import datetime
        version = datetime.date.today().strftime("%Y%m%d")

    archive_basename = f"ltgui-{version}-sdk"

    if fmt == "zip":
        archive_path = shutil.make_archive(
            os.path.join(BUILD_DIR, archive_basename), "zip", sdk_dir)
    else:
        archive_path = shutil.make_archive(
            os.path.join(BUILD_DIR, archive_basename), "gztar", sdk_dir)

    shutil.rmtree(sdk_dir)

    cprint(f"Created {archive_path}", Color.GREEN, bold=True)

# --- cmd_watch ---
def _get_watched_files():
    """Collect all .cpp, .mm, .h files for watching."""
    files = []
    for base in [SRC_DIR, INCLUDE_DIR, EXAMPLES_DIR, APP_DIR, TEST_DIR]:
        if not os.path.exists(base):
            continue
        for root, dirs, filenames in os.walk(base):
            dirs[:] = [d for d in dirs if d not in (".git", "__pycache__")]
            for f in filenames:
                if f.endswith((".cpp", ".h", ".mm")):
                    files.append(os.path.join(root, f))
    return sorted(files)

def cmd_watch(positional, flags):
    """Watch files and auto-rebuild on changes."""
    import time as _time

    target = positional[1] if len(positional) > 1 else None
    compiler, is_msvc = resolve_compiler(flags.get("compiler"))
    check_platform_clean(compiler)
    platform = detect_platform()
    is_release = len(positional) >= 2 and positional[1] == "release"

    def do_build():
        lib_path = build_lib(platform, is_release, compiler, is_msvc)
        if not lib_path:
            return False
        if target and target != "release":
            app_src = os.path.join(APP_DIR, target + ".cpp")
            ex_src = os.path.join(EXAMPLES_DIR, target + ".cpp")
            if os.path.exists(app_src):
                return build_app(target, platform, is_release, lib_path, compiler, is_msvc)
            elif os.path.exists(ex_src):
                return build_example(target, platform, is_release, lib_path, compiler, is_msvc)
            else:
                cprint(f"Target '{target}' not found.", Color.RED)
                return False
        else:
            ok = True
            ok = build_apps(platform, is_release, lib_path, compiler, is_msvc) and ok
            ok = build_examples(platform, is_release, lib_path, compiler, is_msvc) and ok
            return ok

    cprint("Watch mode: performing initial build...", Color.BLUE, bold=True)
    do_build()
    _write_compile_commands()

    watched = _get_watched_files()
    mtimes = {}
    for f in watched:
        try:
            mtimes[f] = os.path.getmtime(f)
        except OSError:
            pass

    cprint(f"\nWatching ({len(watched)} files)... Press Ctrl+C to stop.", Color.MAGENTA, bold=True)

    try:
        while True:
            _time.sleep(1.0)
            changed = []
            for f in watched:
                try:
                    current = os.path.getmtime(f)
                    if current != mtimes.get(f):
                        changed.append(f)
                        mtimes[f] = current
                except OSError:
                    pass

            # Check for new files
            current_watched = _get_watched_files()
            new_files = [f for f in current_watched if f not in mtimes]
            if new_files:
                changed.extend(new_files)
                for f in new_files:
                    try:
                        mtimes[f] = os.path.getmtime(f)
                    except OSError:
                        pass
                watched = current_watched

            if changed:
                cprint(f"\nDetected {len(changed)} file change(s):", Color.YELLOW, bold=True)
                for f in changed[:10]:
                    cprint(f"  {os.path.relpath(f, SCRIPT_DIR)}", Color.CYAN)
                if len(changed) > 10:
                    cprint(f"  ... and {len(changed) - 10} more", Color.CYAN)

                cprint("Rebuilding...", Color.BLUE, bold=True)
                do_build()
                _write_compile_commands()
                cprint(f"\nWatching ({len(watched)} files)... Press Ctrl+C to stop.", Color.MAGENTA, bold=True)
    except KeyboardInterrupt:
        cprint("\n\nWatch mode stopped.", Color.YELLOW)

# --- cmd_debug ---
def _resolve_debugger(platform):
    """Find the best available debugger for a platform."""
    if platform == "windows":
        for exe in ["cdb", "lldb", "gdb"]:
            path = shutil.which(exe)
            if path:
                return (path, ["-c", "g"] if exe == "gdb" else [])
        return (None, [])
    elif platform == "macos":
        path = shutil.which("lldb")
        return (path, []) if path else (None, [])
    else:
        for exe in ["gdb", "lldb"]:
            path = shutil.which(exe)
            if path:
                return (path, ["-ex", "run"] if exe == "gdb" else [])
        return (None, [])

def cmd_debug(positional, flags):
    """Build in debug mode and launch with a debugger."""
    if len(positional) < 2:
        cprint("Usage: python ltgui.py debug <name> [--compiler ...]", Color.YELLOW)
        return

    name = positional[1]
    compiler, is_msvc = resolve_compiler(flags.get("compiler"))
    check_platform_clean(compiler)
    platform = detect_platform()

    cprint(f"Debug mode: building '{name}'...", Color.BLUE, bold=True)
    lib_path = build_lib(platform, False, compiler, is_msvc)
    if not lib_path:
        return

    app_src = os.path.join(APP_DIR, name + ".cpp")
    ex_src = os.path.join(EXAMPLES_DIR, name + ".cpp")

    if os.path.exists(app_src):
        if not build_app(name, platform, False, lib_path, compiler, is_msvc):
            return
    elif os.path.exists(ex_src):
        if not build_example(name, platform, False, lib_path, compiler, is_msvc):
            return
    else:
        cprint(f"Program '{name}' not found in app/ or examples/", Color.RED)
        return

    debugger, debug_args = _resolve_debugger(platform)
    if not debugger:
        cprint("No debugger found for this platform. Install gdb or lldb.", Color.RED)
        return

    exe_name = name + (".exe" if platform == "windows" else "")
    exe_path = os.path.join(BUILD_DIR, exe_name)

    cprint(f"Launching debugger: {debugger}", Color.MAGENTA, bold=True)
    subprocess.run([debugger] + debug_args + [exe_path])

# --- cmd_profile ---
def cmd_profile(positional, flags):
    """Build with profiling flags and run."""
    if len(positional) < 2:
        cprint("Usage: python ltgui.py profile <name> [--compiler ...]", Color.YELLOW)
        return

    name = positional[1]
    compiler, is_msvc = resolve_compiler(flags.get("compiler"))
    check_platform_clean(compiler)
    platform = detect_platform()

    if is_msvc:
        cprint("Profiling: MSVC only supports /PROFILE linker flag.", Color.YELLOW)
        cprint("  Consider using 'clang' compiler for better profiling support.", Color.YELLOW)

    cprint(f"Profile mode: building '{name}' with profiling flags...", Color.BLUE, bold=True)

    global _PROFILE_BUILD
    _PROFILE_BUILD = True
    lib_path = build_lib(platform, False, compiler, is_msvc)
    _PROFILE_BUILD = False

    if not lib_path:
        return

    app_src = os.path.join(APP_DIR, name + ".cpp")
    ex_src = os.path.join(EXAMPLES_DIR, name + ".cpp")

    if os.path.exists(app_src):
        ok = build_app(name, platform, False, lib_path, compiler, is_msvc)
    elif os.path.exists(ex_src):
        ok = build_example(name, platform, False, lib_path, compiler, is_msvc)
    else:
        cprint(f"Program '{name}' not found in app/ or examples/", Color.RED)
        return

    if not ok:
        return

    exe_name = name + (".exe" if platform == "windows" else "")
    exe_path = os.path.join(BUILD_DIR, exe_name)

    cprint(f"\nRunning {exe_name} (profiling)...\n", Color.MAGENTA, bold=True)
    subprocess.run([exe_path])

    if platform != "windows" and os.path.exists("gmon.out"):
        cprint(f"\nGenerated gmon.out. View with:", Color.GREEN)
        cprint(f"  gprof {exe_path} gmon.out", Color.CYAN)

def print_usage():
    cprint("ltgui build system", Color.BLUE, bold=True)
    print("Usage: python ltgui.py <command> [options]")
    print()
    cprint("Commands:", Color.WHITE, bold=True)
    print("  build [release]     Build static library, apps, and examples")
    print("  clean               Remove build/ directory")
    print("  run <name>          Build and run an app or example")
    print("  test                Build and run all tests")
    print("  install             Install library + headers to system directories")
    print("  package             Package the SDK into a distributable archive")
    print("  fmt                 Run clang-format on all source files")
    print("  lint                Run clang-tidy on all source files")
    print("  new <type> <name>   Scaffold a widget, example, or app")
    print("  info                Display project structure and statistics")
    print("  watch [name]        Watch files and auto-rebuild on changes")
    print("  debug <name>        Build (debug) and launch with a debugger")
    print("  profile <name>      Build with profiling flags and run")
    print()
    cprint("Build Options:", Color.WHITE, bold=True)
    print("  --compiler <val>    clang (default), msvc, gcc, or custom path")
    print("  --example <name>    Build only this example (with 'build')")
    print("  --app <name>        Build only this app (with 'build')")
    print("  --dll <dir>         Build shared library (.dll/.so/.dylib) + headers to dir")
    print("  --jobs <N> / -j N   Parallel compilation jobs (default: CPU count)")
    print("  --verbose           Show full compiler output")
    print("  --json              Machine-readable JSON output for CI")
    print()
    cprint("Install/Package Options:", Color.WHITE, bold=True)
    print("  --prefix <dir>      Install prefix (default: /usr/local or %ProgramFiles%\\ltgui)")
    print("  --format <fmt>      Package format: zip or tar.gz (default: auto-detect)")
    print()
    cprint("Examples:", Color.WHITE, bold=True)
    print("  python ltgui.py build")
    print("  python ltgui.py build --compiler msvc -j 8")
    print("  python ltgui.py build release --compiler gcc")
    print("  python ltgui.py build --compiler /usr/bin/g++-13 --example hello")
    print("  python ltgui.py build --dll ./sdk")
    print("  python ltgui.py build --verbose")
    print("  python ltgui.py run demo --compiler gcc")
    print("  python ltgui.py new widget MyButton")
    print("  python ltgui.py watch")
    print("  python ltgui.py watch demo")
    print("  python ltgui.py debug main")
    print("  python ltgui.py profile main")
    print("  python ltgui.py info")
    print("  python ltgui.py fmt")
    print("  python ltgui.py lint")
    print("  python ltgui.py install --prefix /opt/ltgui")
    print("  python ltgui.py package --format tar.gz")

def main():
    global _VERBOSE, _JSON_MODE, _JOBS, _COMPILE_COMMANDS

    args = sys.argv[1:]
    positional, flags = parse_flags(args)

    # Global flags (processed before command dispatch)
    _val = flags.pop("verbose", False)
    if isinstance(_val, str):
        _VERBOSE = _val.lower() not in ("false", "0", "no", "off")
    else:
        _VERBOSE = bool(_val)
    _JSON_MODE = flags.pop("json", False) is not False

    jobs_str = flags.pop("jobs", None) or flags.pop("j", None)
    if jobs_str:
        try:
            _JOBS = int(jobs_str)
        except ValueError:
            cprint(f"Invalid --jobs value: {jobs_str}", Color.RED)
            sys.exit(1)

    if not positional:
        print_usage()
        return

    cmd = positional[0]

    # JSON mode and watch mode are incompatible — watch output is streaming
    if cmd == "watch" and _JSON_MODE:
        _JSON_MODE = False

    start_time = time.time()

    dispatch = {
        "build":   cmd_build,
        "clean":   cmd_clean,
        "run":     cmd_run,
        "test":    cmd_test,
        "install": cmd_install,
        "package": cmd_package,
        "fmt":     cmd_fmt,
        "lint":    cmd_lint,
        "new":     cmd_new,
        "info":    cmd_info,
        "watch":   cmd_watch,
        "debug":   cmd_debug,
        "profile": cmd_profile,
    }

    handler = dispatch.get(cmd)
    if handler:
        try:
            handler(positional, flags)
            json_event("complete", command=cmd, status="success",
                       duration=round(time.time() - start_time, 3))
        except SystemExit:
            json_event("complete", command=cmd, status="error",
                       duration=round(time.time() - start_time, 3))
            raise
        except Exception as e:
            json_event("complete", command=cmd, status="error",
                       error=str(e),
                       duration=round(time.time() - start_time, 3))
            raise
    elif cmd in ("-h", "--help", "help"):
        print_usage()
    else:
        cprint(f"Unknown command: {cmd}", Color.RED)
        print_usage()

if __name__ == "__main__":
    main()
