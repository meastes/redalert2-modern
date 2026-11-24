# SPDX-License-Identifier: GPL-3.0-or-later
# redalert2-modern: Red Alert 2 for the Mac and iPhone/iPad, built from the rebuild tree of redalert2-decomp
# (vendored at vendor/ra2-rebuild, commit in its PIN) on an SDL3 platform layer. See README.md.
#
#   ./build.sh mac | ios simulator | ios device       (the usual way: configures, builds and packages)
#   cmake -S . -B build/mac -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build/mac
#   ctest --test-dir build/mac -L rb-quick             (the played checks need RA2_DATA_DIR: the user's game)
cmake_minimum_required(VERSION 3.24)
project(redalert2_modern VERSION 0.1.0 LANGUAGES C CXX ASM)
if(APPLE)
  # The host's Apple services (platform/sdl/host_apple.mm). Before SDL's directory, so it sees the language.
  enable_language(OBJCXX)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
if(APPLE AND NOT CMAKE_OSX_ARCHITECTURES)
  set(CMAKE_OSX_ARCHITECTURES arm64)
endif()
find_package(Python3 REQUIRED COMPONENTS Interpreter)

# SDL3: built statically from a source folder (required for iOS: no dylib may ship in an iOS app), else the
# installed package (Homebrew's dylib on the Mac, bundled into the app by packaging/make_app.sh).
set(RA2_SDL_SOURCE "" CACHE PATH "SDL 3.4.2 source folder to build statically (iOS)")
if(RA2_SDL_SOURCE)
  set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  set(SDL_STATIC ON CACHE BOOL "" FORCE)
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
  set(SDL_TESTS OFF CACHE BOOL "" FORCE)
  set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
  add_subdirectory("${RA2_SDL_SOURCE}" "${CMAKE_CURRENT_BINARY_DIR}/sdl3" EXCLUDE_FROM_ALL)
elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
  message(FATAL_ERROR "the iOS build needs RA2_SDL_SOURCE (the SDL 3.4.2 source)")
else()
  find_package(SDL3 3.2 CONFIG REQUIRED)
endif()

# The platform seams: Win32 / DirectDraw / DirectSound / Winsock / OLE and the VC6 C runtime surface the game
# imports, implemented on POSIX and SDL3 under the same C names (port/). Built without the VC6 dialect.
file(GLOB RA2_PORT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/port/src/*.cpp")
add_library(ra2_port STATIC ${RA2_PORT_SOURCES})
target_include_directories(ra2_port PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/port/include")
target_compile_options(ra2_port PRIVATE -Wall -Wno-unused-parameter -Wno-unused-function)

# The SDL host (platform/sdl): window, input, touch, the app's setup and the Enhancements page, in its game mode
# (RA2_NATIVE_GAME: the played app's mode, which asks for the user's game folder on first start).
set(ra2_host_sources platform/sdl/main.cpp platform/sdl/host.cpp platform/sdl/touch.cpp platform/sdl/enhancements.cpp
    platform/sdl/mod_import.cpp)
if(APPLE)
  list(APPEND ra2_host_sources platform/sdl/host_apple.mm)
  set_source_files_properties(platform/sdl/host_apple.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
endif()
add_library(ra2_host_objs OBJECT ${ra2_host_sources})
target_link_libraries(ra2_host_objs PRIVATE SDL3::SDL3 ra2_port)
target_compile_definitions(ra2_host_objs PRIVATE RA2_NATIVE_GAME=1)

enable_testing()
include(ra2/ra2.cmake)

# Hygiene (no device, no game): the vendored files equal their PIN list, no game data or oversized file is
# tracked, every own source keeps its SPDX line, THIRD_PARTY.md names each vendor folder.
add_test(NAME repo-hygiene COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/check_tree.py")
set_tests_properties(repo-hygiene PROPERTIES LABELS "rb-quick;static")
