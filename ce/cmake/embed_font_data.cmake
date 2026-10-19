# Build-time conversion of the canonical TTF files to a C++ translation unit.
if(NOT DEFINED FONT_DIRECTORY OR NOT DEFINED OUTPUT)
    message(FATAL_ERROR "FONT_DIRECTORY and OUTPUT are required")
endif()

get_filename_component(output_directory "${OUTPUT}" DIRECTORY)
file(MAKE_DIRECTORY "${output_directory}")
file(WRITE "${OUTPUT}" "// Generated from TTF files. Do not edit.\n#include <cstddef>\nnamespace fallout { namespace scene_renderer { namespace font_data {\n")

function(append_font symbol filename)
    file(READ "${FONT_DIRECTORY}/${filename}" hex HEX)
    string(REGEX REPLACE "(..)" "0x\\1," values "${hex}")
    string(REPEAT "0x[0-9a-f][0-9a-f]," 16 row_pattern)
    string(REGEX REPLACE "(${row_pattern})" "\\1\n" values "${values}")
    file(APPEND "${OUTPUT}" "extern const unsigned char ${symbol}[] = {\n${values}\n};\nextern const std::size_t ${symbol}Size = sizeof(${symbol});\n")
endfunction()

append_font(kSemiBold "IBMPlexMono-SemiBold.ttf")
append_font(kBold "IBMPlexMono-Bold.ttf")
file(APPEND "${OUTPUT}" "} } }\n")
