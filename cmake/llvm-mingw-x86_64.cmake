# SPDX-License-Identifier: GPL-3.0-or-later
# Compile for x86_64 Windows with llvm-mingw (mstorsjo/llvm-mingw: the macOS universal release on a Mac, the
# ucrt-x86_64 release on a Windows host, README "Building on a Windows PC"; from
# redalert2-decomp's native/cmake). Nothing is installed: the toolchain is the extracted release folder, found as
# -DRA2_LLVM_MINGW=<folder>, else $RA2_LLVM_MINGW, else an extracted release under toolchains/ (README "Building").
#
#   cmake -S . -B build/windows -G Ninja -DCMAKE_TOOLCHAIN_FILE=cmake/llvm-mingw-x86_64.cmake -DRA2_SDL_SOURCE=<SDL 3.4.2>
set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(ra2_llvm_mingw_release llvm-mingw-20261020-ucrt-macos-universal)
set(ra2_exe "")
if(CMAKE_HOST_WIN32)
  set(ra2_llvm_mingw_release llvm-mingw-20261020-ucrt-x86_64)
  set(ra2_exe ".exe")
endif()
if(NOT RA2_LLVM_MINGW AND DEFINED ENV{RA2_LLVM_MINGW})
  set(RA2_LLVM_MINGW "$ENV{RA2_LLVM_MINGW}")
endif()
if(NOT RA2_LLVM_MINGW)
  file(GLOB ra2_llvm_mingw_found "${CMAKE_CURRENT_LIST_DIR}/../toolchains/llvm-mingw-*/${ra2_llvm_mingw_release}"
       "${CMAKE_CURRENT_LIST_DIR}/../toolchains/${ra2_llvm_mingw_release}")
  if(ra2_llvm_mingw_found)
    list(GET ra2_llvm_mingw_found 0 RA2_LLVM_MINGW)
  endif()
endif()
file(TO_CMAKE_PATH "${RA2_LLVM_MINGW}" RA2_LLVM_MINGW)
if(NOT EXISTS "${RA2_LLVM_MINGW}/bin/x86_64-w64-mingw32-clang${ra2_exe}")
  message(FATAL_ERROR "llvm-mingw not found: set RA2_LLVM_MINGW to the extracted ${ra2_llvm_mingw_release} folder")
endif()
# Try-compile projects see the same folder.
set(RA2_LLVM_MINGW "${RA2_LLVM_MINGW}" CACHE PATH "Extracted llvm-mingw release folder")
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES RA2_LLVM_MINGW)

set(ra2_mingw_bin "${RA2_LLVM_MINGW}/bin")
set(CMAKE_C_COMPILER "${ra2_mingw_bin}/x86_64-w64-mingw32-clang${ra2_exe}")
set(CMAKE_CXX_COMPILER "${ra2_mingw_bin}/x86_64-w64-mingw32-clang++${ra2_exe}")
set(CMAKE_ASM_COMPILER "${ra2_mingw_bin}/x86_64-w64-mingw32-clang${ra2_exe}")
set(CMAKE_RC_COMPILER "${ra2_mingw_bin}/x86_64-w64-mingw32-windres${ra2_exe}")
set(CMAKE_AR "${ra2_mingw_bin}/llvm-ar${ra2_exe}" CACHE FILEPATH "")
set(CMAKE_RANLIB "${ra2_mingw_bin}/llvm-ranlib${ra2_exe}" CACHE FILEPATH "")
set(CMAKE_NM "${ra2_mingw_bin}/llvm-nm${ra2_exe}" CACHE FILEPATH "")

# Optimised by default (lane C16, owner 2026-11-14), as the iOS build (build-ios.command
# RA2_IOS_OPT): -O2 and no build type, so no NDEBUG. Before this the Windows build had no
# build type and compiled at -O0. -DRA2_WINDOWS_OPT=<flags> chooses others ("-O0" for a
# debugging build); CMAKE_<LANG>_FLAGS given on the command line take precedence.
if(NOT DEFINED RA2_WINDOWS_OPT)
  set(RA2_WINDOWS_OPT "-O2")
endif()
set(CMAKE_C_FLAGS_INIT "${RA2_WINDOWS_OPT}")
set(CMAKE_CXX_FLAGS_INIT "${RA2_WINDOWS_OPT}")
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES RA2_WINDOWS_OPT)

set(CMAKE_FIND_ROOT_PATH "${RA2_LLVM_MINGW}/x86_64-w64-mingw32")
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)

# One self-contained executable: libc++, libunwind and winpthreads linked in, no
# toolchain DLLs beside it (the UCRT is part of Windows 10 and later).
set(CMAKE_EXE_LINKER_FLAGS_INIT "-static")
