#!/usr/bin/env python3
"""
Precompile D3D11 HLSL shaders to C++ byte arrays.

Usage:
    python tools/compile_shaders.py [--output src/platform/gpu/d3d11_shaders.h]

This eliminates the runtime D3DCompile() call during app startup by embedding
precompiled shader bytecode directly in the executable.

Requirements:
    - Windows SDK (fxc.exe or dxc.exe in PATH, or D3DCompile via Python)
    - When fxc is not available, falls back to the runtime compilation path.

The generated header defines:
    extern const uint8_t kSolidVsBytecode[];
    extern const size_t   kSolidVsBytecodeLen;
    ... (for each shader variant: solid, rounded, ellipse, texture × VS, PS)
"""

import subprocess
import sys
import os
import tempfile
import shutil

SHADERS = {
    "solid_vs":    {"src": "solid", "entry": "VSMain", "target": "vs_4_0"},
    "solid_ps":    {"src": "solid", "entry": "PSMain", "target": "ps_4_0"},
    "rounded_vs":  {"src": "rounded", "entry": "VSMain", "target": "vs_4_0"},
    "rounded_ps":  {"src": "rounded", "entry": "PSMain", "target": "ps_4_0"},
    "ellipse_vs":  {"src": "ellipse", "entry": "VSMain", "target": "vs_4_0"},
    "ellipse_ps":  {"src": "ellipse", "entry": "PSMain", "target": "ps_4_0"},
    "texture_vs":  {"src": "texture", "entry": "VSMain", "target": "vs_4_0"},
    "texture_ps":  {"src": "texture", "entry": "PSMain", "target": "ps_4_0"},
}

HLSL_SOURCES = {
    "solid": R"""
struct VS_IN { float2 pos : POSITION; float4 col : COLOR; };
struct VS_OUT { float4 pos : SV_POSITION; float4 col : COLOR; };

VS_OUT VSMain(VS_IN input) {
    VS_OUT output;
    output.pos = float4(input.pos, 0.0, 1.0);
    output.col = input.col;
    return output;
}

float4 PSMain(VS_OUT input) : SV_TARGET {
    return input.col;
}
""",
    "rounded": R"""
struct VS_IN { float2 pos : POSITION; float2 uv : TEXCOORD; float4 col : COLOR; float4 params : TEXCOORD1; };
struct VS_OUT { float4 pos : SV_POSITION; float2 uv : TEXCOORD; float4 col : COLOR; float4 params : TEXCOORD1; };

cbuffer ScreenCB : register(b0) { float2 screenSize; };

VS_OUT VSMain(VS_IN input) {
    VS_OUT output;
    output.pos = float4(input.pos, 0.0, 1.0);
    output.uv = input.uv;
    output.col = input.col;
    output.params = input.params;
    return output;
}

float sdRoundedBox(float2 p, float2 size, float r) {
    float2 d = abs(p) - (size * 0.5) + r;
    return length(max(d, 0.0)) + min(max(d.x, d.y), 0.0) - r;
}

float4 PSMain(VS_OUT input) : SV_TARGET {
    float2 halfSize = float2(input.params.x * 0.5, input.params.y * 0.5);
    float radius = input.params.z;
    float stroke = input.params.w;
    float2 local = (input.uv - 0.5) * float2(input.params.x, input.params.y);

    if (stroke <= 0.0) {
        float d = sdRoundedBox(local, float2(input.params.x, input.params.y), radius);
        float alpha = 1.0 - saturate(d * 0.5);
        return input.col * alpha;
    } else {
        float dOuter = sdRoundedBox(local, float2(input.params.x, input.params.y), radius);
        float dInner = sdRoundedBox(local,
            float2(input.params.x - stroke * 2.0, input.params.y - stroke * 2.0),
            max(radius - stroke, 0.0));
        float alpha = saturate((1.0 - saturate(dOuter * 0.5)) - (1.0 - saturate(dInner * 0.5)));
        return float4(input.col.rgb, input.col.a * alpha);
    }
}
""",
    "ellipse": R"""
struct VS_IN { float2 pos : POSITION; float2 uv : TEXCOORD; float4 col : COLOR; float4 params : TEXCOORD1; };
struct VS_OUT { float4 pos : SV_POSITION; float2 uv : TEXCOORD; float4 col : COLOR; float4 params : TEXCOORD1; };

VS_OUT VSMain(VS_IN input) {
    VS_OUT output;
    output.pos = float4(input.pos, 0.0, 1.0);
    output.uv = input.uv;
    output.col = input.col;
    output.params = input.params;
    return output;
}

float4 PSMain(VS_OUT input) : SV_TARGET {
    float2 center = float2(input.params.x * 0.5, input.params.y * 0.5);
    float rx = input.params.x * 0.5;
    float ry = input.params.y * 0.5;
    float stroke = input.params.w;
    float2 local = (input.uv - 0.5) * float2(input.params.x, input.params.y);
    float dist = (local.x * local.x) / (rx * rx) + (local.y * local.y) / (ry * ry);

    if (stroke <= 0.0) {
        float alpha = 1.0 - saturate((dist - 1.0) * 4.0);
        return input.col * alpha;
    } else {
        float outer = dist;
        float inner = (local.x * local.x) / max(rx - stroke, 0.001f) / max(rx - stroke, 0.001f)
                    + (local.y * local.y) / max(ry - stroke, 0.001f) / max(ry - stroke, 0.001f);
        float alpha = saturate((1.0 - saturate((outer - 1.0) * 4.0))
                             - (1.0 - saturate((inner - 1.0) * 4.0)));
        return float4(input.col.rgb, input.col.a * alpha);
    }
}
""",
    "texture": R"""
struct VS_IN { float2 pos : POSITION; float2 uv : TEXCOORD; float4 col : COLOR; };
struct VS_OUT { float4 pos : SV_POSITION; float2 uv : TEXCOORD; float4 col : COLOR; };

SamplerState smp : register(s0);
Texture2D tex : register(t0);

VS_OUT VSMain(VS_IN input) {
    VS_OUT output;
    output.pos = float4(input.pos, 0.0, 1.0);
    output.uv = input.uv;
    output.col = input.col;
    return output;
}

float4 PSMain(VS_OUT input) : SV_TARGET {
    return tex.Sample(smp, input.uv) * input.col;
}
""",
}


def find_fxc():
    """Find the fxc.exe compiler from Windows SDK."""
    # Common paths
    candidates = [
        "fxc.exe",
        "dxc.exe",
    ]
    # Search in Windows SDK paths
    for root in [r"C:\Program Files (x86)\Windows Kits\10\bin"]:
        if os.path.isdir(root):
            for dirpath, dirs, files in os.walk(root):
                if "fxc.exe" in files:
                    return os.path.join(dirpath, "fxc.exe")
    for c in candidates:
        if shutil.which(c):
            return c
    return None


def compile_with_fxc(fxc_path, source, entry, target):
    """Compile HLSL source using fxc.exe. Returns bytes or None."""
    with tempfile.NamedTemporaryFile(suffix=".hlsl", mode="w", delete=False, encoding="utf-8") as f:
        f.write(source)
        tmp_in = f.name

    tmp_out = tmp_in + ".cso"
    try:
        result = subprocess.run(
            [fxc_path, "/T", target, "/E", entry, "/Fo", tmp_out, tmp_in],
            capture_output=True, text=True, timeout=30
        )
        if result.returncode != 0:
            print(f"  WARNING: fxc compile failed for {entry}({target}): {result.stderr}", file=sys.stderr)
            return None
        with open(tmp_out, "rb") as f:
            return f.read()
    except Exception as e:
        print(f"  WARNING: fxc invocation failed: {e}", file=sys.stderr)
        return None
    finally:
        if os.path.exists(tmp_in):
            os.unlink(tmp_in)
        if os.path.exists(tmp_out):
            os.unlink(tmp_out)


def generate_header(bytecode_map, output_path):
    """Generate C++ header with embedded shader bytecode arrays."""
    lines = []
    lines.append("// Auto-generated by tools/compile_shaders.py — DO NOT EDIT")
    lines.append("// Precompiled D3D11 shader bytecode for fast startup.")
    lines.append("// If this file is missing, d3d11_device.cpp falls back to runtime compilation.")
    lines.append("")
    lines.append("#pragma once")
    lines.append("#include <cstdint>")
    lines.append("#include <cstddef>")
    lines.append("")
    lines.append("namespace ltgui { namespace gpu {")
    lines.append("")

    for name, data in sorted(bytecode_map.items()):
        if data is None:
            lines.append(f"// {name}: compilation failed, runtime fallback will be used")
            lines.append(f"inline const uint8_t* k{name.capitalize()}Bytecode = nullptr;")
            lines.append(f"inline const size_t   k{name.capitalize()}BytecodeLen = 0;")
            lines.append("")
            continue

        lines.append(f"// {name} ({len(data)} bytes)")
        lines.append(f"inline const uint8_t k{name.capitalize()}Bytecode[] = {{")
        for i in range(0, len(data), 16):
            chunk = data[i:i+16]
            hex_str = ", ".join(f"0x{b:02x}" for b in chunk)
            lines.append(f"    {hex_str},")
        lines.append("};")
        lines.append(f"inline const size_t k{name.capitalize()}BytecodeLen = {len(data)};")
        lines.append("")

    lines.append("}} // namespace ltgui::gpu")

    with open(output_path, "w") as f:
        f.write("\n".join(lines))
    print(f"Generated {output_path}")


def main():
    output = sys.argv[2] if len(sys.argv) > 2 and sys.argv[1] == "--output" else \
             os.path.join(os.path.dirname(__file__), "..", "src", "platform", "gpu", "d3d11_shaders.h")

    fxc = find_fxc()
    if not fxc:
        print("WARNING: fxc.exe not found. Install Windows SDK to precompile shaders.")
        print("The framework will fall back to runtime D3DCompile().")
        # Generate a stub header
        generate_header({k: None for k in SHADERS}, output)
        return 1

    print(f"Using compiler: {fxc}")
    bytecode_map = {}
    all_ok = True

    for name, info in SHADERS.items():
        print(f"Compiling {name} ({info['entry']} / {info['target']})...")
        source = HLSL_SOURCES[info["src"]]
        data = compile_with_fxc(fxc, source, info["entry"], info["target"])
        if data is None:
            all_ok = False
        bytecode_map[name] = data

    generate_header(bytecode_map, output)
    return 0 if all_ok else 1


if __name__ == "__main__":
    sys.exit(main())
