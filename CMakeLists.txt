cmake_minimum_required(VERSION 3.16)
project(ltgui VERSION 1.0.0 LANGUAGES CXX)

# --- Version check (single source of truth: include/ltgui/version.h) ---
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/include/ltgui/version.h" LTGUI_VERSION_LINES
     REGEX "^#define LTGUI_VERSION_(MAJOR|MINOR|PATCH) [0-9]+$")
if(NOT LTGUI_VERSION_LINES)
    message(FATAL_ERROR "include/ltgui/version.h is missing LTGUI_VERSION_MAJOR/MINOR/PATCH macros.")
endif()
string(REGEX MATCH "LTGUI_VERSION_MAJOR ([0-9]+)" _ ${LTGUI_VERSION_LINES})
set(_LTGUI_VER_MAJ ${CMAKE_MATCH_1})
string(REGEX MATCH "LTGUI_VERSION_MINOR ([0-9]+)" _ ${LTGUI_VERSION_LINES})
set(_LTGUI_VER_MIN ${CMAKE_MATCH_1})
string(REGEX MATCH "LTGUI_VERSION_PATCH ([0-9]+)" _ ${LTGUI_VERSION_LINES})
set(_LTGUI_VER_PAT ${CMAKE_MATCH_1})
set(_LTGUI_VER "${_LTGUI_VER_MAJ}.${_LTGUI_VER_MIN}.${_LTGUI_VER_PAT}")
if(NOT "${_LTGUI_VER}" STREQUAL "${PROJECT_VERSION}")
    message(FATAL_ERROR
        "Version mismatch: include/ltgui/version.h says ${_LTGUI_VER} but "
        "project(VERSION) says ${PROJECT_VERSION}. Keep both in sync (bump "
        "version.h + CMakeLists.txt + git tag together).")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Options ---
option(LTGUI_BUILD_EXAMPLES "Build example applications" ON)
option(LTGUI_BUILD_TESTS    "Build and enable tests" ON)
option(LTGUI_BUILD_APPS     "Build app targets" ON)

# GNUInstallDirs must be available before the target's INSTALL_INTERFACE
# include dirs are computed (they use CMAKE_INSTALL_INCLUDEDIR).
include(GNUInstallDirs)

if(MSVC)
    # /utf-8 fixes C4819 warnings on Chinese-locale systems. Project-wide:
    # tests/examples/apps compile Chinese comments and literals too.
    add_compile_options(/utf-8)
endif()

# --- Source files ---
file(GLOB_RECURSE LTGUI_SOURCES CONFIGURE_DEPENDS
    "src/*.cpp"
    "src/*.mm"
)

# --- Headers ---
file(GLOB_RECURSE LTGUI_HEADERS CONFIGURE_DEPENDS
    "include/ltgui/*.h"
    "include/ltgui/**/*.h"
)

# --- Static library ---
add_library(ltgui STATIC ${LTGUI_SOURCES} ${LTGUI_HEADERS})
# Consumer side of the LTGUI_API decoration: headers must not assume
# __declspec(dllimport) when linking the static library (propagates to
# tests/examples/apps through target_link_libraries).
target_compile_definitions(ltgui PUBLIC LTGUI_STATIC)
# Export the required standard to consumers via find_package(ltgui).
target_compile_features(ltgui PUBLIC cxx_std_20)
target_include_directories(ltgui PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include/ltgui>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>"
    "$<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/ltgui>"
)
target_include_directories(ltgui PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

# --- Platform-specific ---
if(WIN32)
    # PUBLIC so tests/examples/apps linked against ltgui get the same
    # UNICODE/UI-flag state as the library itself (prevents TCHAR/LPWSTR
    # mismatches in Win32 APIs).
    target_compile_definitions(ltgui PUBLIC _UNICODE UNICODE LTGUI_PLATFORM_WINDOWS)
    target_link_libraries(ltgui PRIVATE
        gdi32 gdiplus user32 comctl32 ole32 imm32
        d3d11 dxgi d3dcompiler
    )
elseif(APPLE)
    target_compile_definitions(ltgui PRIVATE LTGUI_PLATFORM_MACOS)
    target_link_libraries(ltgui PRIVATE
        "-framework Cocoa"
        "-framework CoreGraphics"
        "-framework CoreText"
    )
else()
    target_compile_definitions(ltgui PRIVATE LTGUI_PLATFORM_LINUX)
    find_package(Freetype REQUIRED)
    find_package(EGL REQUIRED)
    find_package(OpenGLES REQUIRED)
    find_package(Fontconfig REQUIRED)
    find_package(X11 REQUIRED)
    target_link_libraries(ltgui PRIVATE
        X11 Xft Xrender
        Freetype::Freetype
        EGL::EGL
        OpenGLES::OpenGLES
        Fontconfig::Fontconfig
    )
endif()

# --- Warnings ---
if(MSVC)
    target_compile_options(ltgui PRIVATE /W4 /wd4100)
else()
    target_compile_options(ltgui PRIVATE -Wall -Wextra -Wpedantic -Wno-unused-parameter -Wno-missing-field-initializers)
endif()

# --- Examples ---
if(LTGUI_BUILD_EXAMPLES)
    file(GLOB LTGUI_EXAMPLES CONFIGURE_DEPENDS "examples/*.cpp")
    foreach(example ${LTGUI_EXAMPLES})
        get_filename_component(name ${example} NAME_WE)
        add_executable(example_${name} ${example})
        target_link_libraries(example_${name} PRIVATE ltgui)
    endforeach()
endif()

# --- Apps ---
if(LTGUI_BUILD_APPS)
    file(GLOB LTGUI_APPS CONFIGURE_DEPENDS "app/*.cpp")
    foreach(app ${LTGUI_APPS})
        get_filename_component(name ${app} NAME_WE)
        add_executable(app_${name} ${app})
        target_link_libraries(app_${name} PRIVATE ltgui)
    endforeach()
endif()

# --- Tests ---
if(LTGUI_BUILD_TESTS)
    enable_testing()
    file(GLOB LTGUI_TESTS CONFIGURE_DEPENDS "test/test_*.cpp")
    foreach(testfile ${LTGUI_TESTS})
        get_filename_component(name ${testfile} NAME_WE)
        add_executable(${name} ${testfile})
        target_include_directories(${name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/vendor")
        target_link_libraries(${name} PRIVATE ltgui)
        add_test(NAME ${name} COMMAND ${name})
    endforeach()
endif()

# --- Install / find_package(ltgui) ---
include(CMakePackageConfigHelpers)

install(TARGETS ltgui EXPORT ltguiTargets
    ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(DIRECTORY include/ltgui DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
# stb_truetype.h guards gpu_font_atlas.h via __has_include; ship it inside
# the ltgui include dir so the installed headers find it.
install(FILES vendor/stb_truetype.h DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/ltgui)

install(EXPORT ltguiTargets
    FILE ltguiTargets.cmake
    NAMESPACE ltgui::
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ltgui
)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/ltgui-config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/ltgui-config.cmake"
    INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ltgui
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/ltgui-config-version.cmake"
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/ltgui-config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/ltgui-config-version.cmake"
    DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/ltgui
)
