# The in-app installer's core (h4/src/install) on the Mac, for testing it without a device:
#   platforms/tools/install_test/run.sh inno <setup.exe> <out dir>   unpack + check
#   platforms/tools/install_test/run.sh gog <out dir>                GOG's file list -> plan (no sign-in:
#                                                                  stops at the download address)
cmake_minimum_required(VERSION 3.25)
project(h4_install_test LANGUAGES C CXX OBJCXX)
set(CMAKE_CXX_STANDARD 20)
set(S ${CMAKE_CURRENT_SOURCE_DIR}/../../../h4/src)
add_executable(install_test main.cpp ${S}/install/json.cpp ${S}/install/hash.cpp
  ${S}/install/manifest.cpp ${S}/install/gog.cpp ${S}/install/inno.cpp ${S}/install/platform_apple.mm)
target_include_directories(install_test PRIVATE ${S} ${PREFIX}/include)
set_source_files_properties(${S}/install/platform_apple.mm PROPERTIES COMPILE_OPTIONS -fobjc-arc)
find_package(SDL3 REQUIRED)
target_link_libraries(install_test PRIVATE SDL3::SDL3 ${PREFIX}/lib/librecomp_innoextract.a
  ${PREFIX}/lib/liblzma.a z bz2 iconv "-framework Foundation" "-framework Security")
