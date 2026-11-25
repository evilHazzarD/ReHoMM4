# The app's own fonts (h4/fonts/*.ttf, src/app_ui/text.h), built into the app as ui_fonts.inc (a
# byte array per file and the table of them).
#
#   h4_ui_fonts(<target>)   app_ui/text.cpp of <target> includes it (from the build folder)
function(h4_ui_fonts target)
  # stb_truetype draws them: h4/third_party/stb (not in the repository), downloaded once, the
  # pinned upstream file checked by its SHA-256.
  set(stb "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../third_party/stb")
  if(NOT EXISTS "${stb}/stb_truetype.h")
    file(DOWNLOAD
      "https://raw.githubusercontent.com/nothings/stb/9f755b604db3492df2cbeb39bf84b81e3e3932e3/stb_truetype.h"
      "${stb}/stb_truetype.h"
      EXPECTED_HASH SHA256=428cfe93520f2aed5e413a220b0db5bba5b17e2ebe5a64e4a8efa9eeeb67ec5f
      TLS_VERIFY ON)
  endif()
  target_include_directories(${target} PRIVATE "${stb}")
  set(dir "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/../fonts")
  file(GLOB files CONFIGURE_DEPENDS "${dir}/*.ttf")
  list(SORT files)
  if(NOT files)
    message(FATAL_ERROR "no ${dir}/*.ttf")
  endif()
  set_property(DIRECTORY APPEND PROPERTY CMAKE_CONFIGURE_DEPENDS ${files})
  set(arrays "")
  set(table "")
  foreach(f IN LISTS files)
    get_filename_component(name "${f}" NAME_WE)
    string(MAKE_C_IDENTIFIER "${name}" id)
    file(READ "${f}" hex HEX)
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," hex "${hex}")
    string(APPEND arrays "static const unsigned char kUiFont_${id}[] = {${hex}};\n")
    string(APPEND table "    {\"${name}\", kUiFont_${id}, sizeof kUiFont_${id}},\n")
  endforeach()
  set(inc "${CMAKE_CURRENT_BINARY_DIR}/ui_fonts.inc")
  set(text "// Made by h4/cmake/ui_fonts.cmake from h4/fonts/*.ttf: do not edit.\n${arrays}\nconst UiFontFile kUiFontFiles[] = {\n${table}};\n")
  file(CONFIGURE OUTPUT "${inc}" CONTENT "${text}" @ONLY)  # (rewritten only when it changes)
  target_include_directories(${target} PRIVATE "${CMAKE_CURRENT_BINARY_DIR}")
  get_target_property(sources ${target} SOURCES)
  foreach(s IN LISTS sources)
    if(s MATCHES "app_ui/text\\.cpp$")
      set_property(SOURCE "${s}" TARGET_DIRECTORY ${target} APPEND PROPERTY OBJECT_DEPENDS "${inc}")
    endif()
  endforeach()
endfunction()
