#!/usr/bin/env python3
"""ltgui build system — python ltgui.py [command]"""

import os
import sys
import subprocess
import glob
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

def cprint(msg, color=Color.WHITE, bold=False):
    prefix = Color.BOLD if bold else ""
    print(f"{prefix}{color}{msg}{Color.RESET}")

SCRIPT_DIR = os.path.dirname(os.path.abspath(__file__))
SRC_DIR = os.path.join(SCRIPT_DIR, "src")
INCLUDE_DIR = os.path.join(SCRIPT_DIR, "include", "ltgui")
EXAMPLES_DIR = os.path.join(SCRIPT_DIR, "examples")
APP_DIR      = os.path.join(SCRIPT_DIR, "app")
BUILD_DIR = os.path.join(SCRIPT_DIR, "build")
OBJ_DIR = os.path.join(BUILD_DIR, "obj")
LIB_DIR = os.path.join(BUILD_DIR, "lib")

# --- Platform detection ---
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
    """Auto-clean if platform changed since last build"""
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

def get_platform_libs(platform):
    if platform == "windows":
        return ["-lgdi32", "-lgdiplus", "-luser32", "-lcomctl32", "-lole32", "-limm32"]
    elif platform == "linux":
        return ["-lX11", "-lXft", "-lXrender", "-lfontconfig"]
    elif platform == "macos":
        return ["-framework", "Cocoa", "-framework", "CoreGraphics", "-framework", "CoreText"]
    return []

def get_platform_flags(platform):
    if platform == "windows":
        return ["-D_UNICODE", "-DUNICODE"]
    return []

def get_source_files():
    """Recursively collect all .cpp files from src/"""
    sources = []
    for root, dirs, files in os.walk(SRC_DIR):
        for f in files:
            if f.endswith(".cpp") or f.endswith(".mm"):
                sources.append(os.path.join(root, f))
    return sorted(sources)

# --- Build ---
def compile_source(src_path, platform, is_release):
    rel = os.path.relpath(src_path, SRC_DIR)
    obj_path = os.path.join(OBJ_DIR, rel + ".o")
    os.makedirs(os.path.dirname(obj_path), exist_ok=True)

    # Check if recompilation is needed
    if os.path.exists(obj_path) and os.path.getmtime(obj_path) > os.path.getmtime(src_path):
        return obj_path

    flags = ["clang++", "-c", "-std=c++17", "-fexec-charset=UTF-8"]
    if is_release:
        flags += ["-O2", "-DNDEBUG"]
    else:
        flags += ["-g", "-O0"]

    flags += ["-I", INCLUDE_DIR]
    flags += get_platform_flags(platform)
    flags += [src_path, "-o", obj_path]

    cprint(f"  Compiling {rel}...", Color.CYAN)
    result = subprocess.run(flags, capture_output=True, text=True)
    if result.returncode != 0:
        cprint(f"  Error compiling {rel}:", Color.RED, bold=True)
        cprint(result.stderr, Color.RED)
        return None
    return obj_path

def build_lib(platform, is_release):
    """Compile all sources and create static library"""
    sources = get_source_files()
    if not sources:
        cprint("No source files found in src/", Color.RED)
        return None

    cprint(f"Building ltgui ({'release' if is_release else 'debug'}) for {platform}...", Color.BLUE, bold=True)
    cprint(f"Found {len(sources)} source files.", Color.WHITE)

    object_files = []
    for src in sources:
        obj = compile_source(src, platform, is_release)
        if obj:
            object_files.append(obj)
        else:
            return None

    # Create static library (Windows: .lib, Unix: .a)
    os.makedirs(LIB_DIR, exist_ok=True)
    lib_name = "ltgui.lib" if platform == "windows" else "libltgui.a"
    lib_path = os.path.join(LIB_DIR, lib_name)

    cprint(f"  Creating {lib_name}...", Color.CYAN)

    if platform == "windows":
        # Use llvm-lib to create static library
        result = subprocess.run(
            ["llvm-lib"] + object_files + ["/OUT:" + lib_path],
            capture_output=True, text=True)
        if result.returncode != 0:
            # Fallback: use ar
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

def build_example(name, platform, is_release, lib_path):
    """Build a single example"""
    src = os.path.join(EXAMPLES_DIR, name + ".cpp")
    if not os.path.exists(src):
        cprint(f"Example '{name}' not found at {src}", Color.RED)
        return False

    exe_path = os.path.join(BUILD_DIR, name + (".exe" if platform == "windows" else ""))

    flags = ["clang++", "-std=c++17", "-fexec-charset=UTF-8"]
    if is_release:
        flags += ["-O2", "-DNDEBUG"]
    else:
        flags += ["-g", "-O0"]

    flags += ["-I", INCLUDE_DIR]
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

def build_examples(platform, is_release, lib_path):
    """Build all examples"""
    if not os.path.exists(EXAMPLES_DIR):
        return

    examples = []
    for f in os.listdir(EXAMPLES_DIR):
        if f.endswith(".cpp"):
            examples.append(f[:-4])

    for ex in sorted(examples):
        if not build_example(ex, platform, is_release, lib_path):
            return False
    return True

def build_app(name, platform, is_release, lib_path):
    """Build a single app from app/"""
    src = os.path.join(APP_DIR, name + ".cpp")
    if not os.path.exists(src):
        cprint(f"App '{name}' not found at {src}", Color.RED)
        return False

    exe_path = os.path.join(BUILD_DIR, name + (".exe" if platform == "windows" else ""))

    flags = ["clang++", "-std=c++17", "-fexec-charset=UTF-8"]
    if is_release:
        flags += ["-O2", "-DNDEBUG"]
    else:
        flags += ["-g", "-O0"]

    flags += ["-I", INCLUDE_DIR]
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

def build_apps(platform, is_release, lib_path):
    """Build all apps in app/"""
    if not os.path.exists(APP_DIR):
        return True

    apps = []
    for f in os.listdir(APP_DIR):
        if f.endswith(".cpp"):
            apps.append(f[:-4])

    if not apps:
        return True

    cprint(f"Building apps...", Color.BLUE, bold=True)
    for app in sorted(apps):
        if not build_app(app, platform, is_release, lib_path):
            return False
    return True

def run_program(name, platform, is_release):
    """Build and run from app/ or examples/"""
    src = os.path.join(APP_DIR, name + ".cpp")
    is_app = os.path.exists(src)
    if not is_app:
        src = os.path.join(EXAMPLES_DIR, name + ".cpp")

    if not os.path.exists(src):
        cprint(f"Program '{name}' not found in app/ or examples/", Color.RED)
        return

    lib_path = build_lib(platform, is_release)
    if not lib_path:
        return

    if is_app:
        if not build_app(name, platform, is_release, lib_path):
            return
    else:
        if not build_example(name, platform, is_release, lib_path):
            return

    exe_name = name + (".exe" if platform == "windows" else "")
    exe_path = os.path.join(BUILD_DIR, exe_name)

    cprint(f"\nRunning {exe_name}...\n", Color.MAGENTA, bold=True)
    subprocess.run([exe_path])

# --- Commands ---
def cmd_build(args):
    is_release = len(args) >= 2 and args[1] == "release"
    check_platform_clean()
    platform = detect_platform()
    lib_path = build_lib(platform, is_release)
    if lib_path:
        build_apps(platform, is_release, lib_path)
        build_examples(platform, is_release, lib_path)
        cprint("\nBuild complete.", Color.GREEN, bold=True)

def cmd_clean(args):
    if os.path.exists(BUILD_DIR):
        shutil.rmtree(BUILD_DIR)
        cprint("Cleaned build directory.", Color.GREEN)
    else:
        cprint("Nothing to clean.", Color.YELLOW)

def list_cpp_files(directory):
    """List .cpp files in a directory (without extension)"""
    if not os.path.exists(directory):
        return []
    return sorted([f[:-4] for f in os.listdir(directory) if f.endswith(".cpp")])

def cmd_run(args):
    if len(args) < 2:
        cprint("Usage: python ltgui.py run <name>", Color.YELLOW)
        apps = list_cpp_files(APP_DIR)
        examples = list_cpp_files(EXAMPLES_DIR)
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

    platform = detect_platform()
    run_program(args[1], platform, is_release=False)

def print_usage():
    cprint("ltgui build system", Color.BLUE, bold=True)
    print("Usage: python ltgui.py <command>")
    print()
    print("Commands:")
    print("  build             Debug build")
    print("  build release     Release build")
    print("  clean             Remove build artifacts")
    print("  run <example>     Build and run an example")

def main():
    args = sys.argv[1:]

    if not args:
        print_usage()
        return

    cmd = args[0]
    if cmd == "build":
        cmd_build(args)
    elif cmd == "clean":
        cmd_clean(args)
    elif cmd == "run":
        cmd_run(args)
    elif cmd in ("-h", "--help", "help"):
        print_usage()
    else:
        cprint(f"Unknown command: {cmd}", Color.RED)
        print_usage()

if __name__ == "__main__":
    main()
