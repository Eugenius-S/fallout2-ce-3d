# Shared by the game and the standalone font test project.
function(fallout_embed_font_data target)
    get_filename_component(ce_root "${CMAKE_CURRENT_FUNCTION_LIST_DIR}/.." ABSOLUTE)
    set(font_directory "${ce_root}/assets/fonts/ibm-plex-mono")
    set(data_source "${CMAKE_CURRENT_BINARY_DIR}/generated/${target}/embedded_font_data.cc")
    add_custom_command(
        OUTPUT "${data_source}"
        COMMAND "${CMAKE_COMMAND}" "-DFONT_DIRECTORY=${font_directory}" "-DOUTPUT=${data_source}"
            -P "${ce_root}/cmake/embed_font_data.cmake"
        DEPENDS "${font_directory}/IBMPlexMono-SemiBold.ttf" "${font_directory}/IBMPlexMono-Bold.ttf"
            "${ce_root}/cmake/embed_font_data.cmake"
        VERBATIM
    )
    target_sources(${target} PRIVATE "${data_source}" "${ce_root}/third_party/stb/stb_truetype.cc")
    target_include_directories(${target} PRIVATE "${ce_root}/third_party/stb")
endfunction()
