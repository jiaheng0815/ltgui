#pragma once
#include <string>
#include <vector>
#include <cstdint>

namespace ltgui {
namespace gpu {

enum class GpuVendor { Unknown, NVIDIA, AMD, Intel, Other };
enum class GpuBackend { D3D11, OpenGL, None };

struct GpuInfo {
    std::string name;          // "NVIDIA GeForce RTX 4060"
    GpuVendor vendor = GpuVendor::Unknown;
    GpuBackend backend = GpuBackend::None;
    int64_t vramBytes = 0;     // dedicated video memory
    bool isIntegrated = true;  // false = discrete GPU
};

// Detect all GPUs, return sorted by preference (discrete first, then by VRAM)
std::vector<GpuInfo> detectGpus();

// Pick the best GPU for rendering
GpuInfo selectBestGpu();

} // namespace gpu
} // namespace ltgui
