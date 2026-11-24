# SPDX-License-Identifier: GPL-3.0-or-later
# The Red Alert 2 game built from the vendored rebuild tree (vendor/ra2-rebuild), included by the top-level
# CMakeLists.txt after it has defined ra2_port (port/), ra2_host_objs (the SDL host, platform/sdl) and SDL3.
# From redalert2-decomp's branch ports-on-rebuild (native/rb/CMakeLists.txt, lanes P1 to P11), which linked a
# src-based build's port library and host objects; here they are targets of this project.
#
# Steps: generate the units (ra2/tools/rb_units.py), compile them with the YR build's flags (rb_probe.py
# --print-flags), bind the data they read but do not define (rb_data.py: .bss storage; rb_original_data.py:
# initialised globals loaded from the user's game.exe at run time), bind undefined functions to the rebuild's
# definition at the same original address (rb_aliases.py), close the link over the rest with traps
# (tools/link_closure.py), and before the link give repeated header constants select-any semantics
# (weaken_duplicates.py) and move the units' static initialisers out of dyld's list
# (yr/tools/defer_initializers.py, __DATA,__rb_init_func) so that glue/rb_host_glue.cpp runs them after the
# game.exe load. Names and addresses come from vendor/ra2-rebuild/symbols.tsv; no game file is read to build.
set(RB_DIR "${CMAKE_CURRENT_SOURCE_DIR}/ra2")
set(RB_ROOT "${CMAKE_CURRENT_SOURCE_DIR}")
option(RB_NON_MATCHING "Compile the rebuild's parked drafts too (-DRA2_NON_MATCHING; WinMain is one of them)" ON)
set(RB_PYTHON3 "${Python3_EXECUTABLE}")
set(RB_PYTHON "${Python3_EXECUTABLE}")
set(rb_port_archive "$<TARGET_FILE:ra2_port>")
set(rb_host_objs "$<TARGET_OBJECTS:ra2_host_objs>")
# The iOS form (CMAKE_SYSTEM_NAME=iOS; build.sh ios): the app is RBTrial.app (bundle id io.github.ra2native.RBTrial),
# SDL3 built statically from RA2_SDL_SOURCE, and the user's game read at run time as on the Mac.
if(CMAKE_SYSTEM_NAME STREQUAL "iOS")
  set(RB_IOS ON)
  set(RB_SDL3 "$<TARGET_FILE:SDL3-static>")
  set(rb_frameworks Foundation UIKit UniformTypeIdentifiers AVFAudio CoreGraphics)
  # SDL3's own (its static library's interface).
  set(rb_sdl_frameworks AVFoundation AudioToolbox CoreAudio CoreBluetooth CoreHaptics CoreMedia CoreMotion CoreVideo GameController
      IOKit Metal OpenGLES QuartzCore UIKit CoreFoundation)
  set(rb_closure_sys "--sdk=${CMAKE_OSX_SYSROOT}" --frameworks ${rb_frameworks} --system-libs "${RB_SDL3}")
else()
  set(RB_IOS OFF)
  set(RB_SDL3 SDL3::SDL3)
  set(rb_closure_sys --sdk= --frameworks Foundation AppKit --system-libs)
endif()

# The generated tree (at configure time; re-run when the pin, the fixups or a glue append change).
set(RB_GEN "${CMAKE_CURRENT_BINARY_DIR}/gen")
execute_process(COMMAND "${RB_PYTHON3}" -B "${RB_DIR}/tools/rb_units.py" --out "${RB_GEN}" RESULT_VARIABLE rb_rc)
if(rb_rc)
  message(FATAL_ERROR "rb_units.py failed")
endif()
execute_process(COMMAND "${RB_PYTHON3}" -B "${RB_DIR}/tools/rb_probe.py" --gen "${RB_GEN}" --print-flags
                OUTPUT_VARIABLE rb_unit_flags RESULT_VARIABLE rb_rc)
if(rb_rc)
  message(FATAL_ERROR "rb_probe.py --print-flags failed")
endif()
# Two-word options kept together (CMake folds repeated options: the second -include went missing).
set(rb_unit_options "")
set(rb_pending "")
foreach(rb_flag IN LISTS rb_unit_flags)
  if(rb_pending)
    list(APPEND rb_unit_options "SHELL:${rb_pending} ${rb_flag}")
    set(rb_pending "")
  elseif(rb_flag MATCHES "^-(include|idirafter|Xclang|arch)$")
    set(rb_pending "${rb_flag}")
  else()
    list(APPEND rb_unit_options "${rb_flag}")
  endif()
endforeach()
file(GLOB rb_appends "${RB_DIR}/glue/*.inc")
set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${rb_appends} "${RB_DIR}/PIN" "${RB_DIR}/fixups.json"
  "${RB_DIR}/tools/rb_units.py" "${RB_DIR}/tools/rb_probe.py")
include("${RB_GEN}/units.cmake")

# The units.
# Lane P8: native additions compiled as units (the units' flags and headers; no port rewrites, no VC6 for-scope rewrite):
# glue/unit/*.cpp (HD vehicles).
file(GLOB rb_unit_glue CONFIGURE_DEPENDS "${RB_DIR}/glue/unit/*.cpp")
add_library(rb_units_objs OBJECT ${RB_NATIVE_UNITS} ${rb_unit_glue})
set_target_properties(rb_units_objs PROPERTIES CXX_STANDARD 17)
target_compile_options(rb_units_objs PRIVATE ${rb_unit_options})
if(RB_NON_MATCHING)
  target_compile_definitions(rb_units_objs PRIVATE RA2_NON_MATCHING)
endif()

# The host's hooks and the native bodies outside any unit; the VC6 C runtime spellings (the YR port's).
file(GLOB rb_glue_sources CONFIGURE_DEPENDS "${RB_DIR}/glue/*.cpp")
add_library(rb_glue_objs OBJECT ${rb_glue_sources} "${RB_ROOT}/yr/glue/yr_crt.cpp")
set_target_properties(rb_glue_objs PROPERTIES CXX_STANDARD 17)
target_compile_options(rb_glue_objs PRIVATE -O2)

# Data: the undefined names of units + port, bound by the rebuild catalog.
file(GENERATE OUTPUT "${RB_GEN}/link_inputs0.txt"
     CONTENT "$<JOIN:$<TARGET_OBJECTS:rb_units_objs>;${rb_port_archive},\n>\n")
add_custom_command(
  OUTPUT "${RB_GEN}/rb_bss_globals.cpp" "${RB_GEN}/rb_original_data.cpp" "${RB_GEN}/rb_bss_aliases.tsv"
  COMMAND "${RB_PYTHON3}" -B "${RB_ROOT}/tools/link_closure.py" --inputs "${RB_GEN}/link_inputs0.txt"
          --host ${rb_host_objs} --out "${RB_GEN}/link_closure0.c" ${rb_closure_sys} --list-undefined "${RB_GEN}/undefined.txt"
  COMMAND "${RB_PYTHON}" -B "${RB_DIR}/tools/rb_data.py" --gen "${RB_GEN}" --undefined "${RB_GEN}/undefined.txt"
  COMMAND "${RB_PYTHON}" -B "${RB_DIR}/tools/rb_original_data.py" --gen "${RB_GEN}"
  DEPENDS rb_units_objs ra2_port ra2_host_objs $<TARGET_OBJECTS:rb_units_objs> "${rb_port_archive}" "${RB_DIR}/tools/rb_data.py"
          "${RB_DIR}/tools/rb_original_data.py" "${RB_ROOT}/tools/link_closure.py"
  WORKING_DIRECTORY "${RB_GEN}" COMMAND_EXPAND_LISTS VERBATIM)
# The original's save format (lane P4): field maps from the units' i686 and arm64 record layouts.
add_custom_command(
  OUTPUT "${RB_GEN}/rb_original_layouts.cpp"
  COMMAND "${RB_PYTHON3}" -B "${RB_DIR}/tools/rb_original_layouts.py" --gen "${RB_GEN}" --out "${RB_GEN}/rb_original_layouts.cpp"
          --report "${RB_GEN}/rb_original_layouts.json" --members "${RB_GEN}/rb_original_layouts_members.json" -j 8
  DEPENDS ${RB_NATIVE_UNITS} "${RB_DIR}/tools/rb_original_layouts.py" "${RB_DIR}/data/original_save_types.txt"
          "${RB_ROOT}/tools/original_layouts.py"
  WORKING_DIRECTORY "${RB_GEN}" VERBATIM)
add_library(rb_data_objs OBJECT "${RB_GEN}/rb_bss_globals.cpp" "${RB_GEN}/rb_original_data.cpp" "${RB_GEN}/rb_original_layouts.cpp")
set_target_properties(rb_data_objs PROPERTIES CXX_STANDARD 17)
target_compile_options(rb_data_objs PRIVATE -O2 -fno-zero-initialized-in-bss)

# Functions: aliases by address, then traps for the rest.
file(GENERATE OUTPUT "${RB_GEN}/link_inputs.txt"
     CONTENT "$<JOIN:$<TARGET_OBJECTS:rb_units_objs>;$<TARGET_OBJECTS:rb_data_objs>;$<TARGET_OBJECTS:rb_glue_objs>;${rb_port_archive},\n>\n")
file(GENERATE OUTPUT "${RB_GEN}/link_order.txt" CONTENT "$<JOIN:$<TARGET_OBJECTS:rb_units_objs>,\n>\n")
add_custom_command(
  OUTPUT "${RB_GEN}/link_closure.c" "${RB_GEN}/alias_list.txt" "${RB_GEN}/link_closure.json"
  COMMAND "${RB_PYTHON3}" -B "${RB_ROOT}/tools/link_closure.py" --inputs "${RB_GEN}/link_inputs.txt"
          --host ${rb_host_objs} --out "${RB_GEN}/link_closure1.c" ${rb_closure_sys} --list-undefined "${RB_GEN}/undefined1.txt"
  COMMAND "${RB_PYTHON}" -B "${RB_DIR}/tools/rb_aliases.py" --gen "${RB_GEN}" --undefined "${RB_GEN}/undefined1.txt"
          --inputs "${RB_GEN}/link_inputs.txt" --extra "${RB_GEN}/rb_bss_aliases.tsv" "${RB_DIR}/data/aliases.tsv" --out "${RB_GEN}/rb_aliases.tsv"
  COMMAND "${RB_PYTHON3}" -B "${RB_ROOT}/tools/link_closure.py" --inputs "${RB_GEN}/link_inputs.txt"
          --host ${rb_host_objs} --out "${RB_GEN}/link_closure.c" ${rb_closure_sys}
          --aliases "${RB_GEN}/rb_aliases.tsv" --alias-list "${RB_GEN}/alias_list.txt"
  DEPENDS rb_units_objs rb_data_objs rb_glue_objs ra2_port ra2_host_objs $<TARGET_OBJECTS:rb_units_objs> $<TARGET_OBJECTS:rb_data_objs>
          $<TARGET_OBJECTS:rb_glue_objs> "${rb_port_archive}" "${RB_GEN}/rb_bss_aliases.tsv"
          "${RB_DIR}/tools/rb_aliases.py" "${RB_DIR}/data/aliases.tsv" "${RB_ROOT}/tools/link_closure.py"
  WORKING_DIRECTORY "${RB_GEN}" COMMAND_EXPAND_LISTS VERBATIM)

add_executable(rb-native-trial ${rb_host_objs} $<TARGET_OBJECTS:rb_units_objs> $<TARGET_OBJECTS:rb_data_objs>
               $<TARGET_OBJECTS:rb_glue_objs> "${RB_GEN}/link_closure.c")
set_source_files_properties("${RB_GEN}/link_closure.c" PROPERTIES COMPILE_OPTIONS "-O1")
if(RB_IOS)
  set(rb_link_frameworks "")
  foreach(f IN LISTS rb_frameworks rb_sdl_frameworks)
    list(APPEND rb_link_frameworks "-framework ${f}")
  endforeach()
  list(REMOVE_DUPLICATES rb_link_frameworks)
  target_link_libraries(rb-native-trial PRIVATE SDL3-static ra2_port z ${rb_link_frameworks})
  target_link_options(rb-native-trial PRIVATE "LINKER:-alias_list,${RB_GEN}/alias_list.txt")
  # The bundle: main's Info.plist template (packaging/ios/Info.plist.in, read only) with the trial's own name and id.
  if(CMAKE_OSX_SYSROOT MATCHES "[Ss]imulator")
    set(RA2_IOS_PLATFORM iPhoneSimulator)
    set(RA2_IOS_PLATFORM_NAME iphonesimulator)
  else()
    set(RA2_IOS_PLATFORM iPhoneOS)
    set(RA2_IOS_PLATFORM_NAME iphoneos)
  endif()
  execute_process(COMMAND xcrun --sdk ${RA2_IOS_PLATFORM_NAME} --show-sdk-version
                  OUTPUT_VARIABLE rb_ios_sdk_version OUTPUT_STRIP_TRAILING_WHITESPACE)
  set(RA2_IOS_SDK_NAME "${RA2_IOS_PLATFORM_NAME}${rb_ios_sdk_version}")
  # The home-screen name (the user, 2026-11-24): "RA2", as the Mac app is RA2.app. The bundle id stays
  # io.github.ra2native.RBTrial, distinct from the played app's (io.github.ra2native.RA2), until parity.
  set(RA2_IOS_APP_NAME "RA2")
  configure_file("${RB_ROOT}/packaging/ios/Info.plist.in" "${CMAKE_CURRENT_BINARY_DIR}/ios/Info.plist.in" @ONLY)
  set_target_properties(rb-native-trial PROPERTIES
    MACOSX_BUNDLE TRUE
    OUTPUT_NAME RBTrial
    MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_BINARY_DIR}/ios/Info.plist.in"
    MACOSX_BUNDLE_EXECUTABLE_NAME RBTrial
    MACOSX_BUNDLE_GUI_IDENTIFIER io.github.ra2native.RBTrial
    MACOSX_BUNDLE_BUNDLE_VERSION 1
    MACOSX_BUNDLE_SHORT_VERSION_STRING 0.1)
  # SDL3's licence and the home-screen icons, as main's ra2_ios_app (native/CMakeLists.txt): the audit
  # (packaging/audit_ios_bundle.py) requires them; never game data.
  set(rb_sdl_licence "${RA2_SDL_SOURCE}/LICENSE.txt")
  if(NOT EXISTS "${rb_sdl_licence}")
    message(FATAL_ERROR "RA2_SDL_SOURCE (${RA2_SDL_SOURCE}) has no LICENSE.txt")
  endif()
  target_sources(rb-native-trial PRIVATE "${rb_sdl_licence}")
  set_source_files_properties("${rb_sdl_licence}" PROPERTIES MACOSX_PACKAGE_LOCATION "licenses")
  set(rb_icon_dir "${CMAKE_CURRENT_BINARY_DIR}/ios/icons")
  set(rb_icon_commands)
  set(rb_icon_files)
  foreach(icon AppIcon60x60@2x.png:120 AppIcon60x60@3x.png:180 AppIcon76x76@2x~ipad.png:152 AppIcon83.5x83.5@2x~ipad.png:167)
    string(REPLACE ":" ";" icon "${icon}")
    list(GET icon 0 icon_name)
    list(GET icon 1 icon_size)
    list(APPEND rb_icon_commands COMMAND sips -z ${icon_size} ${icon_size} "${rb_icon_dir}/icon.png" --out "${rb_icon_dir}/${icon_name}")
    list(APPEND rb_icon_files "${rb_icon_dir}/${icon_name}")
  endforeach()
  add_custom_command(OUTPUT ${rb_icon_files}
    COMMAND "${CMAKE_COMMAND}" -E make_directory "${rb_icon_dir}"
    COMMAND "${RB_PYTHON3}" -I "${RB_ROOT}/packaging/make_icon.py" --full-bleed "${rb_icon_dir}/icon.png"
    ${rb_icon_commands}
    DEPENDS "${RB_ROOT}/packaging/make_icon.py" COMMENT "iOS app icon (RBTrial)" VERBATIM)
  add_custom_target(rb-ios-icons DEPENDS ${rb_icon_files})
  add_dependencies(rb-native-trial rb-ios-icons)
  add_custom_command(TARGET rb-native-trial POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy ${rb_icon_files} "$<TARGET_BUNDLE_DIR:rb-native-trial>" VERBATIM)
else()
  target_link_libraries(rb-native-trial PRIVATE SDL3::SDL3 ra2_port z "-framework Foundation" "-framework AppKit")
  target_link_options(rb-native-trial PRIVATE "LINKER:-alias_list,${RB_GEN}/alias_list.txt" "LINKER:-rpath,/opt/homebrew/lib")
endif()
set_property(TARGET rb-native-trial APPEND PROPERTY LINK_DEPENDS "${RB_GEN}/alias_list.txt")
add_custom_command(TARGET rb-native-trial PRE_LINK
  COMMAND "${RB_PYTHON3}" -B "${RB_ROOT}/tools/weaken_duplicates.py" --inputs "${RB_GEN}/link_order.txt"
          --report "${RB_GEN}/duplicates.json"
  COMMAND "${RB_PYTHON3}" -B "${RB_ROOT}/yr/tools/defer_initializers.py" --inputs "${RB_GEN}/link_order.txt"
          --section __rb_init_func --only /rb_units_objs.dir/
  VERBATIM)

# Checks (lane P3): the scripted runs through the real menus, each in a fresh write folder under the
# build (tests/rb_routes.py). Skipped (77) without the installed game or the pinned target game.exe:
#   ctest --test-dir build/rb-cmake [-L rb-quick]      (rb-native-soak alone takes about 8 minutes)
set(RB_DATA_DIR "$ENV{RA2_DATA_DIR}" CACHE PATH "The user's installed game (read only, at run time; the played checks skip without it)")
set(RB_GAME_EXE "" CACHE FILEPATH "The game.exe whose stored data the game loads (default: the one in RB_DATA_DIR; a supported build, port/include/ra2port/game_builds.hpp)")
if(NOT RB_GAME_EXE AND RB_DATA_DIR)
  set(RB_GAME_EXE "${RB_DATA_DIR}/game.exe")
endif()
enable_testing()
# Lane P4: every type in data/original_save_types.txt has an i686 and an arm64 layout in the rebuild units
# and their members pair completely (main's native-original-layouts over the rebuild).
add_test(NAME rb-native-original-layouts COMMAND "${RB_PYTHON3}" -B "${RB_DIR}/tools/rb_original_layouts.py" --gen "${RB_GEN}"
         --out "${CMAKE_CURRENT_BINARY_DIR}/rb_original_layouts_check.cpp" --check)
set_tests_properties(rb-native-original-layouts PROPERTIES LABELS rb-quick TIMEOUT 600)
foreach(rb_route skirmish soak exit audio save resave original combat combatsave production)
  set(rb_test rb-native-${rb_route})
  if(rb_route STREQUAL "exit")
    set(rb_test rb-native-exit-route)
  endif()
  add_test(NAME ${rb_test} COMMAND "${RB_PYTHON3}" -B "${RB_DIR}/tests/rb_routes.py" --route ${rb_route}
           --binary $<TARGET_FILE:rb-native-trial> --data-dir "${RB_DATA_DIR}" --game-exe "${RB_GAME_EXE}"
           --work "${CMAKE_CURRENT_BINARY_DIR}/tests/${rb_route}")
  set_tests_properties(${rb_test} PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE TIMEOUT 600)
  if(NOT rb_route STREQUAL "soak")
    set_tests_properties(${rb_test} PROPERTIES LABELS rb-quick)
  endif()
endforeach()
# Lane P6: the campaign route over the user's two later Allied saves (ALL02S, ALL03U), each played on to its scripted
# triggers (label rb-campaign, about 2 to 3 minutes each).
foreach(rb_save ALL02S.sav ALL03U.sav)
  add_test(NAME rb-native-campaign-${rb_save} COMMAND "${RB_PYTHON3}" -B "${RB_DIR}/tests/rb_routes.py" --route campaign --save ${rb_save}
           --binary $<TARGET_FILE:rb-native-trial> --data-dir "${RB_DATA_DIR}" --game-exe "${RB_GAME_EXE}"
           --work "${CMAKE_CURRENT_BINARY_DIR}/tests/campaign-${rb_save}")
  set_tests_properties(rb-native-campaign-${rb_save} PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE TIMEOUT 600 LABELS rb-campaign)
endforeach()
# Lane P5: the resave route over the user's other four saves in the install folder (rb-native-resave is ALL01T);
# label rb-resaves, not rb-quick (about 2.5 minutes each).
foreach(rb_save ALL02S.sav ALL03U.sav SAVE397A.SAV SAVE4FBD.SAV)
  add_test(NAME rb-native-resave-${rb_save} COMMAND "${RB_PYTHON3}" -B "${RB_DIR}/tests/rb_routes.py" --route resave --save ${rb_save}
           --binary $<TARGET_FILE:rb-native-trial> --data-dir "${RB_DATA_DIR}" --game-exe "${RB_GAME_EXE}"
           --work "${CMAKE_CURRENT_BINARY_DIR}/tests/resave-${rb_save}")
  set_tests_properties(rb-native-resave-${rb_save} PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE TIMEOUT 600 LABELS rb-resaves)
endforeach()
# Lane P7: the forced end (RA2_NATIVE_FORCE_END, glue/mainloop_native.inc): a skirmish won or lost at loop frame 300, its
# score screen and Continue back to the main menu (rb-quick); main's campaign chain over the rebuild, each side's twelve
# missions won in turn through their score screens to the ending and the main menu (label rb-chain, about 7 minutes each).
foreach(rb_route forcewin forcelose)
  add_test(NAME rb-native-${rb_route} COMMAND "${RB_PYTHON3}" -B "${RB_DIR}/tests/rb_routes.py" --route ${rb_route}
           --binary $<TARGET_FILE:rb-native-trial> --data-dir "${RB_DATA_DIR}" --game-exe "${RB_GAME_EXE}"
           --work "${CMAKE_CURRENT_BINARY_DIR}/tests/${rb_route}")
  set_tests_properties(rb-native-${rb_route} PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE TIMEOUT 600 LABELS rb-quick)
endforeach()
foreach(rb_side allied soviet)
  add_test(NAME rb-native-chain-${rb_side} COMMAND "${RB_PYTHON3}" -B "${RB_DIR}/tests/rb_routes.py" --route chain --side ${rb_side}
           --binary $<TARGET_FILE:rb-native-trial> --data-dir "${RB_DATA_DIR}" --game-exe "${RB_GAME_EXE}"
           --work "${CMAKE_CURRENT_BINARY_DIR}/tests/chain-${rb_side}")
  set_tests_properties(rb-native-chain-${rb_side} PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE TIMEOUT 1500 LABELS rb-chain)
endforeach()
# Lane P8: main's own feature tests (native/tests, read only) run unchanged against the trial, label rb-main. Their skirmish
# starts through tools/rb_skirmish_wrap.py, which stands in for the binary: it keeps their arguments and puts the menus'
# Single Player, Skirmish, Start Game clicks in front of their input script (main's --skirmish harness is not carried;
# glue/harness_input_native.inc answers their @ points and harness-state lines). The cases: Options (Main_Options_Dialog_Proc),
# Map zoom and Zoom gestures (steps, the in-play dialogs at the window's size with and without the render scale, the score
# screen and Abort at render scale 2, a pinch), the command bar's thumbs and the locked intro (N31), Queue five and the
# Modern/Classic controls, touch input (two fingers, A-Move, Modern, the team-button hold, the second-tank taps, Allied 2's
# taps across houses), HD vehicles 2x and 3x and the air base (lane P9: the wrapper's Patriot below the yard), Mac widescreen.
# Not added: enhancements_newgame.py (the page takes the dropdown's row under the click: the trial's rows are the original's,
# LineHeight + 6 from Combo_Proc's OD_SUBCLASSED, main's port keeps user32's 16, so main's click picks 1.25x, not 1.5x).
set(rb_main_dir "${RB_ROOT}/tests")
set(rb_wrap "${RB_DIR}/tools/rb_skirmish_wrap.py")
set(rb_main_cases
  "shell-options|shell_paths.py|--case,options,--work,@W@"
  "zoom-steps|zoom_input.py|--case,steps,--log,@W@.log"
  "zoom-menu|zoom_input.py|--case,menu,--log,@W@.log"
  "zoom-menu-ui|zoom_input.py|--case,menu-ui,--log,@W@.log"
  "zoom-score|zoom_input.py|--case,score,--log,@W@.log"
  "zoom-abort|zoom_input.py|--case,abort,--log,@W@.log"
  "zoom-pinch|zoom_input.py|--case,pinch,--log,@W@.log"
  "command-bar-thumb|command_bar_input.py|--case,thumb,--work,@W@"
  "locked-input|command_bar_input.py|--case,lock,--work,@W@"
  "controls-queue-five|controls_input.py|--case,queue-five,--log,@W@.log"
  "controls-queue-five-off|controls_input.py|--case,queue-five-off,--log,@W@.log"
  "controls-modern|controls_input.py|--case,modern,--log,@W@.log"
  "controls-classic|controls_input.py|--case,classic,--log,@W@.log"
  "touch-two-finger|touch_input.py|--case,two-finger,--log,@W@.log"
  "touch-attack-move|touch_input.py|--case,attack-move,--log,@W@.log"
  "touch-modern|touch_input.py|--case,modern,--log,@W@.log"
  "touch-team-hold|touch_input.py|--case,team-hold,--log,@W@.log"
  "touch-second-classic|touch_input.py|--case,second-classic,--log,@W@.log"
  "touch-second-modern|touch_input.py|--case,second-modern,--log,@W@.log"
  "touch-cross-classic|touch_input.py|--case,cross-classic,--log,@W@.log"
  "touch-cross-modern|touch_input.py|--case,cross-modern,--log,@W@.log"
  "touch-cross-mouse|touch_input.py|--case,cross-mouse,--log,@W@.log"
  "hd-vehicles|hd_vehicles.py|--log,@W@.log"
  "hd-vehicles-3x|hd_vehicles.py|--case,units,--scale,3,--budget-ms,0,--log,@W@.log"
  "hd-air-base|hd_vehicles.py|--case,air-base,--log,@W@.log"
  "widescreen|widescreen.py|--work,@W@")
foreach(rb_case IN LISTS rb_main_cases)
  string(REPLACE "|" ";" rb_parts "${rb_case}")
  list(GET rb_parts 0 rb_name)
  list(GET rb_parts 1 rb_script)
  list(GET rb_parts 2 rb_args)
  string(REPLACE "," ";" rb_args "${rb_args}")
  string(REPLACE "@W@" "${CMAKE_CURRENT_BINARY_DIR}/tests/main-${rb_name}" rb_args "${rb_args}")
  add_test(NAME rb-main-${rb_name} COMMAND "${RB_PYTHON3}" -B "${rb_main_dir}/${rb_script}" ${rb_args}
           --binary "${rb_wrap}" --data-dir "${RB_DATA_DIR}" --game-exe "${RB_GAME_EXE}")
  set_tests_properties(rb-main-${rb_name} PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE TIMEOUT 600 LABELS rb-main
                       ENVIRONMENT "RB_TRIAL=$<TARGET_FILE:rb-native-trial>")
endforeach()
# Lane P9: main's LAN pair (tests/lan_pair.py, C17w/C22): two trial instances play a Network game over UDP through the real
# menus for 18,000 frames beside a one-player skirmish; equal per-frame CRCs, the Sound dialog twice, the edge-scroll rate.
add_test(NAME rb-main-lan-pair COMMAND "${RB_PYTHON3}" -B "${rb_main_dir}/lan_pair.py" --binary "${rb_wrap}" --data-dir "${RB_DATA_DIR}"
         --game-exe "${RB_GAME_EXE}" --work "${CMAKE_CURRENT_BINARY_DIR}/tests/main-lan-pair")
set_tests_properties(rb-main-lan-pair PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE TIMEOUT 3600 LABELS rb-main
                     ENVIRONMENT "RB_TRIAL=$<TARGET_FILE:rb-native-trial>")
# Frame identity against redalert2-decomp's src-based port (label rb-identity): added only when RB_IDENTITY_SRC names
# a built src-based ra2-native (from that repository; not part of this one).
set(RB_IDENTITY_SRC "" CACHE FILEPATH "A src-based ra2-native from redalert2-decomp for the frame identity checks; empty: not added")
if(RB_IDENTITY_SRC)
# Lane P9: frame identity with the src-based port (label rb-identity; tests/rb_identity.py): Allied 1 and Soviet 1 from New
# Campaign, the same seed and orders in both binaries, main's soak digest (every techno's type, house, coordinate, strength and
# mission, every house's money and the random generator) compared every 50 frames through frame 5,000. Both were the same at
# every digest at lane P9's head. The skirmish route is not a row: its start through the menus is not repeatable from run to run
# yet, and the AI's defence placement (parked drafts here, exact in main) differs at frame 3,960.
foreach(rb_side allied soviet)
  add_test(NAME rb-identity-${rb_side} COMMAND "${RB_PYTHON3}" -B "${RB_DIR}/tests/rb_identity.py"
           --src "${RB_IDENTITY_SRC}" --trial $<TARGET_FILE:rb-native-trial> --data-dir "${RB_DATA_DIR}"
           --game-exe "${RB_GAME_EXE}" --work "${CMAKE_CURRENT_BINARY_DIR}/tests/identity-${rb_side}" --side ${rb_side}
           "--orders=+30 deploy *; +600/600 hunt" --frames 5000 --seconds 900)
  set_tests_properties(rb-identity-${rb_side} PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE TIMEOUT 2400 LABELS rb-identity)
endforeach()
# Lane P10: the skirmish route (Single Player, Skirmish, Start Game) with the sides and colours fixed in each run's RA2.INI
# (rb_identity.py: the human Americans, the computer Russians); without them Start Game drew the Random sides from a generator
# still seeded by GetTickCount, so the start was not repeatable. Identical through frame 5,000 at lane P10's head (one full run).
add_test(NAME rb-identity-skirmish COMMAND "${RB_PYTHON3}" -B "${RB_DIR}/tests/rb_identity.py"
         --src "${RB_IDENTITY_SRC}" --trial $<TARGET_FILE:rb-native-trial> --data-dir "${RB_DATA_DIR}"
         --game-exe "${RB_GAME_EXE}" --work "${CMAKE_CURRENT_BINARY_DIR}/tests/identity-skirmish" --route skirmish
         --frames 5000 --seconds 900)
set_tests_properties(rb-identity-skirmish PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE TIMEOUT 2400 LABELS rb-identity)
# Lane P11: the other side pairs P10 started (the human's house, the computer's house index: Arabs against Arabs, Russians
# against Americans, Africans against Confederation), each identical through frame 5,000 at lane P11's head.
foreach(rb_pair Arabs,6 Russians,0 Africans,7)
  string(REGEX REPLACE ",.*" "" rb_pair_name "${rb_pair}")
  add_test(NAME rb-identity-skirmish-${rb_pair_name} COMMAND "${RB_PYTHON3}" -B "${RB_DIR}/tests/rb_identity.py"
           --src "${RB_IDENTITY_SRC}" --trial $<TARGET_FILE:rb-native-trial> --data-dir "${RB_DATA_DIR}"
           --game-exe "${RB_GAME_EXE}" --work "${CMAKE_CURRENT_BINARY_DIR}/tests/identity-skirmish-${rb_pair_name}"
           --route skirmish --sides ${rb_pair} --frames 5000 --seconds 900)
  set_tests_properties(rb-identity-skirmish-${rb_pair_name} PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE TIMEOUT 2400
                       LABELS rb-identity)
endforeach()
# Lane P12: the shell screens of the two ports picture by picture (label rb-shell-frames; tests/rb_shell_frames.py): the
# main menu, Options, Enhancements, Keyboard, Network, Single Player, Load and the skirmish setup, at the Mac's default
# window and at iPhone 17 Pro landscape points with the touch controls; the allowed differences are listed in the test.
foreach(rb_size mac iphone)
  add_test(NAME rb-shell-frames-${rb_size} COMMAND "${RB_PYTHON3}" -B "${RB_DIR}/tests/rb_shell_frames.py"
           --src "${RB_IDENTITY_SRC}" --trial $<TARGET_FILE:rb-native-trial> --data-dir "${RB_DATA_DIR}"
           --game-exe "${RB_GAME_EXE}" --work "${CMAKE_CURRENT_BINARY_DIR}/tests/shell-frames-${rb_size}" --size ${rb_size})
  set_tests_properties(rb-shell-frames-${rb_size} PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE TIMEOUT 600
                       LABELS rb-shell-frames)
endforeach()
endif()
# Lane P7: the iOS Simulator checks (label rb-ios, opt in: RB_IOS_UDID names a Simulator device). The routes of
# tests/rb_routes.py run in RBTrial.app (ra2/tools/rb_ios.sh build simulator) through tools/rb_ios_sim.sh, which
# stands in for the Mac binary; a fixture boots the device if needed and installs the app first, and shuts the device
# down again after if it booted it. Skipped (77) without the app or the device.
set(RB_IOS_UDID "" CACHE STRING "A Simulator device (udid) for the rb-ios checks; empty: they are not added")
get_filename_component(RB_IOS_APP "${RB_ROOT}/build/ios-simulator/RBTrial.app" ABSOLUTE)
set(RB_IOS_APP "${RB_IOS_APP}" CACHE PATH "The Simulator RBTrial.app the rb-ios checks install")
if(NOT RB_IOS AND RB_IOS_UDID)
  add_test(NAME rb-ios-sim-setup COMMAND "${RB_DIR}/tools/rb_ios_sim.sh" --setup "${RB_IOS_UDID}" "${RB_IOS_APP}")
  set_tests_properties(rb-ios-sim-setup PROPERTIES FIXTURES_SETUP rb_ios_sim SKIP_RETURN_CODE 77 RUN_SERIAL TRUE TIMEOUT 300 LABELS rb-ios)
  add_test(NAME rb-ios-sim-cleanup COMMAND "${RB_DIR}/tools/rb_ios_sim.sh" --cleanup "${RB_IOS_UDID}")
  set_tests_properties(rb-ios-sim-cleanup PROPERTIES FIXTURES_CLEANUP rb_ios_sim RUN_SERIAL TRUE TIMEOUT 120 LABELS rb-ios)
  foreach(rb_ios_case skirmish production forcewin campaign:ALL02S.sav)
    string(REPLACE ":" ";" rb_ios_parts "${rb_ios_case}")
    list(GET rb_ios_parts 0 rb_route)
    set(rb_ios_args --route ${rb_route})
    set(rb_ios_name rb-ios-sim-${rb_route})
    list(LENGTH rb_ios_parts rb_ios_n)
    if(rb_ios_n GREATER 1)
      list(GET rb_ios_parts 1 rb_save)
      list(APPEND rb_ios_args --save ${rb_save})
      set(rb_ios_name rb-ios-sim-${rb_route}-${rb_save})
    endif()
    add_test(NAME ${rb_ios_name} COMMAND "${RB_PYTHON3}" -B "${RB_DIR}/tests/rb_routes.py" ${rb_ios_args}
             --binary "${RB_DIR}/tools/rb_ios_sim.sh" --data-dir "${RB_DATA_DIR}" --game-exe "${RB_GAME_EXE}"
             --work "${CMAKE_CURRENT_BINARY_DIR}/tests/ios-${rb_route}")
    set_tests_properties(${rb_ios_name} PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE TIMEOUT 600 LABELS rb-ios
                         FIXTURES_REQUIRED rb_ios_sim ENVIRONMENT "RB_IOS_UDID=${RB_IOS_UDID}")
  endforeach()
  # Lane P8: touch input in the Simulator app: main's touch tests (tests/touch_input.py) and the pinch zoom
  # (zoom_input.py pinch), through tools/rb_skirmish_wrap.py with RBTrial.app as its binary (RB_TRIAL=tools/rb_ios_sim.sh): the
  # scripted fingers go through the host's SDL touch path inside the iOS app.
  foreach(rb_touch_case IN ITEMS "touch-two-finger|touch_input.py|--case,two-finger" "touch-attack-move|touch_input.py|--case,attack-move"
                                 "touch-team-hold|touch_input.py|--case,team-hold" "touch-cross-classic|touch_input.py|--case,cross-classic"
                                 "zoom-pinch|zoom_input.py|--case,pinch")
    string(REPLACE "|" ";" rb_parts "${rb_touch_case}")
    list(GET rb_parts 0 rb_name)
    list(GET rb_parts 1 rb_script)
    list(GET rb_parts 2 rb_args)
    string(REPLACE "," ";" rb_args "${rb_args}")
    add_test(NAME rb-ios-sim-${rb_name} COMMAND "${RB_PYTHON3}" -B "${RB_ROOT}/tests/${rb_script}" ${rb_args}
             --binary "${RB_DIR}/tools/rb_skirmish_wrap.py" --data-dir "${RB_DATA_DIR}" --game-exe "${RB_GAME_EXE}"
             --log "${CMAKE_CURRENT_BINARY_DIR}/tests/ios-${rb_name}.log")
    set_tests_properties(rb-ios-sim-${rb_name} PROPERTIES SKIP_RETURN_CODE 77 RUN_SERIAL TRUE TIMEOUT 600 LABELS rb-ios
                         FIXTURES_REQUIRED rb_ios_sim
                         ENVIRONMENT "RB_IOS_UDID=${RB_IOS_UDID};RB_TRIAL=${RB_DIR}/tools/rb_ios_sim.sh")
  endforeach()
endif()
