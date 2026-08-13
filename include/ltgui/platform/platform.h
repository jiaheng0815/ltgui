#pragma once

#if defined(_WIN32) || defined(_WIN64)
#ifndef LTGUI_PLATFORM_WINDOWS
#define LTGUI_PLATFORM_WINDOWS
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#elif defined(__linux__)
#ifndef LTGUI_PLATFORM_LINUX
#define LTGUI_PLATFORM_LINUX
#endif
#elif defined(__APPLE__)
#ifndef LTGUI_PLATFORM_MACOS
#define LTGUI_PLATFORM_MACOS
#endif
#else
#error "Unsupported platform"
#endif
