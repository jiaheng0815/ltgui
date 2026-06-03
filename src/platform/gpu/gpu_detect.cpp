#include "platform/gpu/gpu_detect.h"
#include "platform/platform.h"
#include "log.h"
#include <algorithm>

#ifdef LTGUI_PLATFORM_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <dxgi.h>
#pragma comment(lib, "dxgi.lib")
#endif

#ifdef LTGUI_PLATFORM_LINUX
#include <cstring>
#include <fstream>
#include <dirent.h>
#endif

namespace ltgui {
namespace gpu {

static GpuVendor vendorFromId(int vendorId) {
    switch (vendorId) {
    case 0x10DE: return GpuVendor::NVIDIA;
    case 0x1002: return GpuVendor::AMD;
    case 0x8086: return GpuVendor::Intel;
    default:     return GpuVendor::Unknown;
    }
}

std::vector<GpuInfo> detectGpus() {
    std::vector<GpuInfo> result;

#ifdef LTGUI_PLATFORM_WINDOWS
    IDXGIFactory* factory = nullptr;
    HRESULT hr = CreateDXGIFactory(__uuidof(IDXGIFactory), (void**)&factory);
    if (FAILED(hr) || !factory) return result;

    IDXGIAdapter* adapter = nullptr;
    for (UINT i = 0; factory->EnumAdapters(i, &adapter) != DXGI_ERROR_NOT_FOUND; i++) {
        DXGI_ADAPTER_DESC desc;
        if (SUCCEEDED(adapter->GetDesc(&desc))) {
            GpuInfo info;
            // Convert wide string to UTF-8
            char nameBuf[256] = {};
            WideCharToMultiByte(CP_UTF8, 0, desc.Description, -1, nameBuf, sizeof(nameBuf), nullptr, nullptr);
            info.name = nameBuf;
            info.vendor = vendorFromId(desc.VendorId);
            info.backend = GpuBackend::D3D11;
            info.vramBytes = static_cast<int64_t>(desc.DedicatedVideoMemory);
            info.isIntegrated = (desc.DedicatedVideoMemory == 0);
            result.push_back(info);
        }
        adapter->Release();
    }
    factory->Release();
#endif

#ifdef LTGUI_PLATFORM_LINUX
    // Scan /sys/class/drm/ for GPU devices
    DIR* dir = opendir("/sys/class/drm");
    if (dir) {
        struct dirent* ent;
        while ((ent = readdir(dir)) != nullptr) {
            if (strncmp(ent->d_name, "card", 4) != 0) continue;
            // Check if it has a device/vendor file (real GPU, not a connector)
            std::string vendorPath = std::string("/sys/class/drm/") + ent->d_name + "/device/vendor";
            std::ifstream vf(vendorPath);
            if (vf.is_open()) {
                std::string line;
                std::getline(vf, line);
                // line is like "0x10de"
                int vendorId = 0;
                if (line.size() > 2) {
                    vendorId = static_cast<int>(std::stoul(line, nullptr, 16));
                }

                GpuInfo info;
                info.name = "GPU"; // Simplified
                info.vendor = vendorFromId(vendorId);
                info.backend = GpuBackend::OpenGL;
                info.vramBytes = 0;
                info.isIntegrated = (vendorId == 0x8086);

                if (info.vendor == GpuVendor::NVIDIA) info.name = "NVIDIA GPU";
                else if (info.vendor == GpuVendor::AMD) info.name = "AMD GPU";
                else if (info.vendor == GpuVendor::Intel) info.name = "Intel GPU";

                result.push_back(info);
            }
        }
        closedir(dir);
    }
#endif

    // Sort: discrete GPUs first, then by VRAM descending
    std::sort(result.begin(), result.end(), [](const GpuInfo& a, const GpuInfo& b) {
        if (a.isIntegrated != b.isIntegrated) return !a.isIntegrated; // discrete first
        return a.vramBytes > b.vramBytes;
    });

    return result;
}

GpuInfo selectBestGpu() {
    auto gpus = detectGpus();
    if (gpus.empty()) return {};

    auto& best = gpus[0];
    LOG_INFO("GPU", "Detected %zu GPU(s), selected: %s (%s, %lld MB VRAM)",
           gpus.size(), best.name.c_str(),
           best.isIntegrated ? "integrated" : "discrete",
           best.vramBytes / (1024 * 1024));
    return best;
}

} // namespace gpu
} // namespace ltgui
