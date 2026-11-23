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

recomp_add_lifted(h4_lifted "${H4_ROOT}/h4/generated")
if(NOT TARGET h4_lifted)
  message(FATAL_ERROR "no lifted game in h4/generated: run tools/build.sh (or tools/lift.sh) first")
endif()
# Tail calls between lifted functions (LIFT_MUSTTAIL) need wasm's tail-call instructions.
target_compile_options(h4_lifted PRIVATE -mtail-call)

find_library(H4_INNOEXTRACT recomp_innoextract REQUIRED)
find_library(H4_LZMA lzma REQUIRED)
find_path(H4_INNOEXTRACT_INCLUDE recomp_innoextract.h REQUIRED)

add_executable(h4
  ${H4_SRC}/main.cpp ${H4_SRC}/hle_extra.cpp ${H4_SRC}/resolution.cpp ${H4_SRC}/smooth.cpp
  ${H4_SRC}/install/installer.cpp ${H4_SRC}/install/gog.cpp ${H4_SRC}/install/inno.cpp
  ${H4_SRC}/install/manifest.cpp ${H4_SRC}/install/json.cpp ${H4_SRC}/install/hash.cpp
  ${H4_SRC}/language.cpp ${H4_SRC}/intro.cpp ${H4_SRC}/window_mode.cpp ${H4_SRC}/pad_config.cpp
  ${H4_SRC}/menu_button_linux.cpp
  src/web_main.cpp src/web_platform.cpp
  $<TARGET_OBJECTS:h4_lifted>)
set_target_properties(h4 PROPERTIES OUTPUT_NAME heroes4 SUFFIX ".js")
target_include_directories(h4 PRIVATE ${H4_SRC} src ${H4_INNOEXTRACT_INCLUDE})
target_compile_definitions(h4 PRIVATE H4_DESKTOP_APP=1 H4_LINUX_APP=1 H4_WEB_APP=1
  RECOMP_HLE_PRIORITY=1 H4_VERSION_STRING="${H4_VERSION}")
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
target_link_options(h4 PRIVATE ${_h4_link})
set_property(TARGET h4 APPEND PROPERTY LINK_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/src/web_pre.js"
  "${CMAKE_CURRENT_SOURCE_DIR}/src/web_fetchfs.js")
