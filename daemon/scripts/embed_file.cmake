# Writes a C++ header holding a file's exact bytes, so the daemon can serve the file without
# finding it on disk. A byte array rather than a string literal, because MSVC limits string
# literals to 64 KiB.
#
#   cmake -DINPUT=<file> -DOUTPUT=<header> -DNAME=<identifier> -P embed_file.cmake
#
# The header declares holder::generated::<NAME>(), returning the bytes as a std::string_view.

foreach(variable INPUT OUTPUT NAME)
  if(NOT DEFINED ${variable})
    message(FATAL_ERROR "embed_file.cmake needs -D${variable}=...")
  endif()
endforeach()

file(READ "${INPUT}" hex HEX)
string(LENGTH "${hex}" hex_length)
math(EXPR size "${hex_length} / 2")
# Sixteen bytes per line keeps the generated header readable in a debugger.
string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," bytes "${hex}")
string(REGEX REPLACE "((0x[0-9a-f][0-9a-f],){16})" "\\1\n  " bytes "${bytes}")

file(WRITE "${OUTPUT}.tmp"
  "// Generated at build time from ${INPUT}; do not edit.\n"
  "#pragma once\n"
  "#include <string_view>\n"
  "namespace holder::generated {\n"
  "inline constexpr unsigned char ${NAME}_bytes[] = {\n  ${bytes}0x00};\n"
  "inline std::string_view ${NAME}() {\n"
  "  return {reinterpret_cast<const char*>(${NAME}_bytes), ${size}};\n"
  "}\n"
  "} // namespace holder::generated\n")
# Only replace the header when its content changes, so unchanged builds stay up to date.
file(COPY_FILE "${OUTPUT}.tmp" "${OUTPUT}" ONLY_IF_DIFFERENT)
file(REMOVE "${OUTPUT}.tmp")
