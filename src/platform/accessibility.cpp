#include "platform/accessibility.h"

#ifdef LTGUI_PLATFORM_WINDOWS
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#include <uiautomationcore.h>
#include <uiautomationcoreapi.h>
#include <string>
#endif

namespace ltgui {

void Accessibility::registerWindow(void* nativeHandle) {
#ifdef LTGUI_PLATFORM_WINDOWS
    if (!nativeHandle) return;
    HWND hwnd = static_cast<HWND>(nativeHandle);

    // Register the window with UIA so screen readers can discover it.
    // Uses the built-in HWND proxy provider — no custom provider needed
    // for basic text reading. Custom controls (TextBox, ListBox, etc.)
    // will need IRawElementProviderSimple implementations for full support.
    IRawElementProviderSimple* provider = nullptr;
    HRESULT hr = UiaHostProviderFromHwnd(hwnd, &provider);
    if (SUCCEEDED(hr) && provider) {
        // The provider is now associated with this HWND.
        // UIA will use it to expose the window to accessibility tools.

        // Raise a window-opened event so screen readers notice the new window.
        // UIA_Window_WindowOpenedEventId is 20010.
        UiaRaiseAutomationEvent(provider, 20010);
        provider->Release();
    }
#else
    (void)nativeHandle;
    // TODO: AT-SPI (Linux) and NSAccessibility (macOS) registration
#endif
}

void Accessibility::unregisterWindow(void* nativeHandle) {
#ifdef LTGUI_PLATFORM_WINDOWS
    if (!nativeHandle) return;
    // UIA proxy providers are automatically cleaned up when the HWND is destroyed.
    (void)nativeHandle;
#else
    (void)nativeHandle;
#endif
}

void Accessibility::setName(void* nativeHandle, const char* name) {
#ifdef LTGUI_PLATFORM_WINDOWS
    if (!nativeHandle || !name) return;
    HWND hwnd = static_cast<HWND>(nativeHandle);

    // Set the accessible name on the window. Screen readers read this
    // as the "name" property of the window element.
    IRawElementProviderSimple* provider = nullptr;
    if (SUCCEEDED(UiaHostProviderFromHwnd(hwnd, &provider)) && provider) {
        // For basic HWND proxy, the window text serves as the accessible name.
        // Custom providers override this with widget-specific names.
        provider->Release();
    }

    // Free the previous name allocation if one exists
    HANDLE prev = GetPropW(hwnd, L"ltgui_AccName");
    if (prev) {
        HeapFree(GetProcessHeap(), 0, prev);
    }

    // Set a window prop for accessibility tools to query
    int len = MultiByteToWideChar(CP_UTF8, 0, name, -1, nullptr, 0);
    if (len > 0) {
        std::wstring wname(static_cast<size_t>(len), L'\0');
        MultiByteToWideChar(CP_UTF8, 0, name, -1, &wname[0], len);
        size_t byteSize = (wname.size() + 1) * sizeof(wchar_t);
        HGLOBAL hMem = ::HeapAlloc(GetProcessHeap(), 0, byteSize);
        if (hMem) {
            memcpy(hMem, wname.c_str(), byteSize);
            SetPropW(hwnd, L"ltgui_AccName", static_cast<HANDLE>(hMem));
        }
    }
#else
    (void)nativeHandle;
    (void)name;
#endif
}

} // namespace ltgui
