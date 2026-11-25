# Heroes IV in the browser: the same static recompilation as the apps (h4/CMakeLists.txt),
# built with Emscripten into one WebAssembly module (web/build.sh configures it).
#
# - The runtime is x86recomp with patches/ and web/patches/x86recomp-web.patch applied, in a
#   copy (X86RECOMP_DIR, made by build.sh): static, no SDL_GPU (the soft device on SDL's software
#   renderer, which draws into the page's canvas).
# - The game's own sources are h4/src as they are, the Linux app's (H4_DESKTOP_APP, H4_LINUX_APP),
#   but for what a browser does differently (web/src): the installer's platform layer,
#   the storage (OPFS), save sync and restarting.
cmake_minimum_required(VERSION 3.25)
project(h4_web LANGUAGES C CXX)
if(NOT EMSCRIPTEN)
  message(FATAL_ERROR "web is built with Emscripten: web/build.sh")
endif()

set(H4_ROOT "${CMAKE_CURRENT_SOURCE_DIR}/..")
set(H4_SRC "${H4_ROOT}/h4/src")
file(STRINGS "${H4_ROOT}/VERSION" H4_VERSION LIMIT_COUNT 1)
string(STRIP "${H4_VERSION}" H4_VERSION)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(X86RECOMP_DIR "" CACHE PATH "the patched x86recomp copy (web/build.sh)")
set(RECOMP_STATIC_APP ON CACHE BOOL "" FORCE)
set(RECOMP_GPU OFF CACHE BOOL "" FORCE)
add_subdirectory("${X86RECOMP_DIR}" x86recomp)

option(H4_WEB_DYNAMIC "Build the experimental worker-loaded web shell" OFF)
if(H4_WEB_DYNAMIC)
  find_program(H4_NODE node REQUIRED)
  find_program(H4_PATCH patch REQUIRED)
  set(H4_DISPATCH_PATCH "${CMAKE_CURRENT_SOURCE_DIR}/patches/x86recomp-dynamic-dispatch.patch")
  set(H4_DYNAMIC_DISPATCH "${CMAKE_CURRENT_BINARY_DIR}/dynamic_dispatch.cpp")
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${H4_DISPATCH_PATCH}")
  file(READ "${X86RECOMP_DIR}/src/runtime/dispatch.cpp" H4_DISPATCH_SOURCE)
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${X86RECOMP_DIR}/src/runtime/dispatch.cpp")
  file(WRITE "${H4_DYNAMIC_DISPATCH}" "${H4_DISPATCH_SOURCE}")
  execute_process(COMMAND "${H4_PATCH}" -s "${H4_DYNAMIC_DISPATCH}" "${H4_DISPATCH_PATCH}"
                  COMMAND_ERROR_IS_FATAL ANY)
  get_target_property(H4_RUNTIME_SOURCES recomp_runtime SOURCES)
  if(NOT "src/runtime/dispatch.cpp" IN_LIST H4_RUNTIME_SOURCES)
    message(FATAL_ERROR "x86recomp dispatcher source changed")
  endif()
  list(REMOVE_ITEM H4_RUNTIME_SOURCES "src/runtime/dispatch.cpp")
  set_property(TARGET recomp_runtime PROPERTY SOURCES "${H4_RUNTIME_SOURCES};${H4_DYNAMIC_DISPATCH}")

  set(H4_HOOK_GENERATOR "${CMAKE_CURRENT_SOURCE_DIR}/tools/generate_dynamic_hooks.mjs")
  set(H4_HOOKS "${CMAKE_CURRENT_BINARY_DIR}/dynamic_hooks.cpp")
  add_custom_command(OUTPUT "${H4_HOOKS}"
    COMMAND "${H4_NODE}" "${H4_HOOK_GENERATOR}" "${H4_ROOT}/h4/lifter.manifest" "${H4_HOOKS}"
    DEPENDS "${H4_HOOK_GENERATOR}" "${H4_ROOT}/h4/lifter.manifest" "${H4_ROOT}/converter/src/manifest.js"
    VERBATIM)
  set(H4_GAME_CODE "${H4_HOOKS}" "${X86RECOMP_DIR}/src/runtime/lifted_table.cpp")
else()
  recomp_add_lifted(h4_lifted "${H4_ROOT}/h4/generated")
  if(NOT TARGET h4_lifted)
    message(FATAL_ERROR "no lifted game in h4/generated: run tools/build.sh (or tools/lift.sh) first")
  endif()
  # Tail calls between lifted functions (LIFT_MUSTTAIL) need wasm's tail-call instructions.
  target_compile_options(h4_lifted PRIVATE -mtail-call)
  set(H4_GAME_CODE $<TARGET_OBJECTS:h4_lifted>)
endif()

find_library(H4_INNOEXTRACT recomp_innoextract REQUIRED)
find_library(H4_LZMA lzma REQUIRED)
find_path(H4_INNOEXTRACT_INCLUDE recomp_innoextract.h REQUIRED)

# The random map generator (h4/CMakeLists.txt): no fused multiply-add, the same maps everywhere.
file(GLOB H4_RANDOM_MAP ${H4_SRC}/random_map/*.cpp)
set_source_files_properties(${H4_RANDOM_MAP} PROPERTIES COMPILE_OPTIONS "-ffp-contract=off")
add_executable(h4
  ${H4_SRC}/main.cpp ${H4_SRC}/hle_extra.cpp ${H4_SRC}/resolution.cpp ${H4_SRC}/smooth.cpp
  ${H4_SRC}/install/installer.cpp ${H4_SRC}/install/gog.cpp ${H4_SRC}/install/inno.cpp
  ${H4_SRC}/install/manifest.cpp ${H4_SRC}/install/json.cpp ${H4_SRC}/install/hash.cpp
  ${H4_SRC}/language.cpp ${H4_SRC}/intro.cpp ${H4_SRC}/window_mode.cpp ${H4_SRC}/pad_config.cpp
  ${H4_SRC}/menu_button_drawn.cpp ${H4_SRC}/app_ui/text.cpp ${H4_SRC}/app_ui/ui.cpp ${H4_SRC}/random_map_ui.cpp ${H4_SRC}/filter_notice.cpp ${H4_SRC}/renderer.cpp ${H4_SRC}/ui_text.cpp ${H4_RANDOM_MAP}
  src/web_main.cpp src/web_platform.cpp src/dynamic_host.cpp
  ${H4_GAME_CODE})
set_target_properties(h4 PROPERTIES OUTPUT_NAME heroes4 SUFFIX ".js")
target_include_directories(h4 PRIVATE ${H4_SRC} src ${H4_INNOEXTRACT_INCLUDE})
include(${H4_ROOT}/h4/cmake/ui_text.cmake)  # the app's words (h4/src/ui_text.h)
h4_ui_text(h4)
include(${H4_ROOT}/h4/cmake/ui_fonts.cmake)  # the settings' own fonts (h4/src/app_ui/text.h)
h4_ui_fonts(h4)
target_include_directories(h4 PRIVATE ${H4_ROOT}/h4/third_party/stb)
target_compile_definitions(h4 PRIVATE H4_DESKTOP_APP=1 H4_LINUX_APP=1 H4_WEB_APP=1
  RECOMP_HLE_PRIORITY=1 H4_VERSION_STRING="${H4_VERSION}")
if(H4_WEB_DYNAMIC)
  target_compile_definitions(h4 PRIVATE H4_WEB_DYNAMIC=1)
endif()
# The game's main() and RestartApp (main.cpp) under other names: web_main.cpp's main() first
# mounts the browser's storage, and restarting is reloading the page.
set_source_files_properties(${H4_SRC}/main.cpp PROPERTIES COMPILE_DEFINITIONS
  "main=h4_game_main;RestartApp=RestartAppNative;SDL_MAIN_HANDLED=1")
set(H4_LANGUAGES_INC "" CACHE FILEPATH "languages.inc (platforms/languages.py)")
set(H4_TRANSLATIONS_URL "" CACHE STRING "the translations' languages.json")
if(H4_LANGUAGES_INC)
  target_compile_definitions(h4 PRIVATE H4_LANGUAGES_INC="${H4_LANGUAGES_INC}")
endif()
if(H4_TRANSLATIONS_URL)
  target_compile_definitions(h4 PRIVATE H4_TRANSLATIONS_URL="${H4_TRANSLATIONS_URL}")
endif()

recomp_link_hle(h4 recomp_runtime recomp_mss32 recomp_dsound recomp_bink recomp_d3d7)
target_link_libraries(h4 PRIVATE ${H4_INNOEXTRACT} ${H4_LZMA})

# What the module is (web/README.md, "How it works"):
# - pthreads, with main() on a worker (PROXY_TO_PTHREAD): the game's message loop blocks like
#   on Windows; the page's own thread only runs SDL's event and drawing calls proxied to it.
# - 4 GiB of memory at most, grown as used: the guest arena takes 2 GiB of it at start.
# - WasmFS with the origin private file system (OPFS) at /persist: the game files, saves and
#   settings stay in the browser (web_main.cpp).
# - native wasm exceptions: the runtime resumes SEH frames by throwing.
set(H4_RES_DIR "" CACHE PATH "files put in the module's file system at / (ui/, wings/)")
set(_h4_link
  -pthread -fwasm-exceptions -sPROXY_TO_PTHREAD=1 -sWASMFS=1 -sFETCH=1
  -sALLOW_MEMORY_GROWTH=1 -sINITIAL_MEMORY=256MB -sMAXIMUM_MEMORY=4GB
  -sSTACK_SIZE=16MB -sDEFAULT_PTHREAD_STACK_SIZE=4MB -sPTHREAD_POOL_SIZE=8
  -sUSE_ZLIB=1 -sEXIT_RUNTIME=1 -sENVIRONMENT=web,worker
  -sEXPORTED_RUNTIME_METHODS=callMain,FS -sEXPORT_NAME=createHeroes4 -sMODULARIZE=1
  -sINVOKE_RUN=1 -sABORT_ON_WASM_EXCEPTIONS=0 -sERROR_ON_UNDEFINED_SYMBOLS=1
  -sOFFSCREENCANVAS_SUPPORT=0 -sASSERTIONS=0
  # SDL's canvas code (its framebuffer for the software renderer) uses Emscripten's Browser
  # library from JS, which nothing in C refers to: without it every frame's update threw on the
  # page's thread and the game, waiting for it, hung.
  "-sDEFAULT_LIBRARY_FUNCS_TO_INCLUDE=$Browser"
  # Function names in the module: stack traces in the browser's console and crash logs say where.
  --profiling-funcs
  "--pre-js=${CMAKE_CURRENT_SOURCE_DIR}/src/web_pre.js"
  # WasmFS's fetch backend (H4_WEB_STREAM), its JS side fixed (web_fetchfs.js).
  "--js-library=${CMAKE_CURRENT_SOURCE_DIR}/src/web_fetchfs.js"
  # The console stays where the game's log goes (main.cpp sends it to a file otherwise).
  -Wl,--wrap=dup2
  # Folders renamed in the browser's storage, which cannot (web_main.cpp).
  -Wl,--wrap=rename
  # The game's quitting told to the page (web_main.cpp).
  -Wl,--wrap=_Exit -Wl,--wrap=_exit)
if(H4_RES_DIR)
  list(APPEND _h4_link "--preload-file=${H4_RES_DIR}@/")
endif()
if(H4_WEB_DYNAMIC)
  set(H4_DYNAMIC_LOADER "${CMAKE_CURRENT_SOURCE_DIR}/src/dynamic_loader.mjs")
  set(H4_DYNAMIC_WORKER "${CMAKE_CURRENT_SOURCE_DIR}/src/dynamic_worker.js")
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS "${H4_DYNAMIC_LOADER}")
  file(READ "${H4_DYNAMIC_LOADER}" H4_LOADER_JS)
  string(REGEX REPLACE "(^|\n)export " "\\1" H4_LOADER_JS "${H4_LOADER_JS}")
  set(H4_LOADER_PRE_JS "${CMAKE_CURRENT_BINARY_DIR}/dynamic_loader.js")
  file(CONFIGURE OUTPUT "${H4_LOADER_PRE_JS}" CONTENT "${H4_LOADER_JS}" @ONLY)
  list(APPEND _h4_link "--pre-js=${H4_LOADER_PRE_JS}" "--pre-js=${H4_DYNAMIC_WORKER}")
  set_property(TARGET h4 APPEND PROPERTY LINK_DEPENDS "${H4_LOADER_PRE_JS}" "${H4_DYNAMIC_WORKER}")
endif()
target_link_options(h4 PRIVATE ${_h4_link})
set_property(TARGET h4 APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/web_pre.js"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/web_fetchfs.js")
