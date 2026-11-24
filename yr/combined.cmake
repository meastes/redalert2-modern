# SPDX-License-Identifier: GPL-3.0-or-later
# One game (from redalert2-decomp's native/yr/combined.cmake, lanes Y5 to Y16; user design 2026-11-15): Red Alert 2
# and Yuri's Revenge in one app, one engine at a time, a toggle in each main menu's sidebar (control 0x7F5E). With
# -DRA2_NATIVE_COMBINED=ON (and -DRA2_NATIVE_YR=ON). Each engine is the same objects as its game's executable, with
# the SDL host compiled again with RA2_NATIVE_ENGINE (main.cpp's main becomes ra2_engine_main) and only that symbol
# exported, so the two games' identical names stay apart; ra2yr-native is the thin host (combined/one_game.cpp).
# Red Alert 2's engine is the rebuild-based game (ra2/ra2.cmake: rb_units_objs, rb_data_objs, rb_glue_objs and its
# link closure; its units' initialisers deferred to __DATA,__rb_init_func by rb-native-trial's PRE_LINK and run by
# ra2/glue/rb_host_glue.cpp after the game.exe load); Yuri's Revenge's is yr-native's (yr/yr.cmake).
if(NOT RA2_NATIVE_COMBINED)
  return()
endif()

# The Red Alert 2 game's pieces (ra2/ra2.cmake).
set(ra2_engine_ra2_objects $<TARGET_OBJECTS:rb_units_objs> $<TARGET_OBJECTS:rb_data_objs> $<TARGET_OBJECTS:rb_glue_objs>)
set(ra2_engine_ra2_closure "${RB_GEN}/link_closure.c")
set(ra2_engine_ra2_alias_list "${RB_GEN}/alias_list.txt")

set(ra2_engine_link_options "LINKER:-exported_symbol,_ra2_engine_main")

# Each game's SDL host compiled as an engine (main.cpp's main becomes ra2_engine_main). The Apple
# host services (host_apple.mm) are compiled once, outside the engines (lane Y9): ra2yr-one links
# that one copy with the thin host and both engines call it (its Objective-C classes are global in
# the runtime); each Mac bundle links it in as before (no class there: the classes are iOS's).
set(ra2_engine_host_sources ${ra2_host_sources})
set(ra2_engine_hostos "")
if(APPLE)
  list(REMOVE_ITEM ra2_engine_host_sources platform/sdl/host_apple.mm)
  add_library(ra2_engine_hostos_objs OBJECT platform/sdl/host_apple.mm)
  set(ra2_engine_hostos $<TARGET_OBJECTS:ra2_engine_hostos_objs>)
endif()
add_library(ra2_engine_host_objs OBJECT ${ra2_engine_host_sources})
target_link_libraries(ra2_engine_host_objs PRIVATE SDL3::SDL3 ra2_port)
target_compile_definitions(ra2_engine_host_objs PRIVATE RA2_NATIVE_GAME=1 RA2_NATIVE_ENGINE=1)
add_library(yr_engine_host_objs OBJECT ${ra2_engine_host_sources})
target_link_libraries(yr_engine_host_objs PRIVATE SDL3::SDL3 ra2_port)
target_compile_definitions(yr_engine_host_objs PRIVATE RA2_NATIVE_GAME=1 RA2_NATIVE_YR=1 RA2_NATIVE_ENGINE=1)

if(WIN32)
  # Windows (lane Y16; yr/README.md "One game on Windows"): the engines are DLLs of the same
  # objects as ra2-native.exe and yr-native.exe, each exporting only ra2_engine_main (its one
  # __declspec(dllexport): lld then exports nothing else), linked against the one SDL3.dll; the thin
  # host RA2YRNative.exe (GUI subsystem; ra2yr-native.exe, console, for the tests) loads a fresh copy
  # per start (combined/one_game.cpp). The C runtime is the UCRT DLL every module shares.
  function(ra2_windows_engine_link target)
    # -static: libc++, libunwind and winpthreads inside each DLL, as in the .exe builds (the toolchain file's
    # CMAKE_EXE_LINKER_FLAGS_INIT); only the system DLLs, the UCRT and SDL3.dll are imported.
    target_link_options(${target} PRIVATE -static "LINKER:--allow-multiple-definition" "LINKER:--exclude-all-symbols")
    target_sources(${target} PRIVATE "${ra2_win_dir}/ctors_win.c")   # the initialisers in link order (in a DLL too)
    target_link_libraries(${target} PRIVATE ws2_32 iphlpapi winmm shell32 ole32 uuid)
    set_target_properties(${target} PROPERTIES PREFIX "" SUFFIX ".dll")
  endfunction()
  add_library(ra2-engine SHARED $<TARGET_OBJECTS:ra2_engine_host_objs> ${ra2_engine_ra2_objects} "${ra2_engine_ra2_closure}")
  target_link_libraries(ra2-engine PRIVATE SDL3::SDL3 ra2_port)
  target_compile_definitions(ra2-engine PRIVATE RA2_NATIVE_GAME=1)
  ra2_windows_engine_link(ra2-engine)
  add_dependencies(ra2-engine rb-native-trial)
  add_library(yr-engine SHARED $<TARGET_OBJECTS:yr_engine_host_objs> $<TARGET_OBJECTS:yr_units_objs>
              $<TARGET_OBJECTS:yr_data_obj> "${RA2_NATIVE_YR_GEN}/link_closure.c")
  target_link_libraries(yr-engine PRIVATE SDL3::SDL3 ra2_port)
  target_compile_definitions(yr-engine PRIVATE RA2_NATIVE_GAME=1 RA2_NATIVE_YR=1)
  ra2_windows_engine_link(yr-engine)
  add_dependencies(yr-engine yr-native)     # after yr-native's PRE_LINK has moved the initialisers
  foreach(host ra2yr-native RA2YRNative)
    if(host STREQUAL "RA2YRNative")
      add_executable(${host} WIN32 "${CMAKE_CURRENT_SOURCE_DIR}/yr/combined/one_game.cpp")
      target_sources(${host} PRIVATE "${ra2_win_icon_dir}/ra2native.rc")
      target_include_directories(${host} PRIVATE "${ra2_win_icon_dir}")
    else()
      add_executable(${host} "${CMAKE_CURRENT_SOURCE_DIR}/yr/combined/one_game.cpp")
    endif()
    target_link_libraries(${host} PRIVATE SDL3::SDL3)
    target_link_options(${host} PRIVATE "LINKER:--stack,8388608")   # the games run on its main thread
    add_dependencies(${host} ra2-engine yr-engine)
  endforeach()
  # SDL3.dll beside them (the build folder is what is copied to a PC, with the three executables).
  add_custom_command(TARGET RA2YRNative POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy_if_different $<TARGET_FILE:SDL3::SDL3-shared> "${CMAKE_CURRENT_BINARY_DIR}"
    VERBATIM)
  # ctest one-game-toggle-wine (lane Y16): the one-game-toggle route on ra2yr-native.exe under Wine (a
  # fresh prefix deleted after the run); the same conditions as the Mac's.
  add_test(NAME one-game-toggle-wine COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/yr/tests/one_game_toggle.py"
           --binary $<TARGET_FILE:ra2yr-native> --data-dir "${RA2_NATIVE_YR_DATA_DIR}"
           --game-exe "${RA2_NATIVE_CENSUS_GAME_EXE}" --work "${CMAKE_CURRENT_BINARY_DIR}/one-game-toggle-wine"
           --wine-prefix "${CMAKE_CURRENT_BINARY_DIR}/wine/one-game" --wine "${RA2_NATIVE_WINE}")
  set_tests_properties(one-game-toggle-wine PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 900)
  return()
endif()

if(NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")   # bundles: the Mac's (iOS loads no bundle)
# Red Alert 2: rb-native-trial's objects (weakened and their initialisers deferred by its PRE_LINK: built after it).
add_library(ra2-engine MODULE $<TARGET_OBJECTS:ra2_engine_host_objs> ${ra2_engine_hostos} ${ra2_engine_ra2_objects}
            "${ra2_engine_ra2_closure}")
set_source_files_properties("${ra2_engine_ra2_closure}" PROPERTIES COMPILE_OPTIONS "-O1")
target_link_libraries(ra2-engine PRIVATE SDL3::SDL3 ra2_port ${ra2_host_frameworks})
target_compile_definitions(ra2-engine PRIVATE RA2_NATIVE_GAME=1)
target_link_options(ra2-engine PRIVATE ${ra2_engine_link_options} "LINKER:-alias_list,${ra2_engine_ra2_alias_list}")
set_property(TARGET ra2-engine APPEND PROPERTY LINK_DEPENDS "${ra2_engine_ra2_alias_list}")
set_target_properties(ra2-engine PROPERTIES PREFIX "" SUFFIX ".so")
add_dependencies(ra2-engine rb-native-trial)

# Yuri's Revenge: yr-native's objects (weakened and their initialisers deferred by its PRE_LINK).
add_library(yr-engine MODULE $<TARGET_OBJECTS:yr_engine_host_objs> ${ra2_engine_hostos} $<TARGET_OBJECTS:yr_units_objs>
            $<TARGET_OBJECTS:yr_data_obj> "${RA2_NATIVE_YR_GEN}/link_closure.c")
target_link_libraries(yr-engine PRIVATE SDL3::SDL3 ra2_port ${ra2_host_frameworks})
target_compile_definitions(yr-engine PRIVATE RA2_NATIVE_GAME=1 RA2_NATIVE_YR=1)
target_link_options(yr-engine PRIVATE ${ra2_engine_link_options} "LINKER:-alias_list,${RA2_NATIVE_YR_GEN}/alias_list.txt")
set_property(TARGET yr-engine APPEND PROPERTY LINK_DEPENDS "${RA2_NATIVE_YR_GEN}/alias_list.txt")
set_target_properties(yr-engine PROPERTIES PREFIX "" SUFFIX ".so")
add_dependencies(yr-engine yr-native)

# The thin host: the process, SDL, the window and the swap; the engines beside it.
add_executable(ra2yr-native "${CMAKE_CURRENT_SOURCE_DIR}/yr/combined/one_game.cpp")
target_link_libraries(ra2yr-native PRIVATE SDL3::SDL3)
add_dependencies(ra2yr-native ra2-engine yr-engine)

# ctest one-game-toggle (lane Y5): start -> the RA2 main menu -> toggle -> the YR main menu (no
# start-up movie) -> toggle -> the RA2 main menu, then a skirmish started in each game after a
# swap; the selection kept in the settings; no game process left (yr/tests/one_game_toggle.py).
# Needs RA2_NATIVE_YR_DATA_DIR (RA2 + YR folder) and RA2_NATIVE_CENSUS_GAME_EXE (game.exe).
add_test(NAME one-game-toggle COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/yr/tests/one_game_toggle.py"
         --binary $<TARGET_FILE:ra2yr-native> --data-dir "${RA2_NATIVE_YR_DATA_DIR}"
         --game-exe "${RA2_NATIVE_CENSUS_GAME_EXE}" --work "${CMAKE_CURRENT_BINARY_DIR}/one-game-toggle")
set_tests_properties(one-game-toggle PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 600)
# ctest one-game-saves (lane Y10): each game saves in its original's format after the toggle (RA2 -> YR,
# YR -> RA2) and each save loads back in its game (yr/tests/one_game_saves.py); the same conditions.
add_test(NAME one-game-saves COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/yr/tests/one_game_saves.py"
         --binary $<TARGET_FILE:ra2yr-native> --data-dir "${RA2_NATIVE_YR_DATA_DIR}"
         --game-exe "${RA2_NATIVE_CENSUS_GAME_EXE}" --work "${CMAKE_CURRENT_BINARY_DIR}/one-game-saves")
set_tests_properties(one-game-saves PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 700)
# ctest one-game-menu-backdrop (lane Y11): main menu -> Options -> Enhancements -> Options -> main menu
# in each game, the backdrop region drawn (not black, not noise) on Options and Enhancements
# (yr/tests/menu_backdrop.py); the same conditions.
add_test(NAME one-game-menu-backdrop COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/yr/tests/menu_backdrop.py"
         --binary $<TARGET_FILE:ra2yr-native> --data-dir "${RA2_NATIVE_YR_DATA_DIR}"
         --game-exe "${RA2_NATIVE_CENSUS_GAME_EXE}" --work "${CMAKE_CURRENT_BINARY_DIR}/one-game-menu-backdrop")
set_tests_properties(one-game-menu-backdrop PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 400)

endif()

# One game in one executable (lane Y8; yr/README.md "iOS in one executable", stage 1 on the
# Mac): the same two engines as relocatable objects instead of bundles, linked with the thin host
# into ra2yr-one. Each engine object is `ld -r` of its objects between two markers
# (combined/engine_marker.c) with only its entry (renamed ra2_engine_main_<game>) and its markers
# exported, so the games' identical names stay apart as in the bundles; its initialisers move to
# __DATA,__<game>e_init (defer_initializers.py) and the host runs them at each start of the game,
# after copying the game's pristine __data/__bss back (a fresh start without a fresh image).
if(RA2_NATIVE_ONE_EXECUTABLE)
  add_library(ra2_one_port_objs OBJECT ${RA2_PORT_SOURCES})
  target_include_directories(ra2_one_port_objs PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}/port/include")
  target_compile_options(ra2_one_port_objs PRIVATE -Wall -Wno-unused-parameter -Wno-unused-function)
  target_link_libraries(ra2_one_port_objs PRIVATE SDL3::SDL3)
  set(ra2_one_engine_objects "")
  foreach(engine ra2 yr)
    foreach(side begin end)
      add_library(${engine}_one_marker_${side} OBJECT "${CMAKE_CURRENT_SOURCE_DIR}/yr/combined/engine_marker.c")
      target_compile_definitions(${engine}_one_marker_${side} PRIVATE ENGINE=${engine} SIDE=${side})
      target_compile_options(${engine}_one_marker_${side} PRIVATE -fno-common)
    endforeach()
  endforeach()
  add_library(ra2_one_closure OBJECT "${ra2_engine_ra2_closure}")
  target_compile_definitions(ra2_one_closure PRIVATE RA2_NATIVE_GAME=1)
  add_library(yr_one_closure OBJECT "${RA2_NATIVE_YR_GEN}/link_closure.c")
  target_compile_definitions(yr_one_closure PRIVATE RA2_NATIVE_GAME=1 RA2_NATIVE_YR=1)
  # No commons: an engine's zero placeholders must be its own storage inside its bss span, not
  # tentative definitions the final link would merge with the other engine's of the same name.
  target_compile_options(ra2_one_closure PRIVATE -fno-common)
  target_compile_options(yr_one_closure PRIVATE -fno-common)
  set(ra2_one_inputs_ra2 $<TARGET_OBJECTS:ra2_one_marker_begin> $<TARGET_OBJECTS:ra2_engine_host_objs>
      ${ra2_engine_ra2_objects} $<TARGET_OBJECTS:ra2_one_closure>
      $<TARGET_OBJECTS:ra2_one_port_objs> $<TARGET_OBJECTS:ra2_one_marker_end>)
  set(ra2_one_inputs_yr $<TARGET_OBJECTS:yr_one_marker_begin> $<TARGET_OBJECTS:yr_engine_host_objs>
      $<TARGET_OBJECTS:yr_units_objs> $<TARGET_OBJECTS:yr_data_obj> $<TARGET_OBJECTS:yr_one_closure>
      $<TARGET_OBJECTS:ra2_one_port_objs> $<TARGET_OBJECTS:yr_one_marker_end>)
  set(ra2_one_alias_ra2 "${ra2_engine_ra2_alias_list}")
  set(ra2_one_alias_yr "${RA2_NATIVE_YR_GEN}/alias_list.txt")
  set(ra2_one_after_ra2 rb-native-trial)
  set(ra2_one_after_yr yr-native)
  set(ra2_one_r_target "")
  if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    if(CMAKE_OSX_SYSROOT MATCHES "[Ss]imulator")
      set(ra2_one_r_target "--target=arm64-apple-ios${CMAKE_OSX_DEPLOYMENT_TARGET}-simulator")
    else()
      set(ra2_one_r_target "--target=arm64-apple-ios${CMAKE_OSX_DEPLOYMENT_TARGET}")
    endif()
    execute_process(COMMAND xcrun --sdk "${CMAKE_OSX_SYSROOT}" --show-sdk-path OUTPUT_VARIABLE ra2_one_sdk OUTPUT_STRIP_TRAILING_WHITESPACE)
    list(APPEND ra2_one_r_target -isysroot "${ra2_one_sdk}")
  endif()
  foreach(engine ra2 yr)
    set(out "${CMAKE_CURRENT_BINARY_DIR}/${engine}-engine-one.o")
    file(GENERATE OUTPUT "${CMAKE_CURRENT_BINARY_DIR}/${engine}-engine-one.filelist"
         CONTENT "$<JOIN:${ra2_one_inputs_${engine}},\n>\n")
    add_custom_command(OUTPUT "${out}"
      COMMAND "${CMAKE_CXX_COMPILER}" -arch arm64 ${ra2_one_r_target} -r -nostdlib -o "${out}"
              "-Wl,-filelist,${CMAKE_CURRENT_BINARY_DIR}/${engine}-engine-one.filelist"
              "-Wl,-alias_list,${ra2_one_alias_${engine}}"
              "-Wl,-alias,_ra2_engine_main,_ra2_engine_main_${engine}"
              "-Wl,-exported_symbol,_ra2_engine_main_${engine}"
              "-Wl,-exported_symbol,_${engine}_*_begin" "-Wl,-exported_symbol,_${engine}_*_end"
      COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/yr/tools/defer_initializers.py"
              --object "${out}" --section "__${engine}e_init"
      DEPENDS ${ra2_one_inputs_${engine}} "${ra2_one_alias_${engine}}" ${ra2_one_after_${engine}}
              "${CMAKE_CURRENT_BINARY_DIR}/${engine}-engine-one.filelist"
      COMMENT "One executable: the ${engine} engine object"
      COMMAND_EXPAND_LISTS VERBATIM)
    list(APPEND ra2_one_engine_objects "${out}")
  endforeach()
  set_source_files_properties(${ra2_one_engine_objects} PROPERTIES EXTERNAL_OBJECT TRUE GENERATED TRUE)
  add_executable(ra2yr-one "${CMAKE_CURRENT_SOURCE_DIR}/yr/combined/one_game.cpp" ${ra2_engine_hostos} ${ra2_one_engine_objects})
  target_compile_definitions(ra2yr-one PRIVATE RA2_NATIVE_ONE_EXECUTABLE=1)
  target_link_libraries(ra2yr-one PRIVATE SDL3::SDL3 ${ra2_host_frameworks})
  if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
    # The iOS app (build-ios.command, the default since lane Y9): it replaces the Red Alert 2 app,
    # with its bundle id and name (owner ruling 2026-11-16), so the player's Red Alert 2 saves,
    # settings and imported files carry over; it asks only for Yuri's Revenge's files.
    # The bundle id is the played app's (io.github.ra2native.RA2) unless RA2_ONE_BUNDLE_ID names another.
    set(RA2_ONE_BUNDLE_ID "io.github.ra2native.RA2" CACHE STRING "Bundle id of the iOS combined app (RA2YR.app)")
    ra2_ios_app(ra2yr-one RA2YR "${RA2_ONE_BUNDLE_ID}" "Red Alert 2")
  endif()
endif()
# The iOS combined app in the "RA2 Duo Validation" Simulator (label one-ios, opt in: RA2_ONE_IOS_SIM=ON in the Mac
# build; the app from ./build.sh ios simulator). Each test boots that Simulator if needed and shuts it down after;
# no other Simulator is used. one-game-ios-sim: the carry-over route (a Red Alert 2 import, its skirmish and save, the
# toggle, Yuri's Revenge's own import, its skirmish and save, a second YR start after a reset, both saves loaded);
# -fresh: one import of both games in a new install (removed afterwards); -menus: both games' menus and play.
option(RA2_ONE_IOS_SIM "Add the iOS combined app's Simulator checks (label one-ios)" OFF)
get_filename_component(RA2_ONE_IOS_APP "${CMAKE_CURRENT_SOURCE_DIR}/build/ios-simulator/RA2YR.app" ABSOLUTE)
set(RA2_ONE_IOS_APP "${RA2_ONE_IOS_APP}" CACHE PATH "The Simulator RA2YR.app the one-ios checks install")
if(RA2_ONE_IOS_SIM AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
  foreach(one_case "sim|ios_sim_one_game.py|" "sim-fresh|ios_sim_one_game.py|--fresh,--uninstall-after" "sim-menus|ios_sim_menus.py|")
    string(REPLACE "|" ";" one_parts "${one_case}")
    list(GET one_parts 0 one_name)
    list(GET one_parts 1 one_script)
    list(LENGTH one_parts one_n)
    set(one_args "")
    if(one_n GREATER 2)
      list(GET one_parts 2 one_args)
      string(REPLACE "," ";" one_args "${one_args}")
    endif()
    add_test(NAME one-game-ios-${one_name} COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/yr/tests/${one_script}"
             --app "${RA2_ONE_IOS_APP}" --data-dir "${RA2_NATIVE_YR_DATA_DIR}" --work "${CMAKE_CURRENT_BINARY_DIR}/one-ios-${one_name}" ${one_args})
    set_tests_properties(one-game-ios-${one_name} PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE TIMEOUT 1800 LABELS one-ios)
  endforeach()
endif()
if(RA2_NATIVE_ONE_EXECUTABLE AND NOT CMAKE_SYSTEM_NAME STREQUAL "iOS")
  # ctest one-exe-toggle (lane Y8): the one-game-toggle route on ra2yr-one (the second Red Alert 2
  # start is a reset of its data in place, not a fresh image).
  add_test(NAME one-exe-toggle COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/yr/tests/one_game_toggle.py"
           --binary $<TARGET_FILE:ra2yr-one> --data-dir "${RA2_NATIVE_YR_DATA_DIR}"
           --game-exe "${RA2_NATIVE_CENSUS_GAME_EXE}" --work "${CMAKE_CURRENT_BINARY_DIR}/one-exe-toggle")
  set_tests_properties(one-exe-toggle PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 600)
  # ctest one-exe-saves (lane Y10): the one-game-saves route on ra2yr-one.
  add_test(NAME one-exe-saves COMMAND "${Python3_EXECUTABLE}" -B "${CMAKE_CURRENT_SOURCE_DIR}/yr/tests/one_game_saves.py"
           --binary $<TARGET_FILE:ra2yr-one> --data-dir "${RA2_NATIVE_YR_DATA_DIR}"
           --game-exe "${RA2_NATIVE_CENSUS_GAME_EXE}" --work "${CMAKE_CURRENT_BINARY_DIR}/one-exe-saves")
  set_tests_properties(one-exe-saves PROPERTIES SKIP_RETURN_CODE 77 TIMEOUT 700)
endif()
