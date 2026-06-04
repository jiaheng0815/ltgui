#pragma once
//
// Accessibility stubs for screen-reader integration.
//
// Full UIAutomation (Windows), AT-SPI (Linux), and NSAccessibility (macOS)
// integration requires implementing provider interfaces per-platform.
// This file provides the registration hooks so that:
//   - Windows: the HWND is registered with UIA so Narrator sees it
//   - Linux:   AT-SPI bus registration is available
//   - macOS:   NSAccessibility role is set on the NSView
//
// Call Accessibility::registerWindow() after create() and before show().
//
#include "platform/platform.h"

namespace ltgui {

class Accessibility {
public:
    // Register the native window with the platform accessibility system.
    // On Windows this sets the window's UIA Provider; on other platforms
    // it's a no-op until full AT-SPI/NSAccessibility support is added.
    static void registerWindow(void* nativeHandle);

    // Unregister on window close.
    static void unregisterWindow(void* nativeHandle);

    // Set a human-readable name for accessibility tools.
    // Names can be set per-widget and are read by screen readers.
    static void setName(void* nativeHandle, const char* name);
};

} // namespace ltgui
