#pragma once

// LTGUI_API: DLL import/export decoration for public classes.
//
//   - LTGUI_STATIC  -> no decoration (static-library build or consumer)
//   - LTGUI_EXPORTS -> __declspec(dllexport) while building the DLL
//   - otherwise     -> __declspec(dllimport) on Windows, empty elsewhere
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
