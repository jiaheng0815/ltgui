#pragma once

// LTGUI version — single source of truth.
//
// ltgui.py parses these macros at startup (LTGUI_VERSION) and
// CMakeLists.txt compares them against project(VERSION) at configure
// time. Bump all three places together:
//   1. this file, 2. CMakeLists.txt project(VERSION ...), 3. git tag vX.Y.Z
#define LTGUI_VERSION_MAJOR 1
#define LTGUI_VERSION_MINOR 0
#define LTGUI_VERSION_PATCH 0
#define LTGUI_VERSION_STRING "1.0.0"
