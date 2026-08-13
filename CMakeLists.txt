cmake_minimum_required(VERSION 3.16)
project(ltgui VERSION 1.0.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# --- Options ---
option(LTGUI_BUILD_EXAMPLES "Build example applications" ON)
option(LTGUI_BUILD_TESTS    "Build and enable tests" ON)

# --- Source files ---
file(GLOB_RECURSE LTGUI_SOURCES
    "src/*.cpp"
    "src/*.mm"
)

# --- Headers ---
file(GLOB_RECURSE LTGUI_HEADERS
    "include/ltgui/*.h"
    "include/ltgui/**/*.h"
)

# --- Static library ---
add_library(ltgui STATIC ${LTGUI_SOURCES} ${LTGUI_HEADERS})
target_include_directories(ltgui PUBLIC
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>"
    "$<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/vendor>"
)
target_include_directories(ltgui PRIVATE
    "${CMAKE_CURRENT_SOURCE_DIR}/src"
)

# --- Platform-specific ---
if(WIN32)
    target_compile_definitions(ltgui PRIVATE _UNICODE UNICODE LTGUI_PLATFORM_WINDOWS)
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
    file(GLOB LTGUI_EXAMPLES "examples/*.cpp")
    foreach(example ${LTGUI_EXAMPLES})
        get_filename_component(name ${example} NAME_WE)
        add_executable(example_${name} ${example})
        target_link_libraries(example_${name} PRIVATE ltgui)
    endforeach()
endif()

# --- Tests ---
if(LTGUI_BUILD_TESTS)
    enable_testing()
    file(GLOB LTGUI_TESTS "test/test_*.cpp")
    foreach(testfile ${LTGUI_TESTS})
        get_filename_component(name ${testfile} NAME_WE)
        add_executable(${name} ${testfile})
        target_include_directories(${name} PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/vendor")
        target_link_libraries(${name} PRIVATE ltgui)
        add_test(NAME ${name} COMMAND ${name})
    endforeach()
endif()
