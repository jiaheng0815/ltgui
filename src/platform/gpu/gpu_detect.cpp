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

#ifdef LTGUI_PLATFORM_MACOS
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/graphics/IOGraphicsLib.h>
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

#ifdef LTGUI_PLATFORM_MACOS
    // Query GPUs via IOKit — iterate PCI devices matching IOPCIDevice
    // and filter for display controller class (0x03).
    {
        CFMutableDictionaryRef match = IOServiceMatching("IOPCIDevice");
        if (match) {
            io_iterator_t iter = 0;
            if (IOServiceGetMatchingServices(kIOMasterPortDefault, match, &iter) == KERN_SUCCESS) {
                io_object_t entry;
                while ((entry = IOIteratorNext(iter)) != 0) {
                    // Check PCI class code — display controller = 0x03xxxx
                    CFTypeRef classCodeRef = IORegistryEntryCreateCFProperty(
                        entry, CFSTR("class-code"), kCFAllocatorDefault, 0);
                    if (classCodeRef) {
                        uint32_t classCode = 0;
                        if (CFGetTypeID(classCodeRef) == CFDataGetTypeID()) {
                            CFDataGetBytes((CFDataRef)classCodeRef,
                                           CFRangeMake(0, sizeof(classCode)),
                                           (UInt8*)&classCode);
                        } else if (CFGetTypeID(classCodeRef) == CFNumberGetTypeID()) {
                            CFNumberGetValue((CFNumberRef)classCodeRef,
                                             kCFNumberSInt32Type, &classCode);
                        }
                        CFRelease(classCodeRef);

                        // Display controller class = 0x03 (upper byte of 24-bit class code)
                        if ((classCode >> 16) == 0x03) {
                            GpuInfo info;
                            info.backend = GpuBackend::OpenGL; // macOS supports GL via CGL

                            // Try to get the model name
                            CFTypeRef modelRef = IORegistryEntryCreateCFProperty(
                                entry, CFSTR("model"), kCFAllocatorDefault, 0);
                            if (modelRef) {
                                if (CFGetTypeID(modelRef) == CFStringGetTypeID()) {
                                    char buf[256] = {};
                                    CFStringGetCString((CFStringRef)modelRef, buf,
                                                       sizeof(buf), kCFStringEncodingUTF8);
                                    info.name = buf;
                                }
                                CFRelease(modelRef);
                            }
                            if (info.name.empty()) info.name = "Apple GPU";

                            // Try to get vendor ID
                            CFTypeRef vendorRef = IORegistryEntryCreateCFProperty(
                                entry, CFSTR("vendor-id"), kCFAllocatorDefault, 0);
                            if (vendorRef) {
                                uint32_t vendorId = 0;
                                if (CFGetTypeID(vendorRef) == CFDataGetTypeID()) {
                                    CFDataGetBytes((CFDataRef)vendorRef,
                                                   CFRangeMake(0, sizeof(vendorId)),
                                                   (UInt8*)&vendorId);
                                } else if (CFGetTypeID(vendorRef) == CFNumberGetTypeID()) {
                                    CFNumberGetValue((CFNumberRef)vendorRef,
                                                     kCFNumberSInt32Type, &vendorId);
                                }
                                CFRelease(vendorRef);
                                info.vendor = vendorFromId(vendorId);
                                info.isIntegrated = (vendorId == 0x8086); // Intel = integrated
                            }

                            // For Apple Silicon (no PCI vendor), mark as integrated
                            if (info.vendor == GpuVendor::Unknown) {
                                info.isIntegrated = true;
                                if (info.name.empty() || info.name == "Apple GPU") {
                                    info.name = "Apple Silicon GPU";
                                }
                            }

                            result.push_back(info);
                        }
                    }
                    IOObjectRelease(entry);
                }
                IOObjectRelease(iter);
            }
        }
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
