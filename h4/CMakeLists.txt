# Contains code from HoMM4Deadware (https://github.com/LeoDeadware/HoMM4Deadware): Copyright (c) 2026 LeoDeadware, MIT License.
cmake_minimum_required(VERSION 3.25)
project(h4_recomp LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
# x86recomp's runtime and HLE modules are shared libraries: rebuilding one
# does not relink the game.
set(CMAKE_LINK_DEPENDS_NO_SHARED ON)

set(X86RECOMP_DIR "${CMAKE_CURRENT_SOURCE_DIR}/../third_party/x86recomp"
    CACHE PATH "x86recomp source tree")

enable_testing()
add_subdirectory("${X86RECOMP_DIR}" x86recomp)

# The game: runtime + HLE + lifted code (generated/, produced by recomp_lift).
recomp_add_lifted(h4_lifted "${CMAKE_CURRENT_SOURCE_DIR}/generated")
if(TARGET h4_lifted)
  option(H4_DEBUG_HOOKS "Dev instrumentation of lifted functions (needs the replace lines in lifter.manifest)" OFF)
  set(_h4_sources src/main.cpp src/hle_extra.cpp src/resolution.cpp src/smooth.cpp)
  if(H4_DEBUG_HOOKS)
    list(APPEND _h4_sources src/debug_hooks.cpp)
  endif()
  set(_h4_libs recomp_runtime recomp_mss32 recomp_dsound recomp_bink recomp_d3d7)
  # H4_DESKTOP_APP: the Linux (Steam Deck) and Mac app builds, made like the mobile ones
  # (platforms/build.sh linux|macos-*): one binary with the in-app installer, the translation and the
  # wings beside it.
  option(H4_DESKTOP_APP "Linux/Mac app build (platforms/build.sh linux|macos-*)" OFF)
  # H4_SWITCH_APP: Nintendo Switch homebrew (platforms/build.sh switch): heroes4.elf, packed into an
  # .nro by platforms/switch/package.sh; the soft gfx device (no SDL_GPU there).
  option(H4_SWITCH_APP "Nintendo Switch homebrew build (platforms/build.sh switch)" OFF)
  if(ANDROID OR IOS OR H4_DESKTOP_APP OR H4_SWITCH_APP)
    # The in-app installer (src/install/installer.h): GOG download or the offline installer,
    # unpacked by innoextract (platforms/deps.sh builds it into the prefix).
    list(APPEND _h4_sources src/install/installer.cpp src/install/gog.cpp src/install/inno.cpp
         src/install/manifest.cpp src/install/json.cpp src/install/hash.cpp
         src/language.cpp)
    find_library(H4_INNOEXTRACT recomp_innoextract REQUIRED)
    find_library(H4_LZMA lzma REQUIRED)
    find_path(H4_INNOEXTRACT_INCLUDE recomp_innoextract.h REQUIRED)
  endif()
  if(H4_DESKTOP_APP AND APPLE)
    # Packed into Heroes IV.app by platforms/macos/package.sh (SDL3 static, Metal).
    enable_language(OBJCXX)
    add_executable(h4 ${_h4_sources} src/install/platform_desktop.cpp src/menu_button_mac.mm
                   src/install/gog_login_mac.mm src/save_sync_apple.mm src/save_sync.cpp $<TARGET_OBJECTS:h4_lifted>)
    set_source_files_properties(src/menu_button_mac.mm src/install/gog_login_mac.mm src/save_sync_apple.mm
                                PROPERTIES COMPILE_OPTIONS -fobjc-arc)
    set_target_properties(h4 PROPERTIES OUTPUT_NAME heroes4)
    target_compile_definitions(h4 PRIVATE H4_DESKTOP_APP=1)
    target_link_libraries(h4 PRIVATE ${H4_INNOEXTRACT} ${H4_LZMA} z bz2 iconv "-framework AppKit" "-framework WebKit")
  elseif(H4_SWITCH_APP)
    add_executable(h4 ${_h4_sources} src/install/platform_switch.cpp src/menu_button_linux.cpp
                   $<TARGET_OBJECTS:h4_lifted>)
    set_target_properties(h4 PROPERTIES OUTPUT_NAME heroes4 SUFFIX ".elf")
    target_compile_definitions(h4 PRIVATE H4_SWITCH_APP=1)
    target_link_libraries(h4 PRIVATE ${H4_INNOEXTRACT} ${H4_LZMA} curl mbedtls mbedx509 mbedcrypto
                          bz2 z nx)
  elseif(H4_DESKTOP_APP)
    add_executable(h4 ${_h4_sources} src/install/platform_desktop.cpp src/menu_button_linux.cpp
                   src/save_sync.cpp $<TARGET_OBJECTS:h4_lifted>)
    set_target_properties(h4 PROPERTIES OUTPUT_NAME heroes4 BUILD_WITH_INSTALL_RPATH ON
                          INSTALL_RPATH "$ORIGIN/lib")
    target_compile_definitions(h4 PRIVATE H4_DESKTOP_APP=1 H4_LINUX_APP=1)
    target_link_libraries(h4 PRIVATE ${H4_INNOEXTRACT} ${H4_LZMA} z dl pthread)
    # The C++ runtime inside (the sysroot's is older than many systems', never newer).
    target_link_options(h4 PRIVATE -static-libstdc++ -static-libgcc)
  elseif(ANDROID)
    # SDLActivity loads libmain.so and calls its SDL_main (SDL_main.h).
    add_library(h4 SHARED ${_h4_sources} src/install/platform_android.cpp src/menu_button_android.cpp
                $<TARGET_OBJECTS:h4_lifted>)
    set_target_properties(h4 PROPERTIES OUTPUT_NAME main)
    target_link_libraries(h4 PRIVATE log android ${H4_INNOEXTRACT} ${H4_LZMA} z)
    # ffmpeg's arm64 assembly addresses its tables directly (adrp): with the static ffmpeg in a
    # shared library, its symbols must bind inside it.
    target_link_options(h4 PRIVATE "LINKER:-Bsymbolic")
  elseif(IOS)
    # A static app: runtime, HLE, SDL3 and ffmpeg all inside (platforms/README.md).
    enable_language(OBJCXX)
    list(APPEND _h4_sources src/ios_safe_area.mm src/install/platform_apple.mm src/menu_button_apple.mm
         src/save_sync_apple.mm)
    set_source_files_properties(src/install/platform_apple.mm src/menu_button_apple.mm src/save_sync_apple.mm
                                PROPERTIES COMPILE_OPTIONS -fobjc-arc)
    add_executable(h4 MACOSX_BUNDLE ${_h4_sources} $<TARGET_OBJECTS:h4_lifted>
                   "${CMAKE_CURRENT_SOURCE_DIR}/../platforms/ios/LaunchScreen.storyboard")
    set(H4_BUNDLE_ID "games.hazzard.heroes4" CACHE STRING
        "iOS bundle identifier (unique per Apple ID for a free account)")
    set(H4_TEAM_ID "" CACHE STRING "Apple development team ID (Xcode > Settings > Accounts)")
    # The app icon (assets/icon/icon.sh: the project's own shield, drawn at build time).
    set(H4_IOS_ASSETS "" CACHE PATH "Assets.xcassets holding AppIcon")
    # The side panels (platforms/wings.sh): <bundle>/wings/*.jpg.
    set(H4_IOS_WINGS "" CACHE PATH "folder of wing images")
    # The licenses (platforms/notices.py): <bundle>/THIRD_PARTY_NOTICES.txt.
    set(H4_IOS_NOTICES "" CACHE FILEPATH "THIRD_PARTY_NOTICES.txt")
    if(H4_IOS_NOTICES)
      target_sources(h4 PRIVATE "${H4_IOS_NOTICES}")
      set_source_files_properties("${H4_IOS_NOTICES}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
    endif()
    # The language packs that go in the app (platforms/languages.py): <bundle>/<code>.h4p.
    set(H4_IOS_PACKS_DIR "" CACHE PATH "folder of <code>.h4p")
    if(H4_IOS_PACKS_DIR)
      file(GLOB _h4_packs "${H4_IOS_PACKS_DIR}/*.h4p")
      if(_h4_packs)
        target_sources(h4 PRIVATE ${_h4_packs})
        set_source_files_properties(${_h4_packs} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
      endif()
    endif()
    if(H4_IOS_WINGS)
      file(GLOB _h4_wings "${H4_IOS_WINGS}/*.jpg" "${H4_IOS_WINGS}/index.txt")
      target_sources(h4 PRIVATE ${_h4_wings})
      set_source_files_properties(${_h4_wings} PROPERTIES MACOSX_PACKAGE_LOCATION Resources/wings)
    endif()
    if(H4_IOS_ASSETS)
      target_sources(h4 PRIVATE "${H4_IOS_ASSETS}")
      set_source_files_properties("${H4_IOS_ASSETS}" PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
      set_target_properties(h4 PROPERTIES XCODE_ATTRIBUTE_ASSETCATALOG_COMPILER_APPICON_NAME AppIcon)
    endif()
    set_target_properties(h4 PROPERTIES
      OUTPUT_NAME "Heroes IV"
      MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/../platforms/ios/Info.plist.in"
      MACOSX_BUNDLE_GUI_IDENTIFIER "${H4_BUNDLE_ID}"
      MACOSX_BUNDLE_BUNDLE_NAME "Heroes IV"
      MACOSX_BUNDLE_SHORT_VERSION_STRING "0.1"
      MACOSX_BUNDLE_BUNDLE_VERSION "1"
      RESOURCE "${CMAKE_CURRENT_SOURCE_DIR}/../platforms/ios/LaunchScreen.storyboard"
      XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "${H4_BUNDLE_ID}"
      XCODE_ATTRIBUTE_DEVELOPMENT_TEAM "${H4_TEAM_ID}"
      XCODE_ATTRIBUTE_CODE_SIGN_STYLE Automatic
      XCODE_ATTRIBUTE_CODE_SIGN_IDENTITY "Apple Development"
      XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY "1,2"
      XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_CURRENT_SOURCE_DIR}/../platforms/ios/h4.entitlements"
      XCODE_ATTRIBUTE_ENABLE_BITCODE NO)
    target_link_libraries(h4 PRIVATE "-framework UIKit" "-framework Metal" "-framework QuartzCore"
      "-framework AVFoundation" "-framework AudioToolbox" "-framework CoreAudio"
      "-framework CoreMotion" "-framework CoreHaptics" "-framework GameController"
      "-framework CoreBluetooth" "-framework CoreGraphics" "-framework Foundation"
      "-framework IOKit" "-framework UniformTypeIdentifiers" "-framework OpenGLES"
      "-framework CoreFoundation" "-framework Security" "-framework VideoToolbox"
      "-framework WebKit" ${H4_INNOEXTRACT} ${H4_LZMA} z bz2 iconv)
  else()
    add_executable(h4 ${_h4_sources} $<TARGET_OBJECTS:h4_lifted>)
  endif()
  recomp_link_hle(h4 ${_h4_libs})
  if(ANDROID OR IOS OR H4_DESKTOP_APP OR H4_SWITCH_APP)
    target_include_directories(h4 PRIVATE src ${H4_INNOEXTRACT_INCLUDE})
    # The translations (platforms/languages.py): the list the app starts with, and where the
    # latest release's list is (checked at every start).
    set(H4_LANGUAGES_INC "" CACHE FILEPATH "languages.inc")
    set(H4_TRANSLATIONS_URL "" CACHE STRING "the translations' languages.json")
    if(H4_TRANSLATIONS_URL)
      target_compile_definitions(h4 PRIVATE H4_TRANSLATIONS_URL="${H4_TRANSLATIONS_URL}")
    endif()
    if(H4_LANGUAGES_INC)
      target_compile_definitions(h4 PRIVATE H4_LANGUAGES_INC="${H4_LANGUAGES_INC}")
      set_property(SOURCE src/language.cpp APPEND PROPERTY OBJECT_DEPENDS "${H4_LANGUAGES_INC}")
    endif()
  endif()
  # Our HLE handlers (hle_extra.cpp, ...) override the runtime's in any link order.
  target_compile_definitions(h4 PRIVATE RECOMP_HLE_PRIORITY=1)
  set_target_properties(h4 PROPERTIES ENABLE_EXPORTS ON)  # symbol names in fault backtraces
endif()
