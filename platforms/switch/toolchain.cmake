# Nintendo Switch homebrew (platforms/build.sh switch): devkitPro's toolchain
# (devkitA64, libnx; /opt/devkitpro) with this project's dependencies
# (H4_SWITCH_PREFIX: SDL3, ffmpeg, innoextract) searched as well.
set(DEVKITPRO $ENV{DEVKITPRO})
if(NOT DEVKITPRO)
  set(DEVKITPRO /opt/devkitpro)
endif()
include(${DEVKITPRO}/cmake/Switch.cmake)
if(DEFINED ENV{H4_SWITCH_PREFIX})
  list(PREPEND CMAKE_PREFIX_PATH $ENV{H4_SWITCH_PREFIX})
  list(PREPEND CMAKE_FIND_ROOT_PATH $ENV{H4_SWITCH_PREFIX})
endif()
# Whole-archive linking (RECOMP_STATIC_APP's $<LINK_LIBRARY:WHOLE_ARCHIVE,...>): GNU ld's, which
# the generic platform does not declare.
set(CMAKE_LINK_LIBRARY_USING_WHOLE_ARCHIVE
    "LINKER:--whole-archive" "<LINK_ITEM>" "LINKER:--no-whole-archive")
set(CMAKE_LINK_LIBRARY_USING_WHOLE_ARCHIVE_SUPPORTED TRUE)
set(CMAKE_LINK_LIBRARY_WHOLE_ARCHIVE_ATTRIBUTES LIBRARY_TYPE=STATIC DEDUPLICATION=YES OVERRIDE=DEFAULT)
# devkitPro's portlibs (curl, zlib, bzip2, EGL/GLES) for every target. In the cache: devkitPro's
# Platform/Generic-dkP.cmake caches libnx/include alone under these names, and on a first
# configure that hides a plain variable (zlib.h then not found until cmake runs again).
foreach(_lang C CXX ASM)
  set(CMAKE_${_lang}_STANDARD_INCLUDE_DIRECTORIES
      "${DEVKITPRO}/portlibs/switch/include;${DEVKITPRO}/libnx/include" CACHE STRING "" FORCE)
endforeach()
