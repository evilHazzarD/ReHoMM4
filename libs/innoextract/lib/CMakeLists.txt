# innoextract 1.9 as a static library (recomp_innoextract.h), with the parts of
# Boost it needs compiled in: the in-app installer on iOS and Android, which
# cannot run a helper program. From HommaRe (kit/third_party/innoextract, MIT,
# LICENSE-HommaRe.txt); here configured by platforms/deps.sh, which fetches the
# sources and passes their folders:
#   INNOEXTRACT_SRC  innoextract-1.9 (patched)
#   BOOST_SRC        boost_1_89_0 (headers and libs/{filesystem,iostreams}/src)
#   LZMA_INCLUDE     XZ Utils' src/liblzma/api (liblzma itself is built apart)
#   BZIP2_SRC        bzip2-1.0.8 (optional: Android has no libbz2; iOS uses the system's)
# zlib and iconv are the system's.
cmake_minimum_required(VERSION 3.24)
project(recomp_innoextract LANGUAGES C CXX)

# C++20 for Boost.Filesystem's std::atomic_ref.
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(src ${INNOEXTRACT_SRC}/src)
set(innoextract_sources
  cli/extract.cpp cli/gog.cpp cli/goggalaxy.cpp
  crypto/adler32.cpp crypto/arc4.cpp crypto/checksum.cpp crypto/crc32.cpp crypto/hasher.cpp
  crypto/md5.cpp crypto/sha1.cpp
  loader/exereader.cpp loader/offsets.cpp
  setup/component.cpp setup/data.cpp setup/delete.cpp setup/directory.cpp setup/expression.cpp
  setup/file.cpp setup/filename.cpp setup/header.cpp setup/icon.cpp setup/info.cpp setup/ini.cpp
  setup/item.cpp setup/language.cpp setup/message.cpp setup/permission.cpp setup/registry.cpp
  setup/run.cpp setup/task.cpp setup/type.cpp setup/version.cpp setup/windows.cpp
  stream/block.cpp stream/chunk.cpp stream/file.cpp stream/lzma.cpp stream/slice.cpp
  util/console.cpp util/encoding.cpp util/load.cpp util/log.cpp util/time.cpp)
list(TRANSFORM innoextract_sources PREPEND ${src}/)

# Boost.Filesystem and the zlib and bzip2 filters of Boost.Iostreams; the
# rest innoextract uses (Date_Time, System, Range, ...) is header-only.
set(boost_sources
  ${BOOST_SRC}/libs/filesystem/src/codecvt_error_category.cpp
  ${BOOST_SRC}/libs/filesystem/src/directory.cpp
  ${BOOST_SRC}/libs/filesystem/src/exception.cpp
  ${BOOST_SRC}/libs/filesystem/src/operations.cpp
  ${BOOST_SRC}/libs/filesystem/src/path.cpp
  ${BOOST_SRC}/libs/filesystem/src/path_traits.cpp
  ${BOOST_SRC}/libs/filesystem/src/portability.cpp
  ${BOOST_SRC}/libs/filesystem/src/unique_path.cpp
  ${BOOST_SRC}/libs/filesystem/src/utf8_codecvt_facet.cpp
  ${BOOST_SRC}/libs/iostreams/src/bzip2.cpp
  ${BOOST_SRC}/libs/iostreams/src/file_descriptor.cpp
  ${BOOST_SRC}/libs/iostreams/src/zlib.cpp)

set(bzip2_sources)
if(BZIP2_SRC)
  foreach(f blocksort huffman crctable randtable compress decompress bzlib)
    list(APPEND bzip2_sources ${BZIP2_SRC}/${f}.c)
  endforeach()
endif()

add_library(recomp_innoextract STATIC
  ${CMAKE_CURRENT_SOURCE_DIR}/recomp_innoextract.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/release.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/process.cpp
  ${innoextract_sources}
  ${boost_sources}
  ${bzip2_sources})
if(BZIP2_SRC)
  target_include_directories(recomp_innoextract SYSTEM PRIVATE ${BZIP2_SRC})
endif()
target_include_directories(recomp_innoextract PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR} ${src})
target_include_directories(recomp_innoextract SYSTEM PRIVATE ${BOOST_SRC} ${LZMA_INCLUDE})
if(WIN32)
  # MinGW has no zlib development files in its base toolchain; platforms/deps.sh supplies it.
  target_include_directories(recomp_innoextract SYSTEM PRIVATE ${CMAKE_INSTALL_PREFIX}/include)
  target_sources(recomp_innoextract PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/windows_console.cpp
    ${BOOST_SRC}/libs/filesystem/src/windows_file_codecvt.cpp)
endif()
target_compile_definitions(recomp_innoextract PRIVATE
  # Boost built into this library, without auto-linking or DLL markup.
  BOOST_ALL_NO_LIB BOOST_FILESYSTEM_STATIC_LINK=1 BOOST_IOSTREAMS_STATIC_LINK=1
  BOOST_FILESYSTEM_NO_DEPRECATED LZMA_API_STATIC)
# Boost's own sources build the library (its headers then define the symbols
# its users call through versioned names).
set_source_files_properties(${boost_sources} PROPERTIES
  COMPILE_DEFINITIONS "BOOST_FILESYSTEM_SOURCE;BOOST_IOSTREAMS_SOURCE")
target_compile_options(recomp_innoextract PRIVATE -O2 -w -fvisibility=hidden -fvisibility-inlines-hidden)
if(NINTENDO_SWITCH OR CMAKE_SYSTEM_NAME STREQUAL "NintendoSwitch")
  target_compile_options(recomp_innoextract PRIVATE -include ${CMAKE_CURRENT_SOURCE_DIR}/switch_compat.h)
  # Boost does not know the platform: newlib's POSIX parts.
  target_compile_definitions(recomp_innoextract PRIVATE BOOST_HAS_GETTIMEOFDAY BOOST_HAS_UNISTD_H
    BOOST_DATE_TIME_HAS_HIGH_PRECISION_CLOCK BOOST_HAS_PTHREADS _GNU_SOURCE _DEFAULT_SOURCE)
  set_target_properties(recomp_innoextract PROPERTIES CXX_EXTENSIONS ON C_EXTENSIONS ON)
  target_include_directories(recomp_innoextract SYSTEM PRIVATE ${DEVKITPRO}/portlibs/switch/include)
endif()
# Only the C entry point is visible.
set_source_files_properties(${CMAKE_CURRENT_SOURCE_DIR}/recomp_innoextract.cpp PROPERTIES
  COMPILE_OPTIONS "-fvisibility=default")

install(TARGETS recomp_innoextract ARCHIVE DESTINATION lib)
install(FILES ${CMAKE_CURRENT_SOURCE_DIR}/recomp_innoextract.h DESTINATION include)
