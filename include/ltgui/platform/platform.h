#pragma once

#if defined(_WIN32) || defined(_WIN64)
#  define LTGUI_PLATFORM_WINDOWS
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
#elif defined(__linux__)
#  define LTGUI_PLATFORM_LINUX
#elif defined(__APPLE__)
#  define LTGUI_PLATFORM_MACOS
#else
#  error "Unsupported platform"
#endif
