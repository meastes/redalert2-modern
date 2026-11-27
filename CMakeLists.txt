# SPDX-License-Identifier: GPL-3.0-or-later
# redalert2-modern: Red Alert 2 for the Mac and iPhone/iPad, built from the rebuild tree of redalert2-decomp
# (vendored at vendor/ra2-rebuild, commit in its PIN) on an SDL3 platform layer. See README.md.
#
#   ./build.sh mac | ios simulator | ios device       (the usual way: configures, builds and packages)
#   cmake -S . -B build/mac -G Ninja -DCMAKE_BUILD_TYPE=Release && cmake --build build/mac
#   ctest --test-dir build/mac -L ra2-quick             (the played checks need RA2_DATA_DIR: the user's game)
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
  # The combined app on Windows shares one SDL3.dll between its host and its two engine DLLs (one SDL owns the window
  # both games draw in): SDL3::SDL3 is then the shared library; the static one stays for the link closure.
  if(WIN32 AND RA2_NATIVE_COMBINED)
    set(SDL_SHARED ON CACHE BOOL "" FORCE)
  else()
    set(SDL_SHARED OFF CACHE BOOL "" FORCE)
  endif()
  set(SDL_STATIC ON CACHE BOOL "" FORCE)
  set(SDL_TEST_LIBRARY OFF CACHE BOOL "" FORCE)
  set(SDL_TESTS OFF CACHE BOOL "" FORCE)
  set(SDL_EXAMPLES OFF CACHE BOOL "" FORCE)
  add_subdirectory("${RA2_SDL_SOURCE}" "${CMAKE_CURRENT_BINARY_DIR}/sdl3" EXCLUDE_FROM_ALL)
elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS" OR WIN32)
  message(FATAL_ERROR "the iOS and Windows builds need RA2_SDL_SOURCE (the SDL 3.4.2 source)")
else()
  find_package(SDL3 3.2 CONFIG REQUIRED)
endif()

# Lane P35: an AddressSanitizer build of everything (configure a separate folder, e.g. build/asan, with
# -DRA2_NATIVE_ASAN=ON): reports are recoverable (halt_on_error=0 in the ra2-asan tests' ASAN_OPTIONS), and
# use-after-scope is off (MixFileClass's Straw chain trips it by the original's destructor order). Adds the ctest label
# ra2-asan (ra2/ra2.cmake): campaign replays that must win at their recorded ticks with no report; with RA2_NATIVE_YR=ON
# also yr-asan (yr/yr.cmake, lane P44): the YR mission soaks with no report, and every yr- test logs its reports.
option(RA2_NATIVE_ASAN "Build with AddressSanitizer (a test build; adds the ra2-asan checks)" OFF)
if(RA2_NATIVE_ASAN)
  add_compile_options(-g -fsanitize=address -fsanitize-recover=address -fno-sanitize-address-use-after-scope
                      -fno-omit-frame-pointer)
  add_link_options(-fsanitize=address)
endif()
# Lane P54: a float-cast-overflow build (UBSan's -fsanitize=float-cast-overflow, recoverable; configure a separate
# folder, e.g. build/ubsan-fc, with -DRA2_NATIVE_UBSAN_FLOAT=ON): every floating-to-integer conversion out of the
# target's range is reported (VC6's x87 _ftol and fistp give such a value a defined result; clang does not). Adds the
# ctest label ra2-ubsan (ra2/ra2.cmake): campaign replays that must win at their recorded ticks with no report.
option(RA2_NATIVE_UBSAN_FLOAT "Build with UBSan's float-cast-overflow check (a test build; adds the ra2-ubsan checks)" OFF)
if(RA2_NATIVE_UBSAN_FLOAT)
  add_compile_options(-g -fsanitize=float-cast-overflow -fsanitize-recover=float-cast-overflow -fno-omit-frame-pointer)
  add_link_options(-fsanitize=float-cast-overflow)
endif()

# The platform seams: Win32 / DirectDraw / DirectSound / Winsock / OLE and the VC6 C runtime surface the game
# imports, implemented on POSIX and SDL3 under the same C names (port/). Built without the VC6 dialect.
file(GLOB RA2_PORT_SOURCES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/port/src/*.cpp")
add_library(ra2_port STATIC ${RA2_PORT_SOURCES})
target_include_directories(ra2_port PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/port/include")
target_compile_options(ra2_port PRIVATE -Wall -Wno-unused-parameter -Wno-unused-function)
if(WIN32)
  # The x86_64 Windows build (cmake/llvm-mingw-x86_64.cmake): the port's Windows layer (platform/win).
  include(cmake/windows.cmake)
  ra2_windows_port()
endif()

# The SDL host (platform/sdl): window, input, touch, the app's setup and the Enhancements page, in its game mode
# (RA2_NATIVE_GAME: the players' app mode, which asks for the user's game folder on first start).
set(ra2_host_sources platform/sdl/main.cpp platform/sdl/host.cpp platform/sdl/touch.cpp platform/sdl/enhancements.cpp
    platform/sdl/mod_import.cpp)
if(APPLE)
  list(APPEND ra2_host_sources platform/sdl/host_apple.mm)
  set_source_files_properties(platform/sdl/host_apple.mm PROPERTIES COMPILE_OPTIONS "-fobjc-arc")
elseif(WIN32)
  list(APPEND ra2_host_sources platform/win/host_win.cpp)
endif()
add_library(ra2_host_objs OBJECT ${ra2_host_sources})
target_link_libraries(ra2_host_objs PRIVATE SDL3::SDL3 ra2_port)
target_compile_definitions(ra2_host_objs PRIVATE RA2_NATIVE_GAME=1)

enable_testing()
include(ra2/ra2.cmake)

# Yuri's Revenge (yr/yr.cmake: yr-native, from the pinned ra2-decomp tree in vendor/ra2-decomp) and the combined app
# (yr/combined.cmake: Red Alert 2 from the rebuild and Yuri's Revenge behind one thin host). From redalert2-decomp's
# native/CMakeLists.txt, which defined the names below for them.
option(RA2_NATIVE_YR "Also build yr-native, Yuri's Revenge from the pinned ra2-decomp tree (vendor/ra2-decomp)" OFF)
option(RA2_NATIVE_COMBINED "Also build the combined app: Red Alert 2 and Yuri's Revenge in one (needs RA2_NATIVE_YR)" OFF)
option(RA2_NATIVE_ONE_EXECUTABLE "Also build ra2yr-one: both engines linked into one executable (the iOS form)" OFF)
if(RA2_NATIVE_COMBINED AND NOT RA2_NATIVE_YR)
  message(FATAL_ERROR "RA2_NATIVE_COMBINED needs RA2_NATIVE_YR=ON")
endif()
if(RA2_NATIVE_YR)
  # The VC6 dialect on clang/arm64 for the YR units (port/include/ra2port/prelude.hpp explains each choice; the Red
  # Alert 2 units take theirs from ra2/tools/ra2_probe.py).
  add_library(ra2_dialect INTERFACE)
  target_compile_options(ra2_dialect INTERFACE
    "$<$<COMPILE_LANGUAGE:CXX>:-fms-extensions;-fms-compatibility;-fdelayed-template-parsing>"
    "$<$<COMPILE_LANGUAGE:CXX>:SHELL:-Xclang -fno-wchar>" "$<$<COMPILE_LANGUAGE:CXX>:-fshort-wchar>"
    "$<$<COMPILE_LANGUAGE:CXX>:-fwrapv;-fno-strict-aliasing>"
    "$<$<COMPILE_LANGUAGE:CXX>:-fno-delete-null-pointer-checks>"
    "$<$<COMPILE_LANGUAGE:CXX>:-mms-bitfields;-Wno-incompatible-ms-struct>"
    "$<$<COMPILE_LANGUAGE:CXX>:-fwritable-strings>"
    "$<$<COMPILE_LANGUAGE:CXX>:-frounding-math>"
    "$<$<COMPILE_LANGUAGE:CXX>:-Wno-error=address-of-temporary;-Wno-register;-Wno-c++11-narrowing;-w>"
    "$<$<COMPILE_LANGUAGE:CXX>:SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/port/include/ra2port/prelude.hpp>")
  if(WIN32)
    # The VC6 dialect on mingw-w64 headers (platform/win/dialect_win.hpp), before the prelude.
    target_compile_options(ra2_dialect BEFORE INTERFACE
      "$<$<COMPILE_LANGUAGE:CXX>:SHELL:-include ${CMAKE_CURRENT_SOURCE_DIR}/platform/win/dialect_win.hpp>")
    # A function that falls off its end without a return (VC6 left eax as it was) returns as on the Mac, whose clang
    # has -fno-strict-return by default; mingw's clang put a trap (int3) there (lane P19: Yuri's Revenge's
    # TechnoClass::VoiceDeploy at an MCV's deploy click, "Red Alert 2 stopped" on codex1's PC).
    target_compile_options(ra2_dialect INTERFACE "$<$<COMPILE_LANGUAGE:CXX>:-fno-strict-return>")
  endif()
  # The host's Apple frameworks and zlib, and what link_closure.py counts as system.
  if(WIN32)
    set(ra2_closure_frameworks "")
    set(RA2_NATIVE_CLOSURE_SYSTEM_LIBS "$<TARGET_FILE:SDL3-static>")
  elseif(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    set(ra2_closure_frameworks Foundation UIKit UniformTypeIdentifiers AVFAudio CoreGraphics)
    set(RA2_NATIVE_CLOSURE_SYSTEM_LIBS "$<TARGET_FILE:SDL3-static>")
  else()
    set(ra2_closure_frameworks Foundation AppKit)
    set(RA2_NATIVE_CLOSURE_SYSTEM_LIBS "")
  endif()
  set(ra2_host_frameworks "")
  if(APPLE)
    set(ra2_host_frameworks -lz)
  endif()
  foreach(f IN LISTS ra2_closure_frameworks)
    list(APPEND ra2_host_frameworks "-framework ${f}")
  endforeach()
  set(ra2_original_layouts_target "")
  if(WIN32)
    set(ra2_original_layouts_target --target windows "--cxx=${CMAKE_CXX_COMPILER}")
  endif()
  if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    # One iOS app bundle: TARGET's executable named EXE in EXE.app, bundle id ID, home-screen name NAME, the icon
    # drawn by make_icon.py with the remaining arguments, SDL3's licence; never game data
    # (packaging/audit_ios_bundle.py). Files in the build folder's ios-<exe>/.
    set(RA2_IOS_BUILD "1" CACHE STRING "CFBundleVersion of the iOS app")
    function(ra2_ios_app target exe id name)
      set(icon_args ${ARGN})
      if(CMAKE_OSX_SYSROOT MATCHES "[Ss]imulator")
        set(RA2_IOS_PLATFORM iPhoneSimulator)
        set(RA2_IOS_PLATFORM_NAME iphonesimulator)
      else()
        set(RA2_IOS_PLATFORM iPhoneOS)
        set(RA2_IOS_PLATFORM_NAME iphoneos)
      endif()
      execute_process(COMMAND xcrun --sdk ${RA2_IOS_PLATFORM_NAME} --show-sdk-version
                      OUTPUT_VARIABLE ra2_ios_sdk_version OUTPUT_STRIP_TRAILING_WHITESPACE)
      set(RA2_IOS_SDK_NAME "${RA2_IOS_PLATFORM_NAME}${ra2_ios_sdk_version}")
      set(RA2_IOS_APP_NAME "${name}")
      set(dir "${CMAKE_CURRENT_BINARY_DIR}/ios-${exe}")
      configure_file("${CMAKE_CURRENT_SOURCE_DIR}/packaging/ios/Info.plist.in" "${dir}/Info.plist.in" @ONLY)
      set(sdl_licence "${RA2_SDL_SOURCE}/LICENSE.txt")
      target_sources(${target} PRIVATE "${sdl_licence}")
      set_source_files_properties("${sdl_licence}" PROPERTIES MACOSX_PACKAGE_LOCATION "licenses")
      set_target_properties(${target} PROPERTIES
        MACOSX_BUNDLE TRUE
        OUTPUT_NAME ${exe}
        MACOSX_BUNDLE_INFO_PLIST "${dir}/Info.plist.in"
        MACOSX_BUNDLE_EXECUTABLE_NAME ${exe}
        MACOSX_BUNDLE_GUI_IDENTIFIER "${id}"
        MACOSX_BUNDLE_BUNDLE_VERSION "${RA2_IOS_BUILD}"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "${PROJECT_VERSION}")
      set(icon_dir "${dir}/icons")
      set(icon_commands)
      set(icon_files)
      foreach(icon AppIcon60x60@2x.png:120 AppIcon60x60@3x.png:180 AppIcon76x76@2x~ipad.png:152 AppIcon83.5x83.5@2x~ipad.png:167)
        string(REPLACE ":" ";" icon "${icon}")
        list(GET icon 0 icon_name)
        list(GET icon 1 icon_size)
        list(APPEND icon_commands COMMAND sips -z ${icon_size} ${icon_size} "${icon_dir}/icon.png" --out "${icon_dir}/${icon_name}")
        list(APPEND icon_files "${icon_dir}/${icon_name}")
      endforeach()
      add_custom_command(OUTPUT ${icon_files}
        COMMAND "${CMAKE_COMMAND}" -E make_directory "${icon_dir}"
        COMMAND "${Python3_EXECUTABLE}" -I "${CMAKE_CURRENT_SOURCE_DIR}/packaging/make_icon.py" --full-bleed ${icon_args} "${icon_dir}/icon.png"
        ${icon_commands}
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/packaging/make_icon.py" COMMENT "iOS app icon (${exe})" VERBATIM)
      add_custom_target(${target}-ios-icons DEPENDS ${icon_files})
      add_dependencies(${target} ${target}-ios-icons)
      add_custom_command(TARGET ${target} POST_BUILD
        COMMAND "${CMAKE_COMMAND}" -E copy ${icon_files} "$<TARGET_BUNDLE_DIR:${target}>" VERBATIM)
    endfunction()
  endif()
  # The YR checks read the same installation as the Red Alert 2 checks (Red Alert 2 with Yuri's Revenge installed).
  set(RA2_NATIVE_YR_DATA_DIR "${RB_DATA_DIR}" CACHE PATH "Installed RA2 + Yuri's Revenge folder for the YR checks (read only)")
  set(RA2_NATIVE_CENSUS_GAME_EXE "${RB_GAME_EXE}")
  include(yr/yr.cmake)
endif()

# Hygiene (no device, no game): the vendored files equal their PIN list, no game data or oversized file is
# tracked, every own source keeps its SPDX line, THIRD_PARTY.md names each vendor folder.
add_test(NAME repo-hygiene COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tools/check_tree.py")
set_tests_properties(repo-hygiene PROPERTIES LABELS "ra2-quick;static")
# Lane P42: the generated CMake lists and #line paths with a Windows host's spellings, under this cmake -P.
add_test(NAME host-paths COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/host_paths.py"
         --cmake "${CMAKE_COMMAND}")
set_tests_properties(host-paths PROPERTIES LABELS "ra2-quick;static")
# Lane P73: ra2/tools/ra2_mapinfo.py's trigger event/action numbers, as the two executables' jump tables give them.
add_test(NAME mapinfo-tables COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/tests/mapinfo_tables.py")
set_tests_properties(mapinfo-tables PROPERTIES LABELS "ra2-quick;static")
