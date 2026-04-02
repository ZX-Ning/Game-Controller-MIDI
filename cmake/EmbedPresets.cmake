# EmbedPresets.cmake - Functions to embed preset files into the binary

# Function to embed a single preset file
function(embed_preset TARGET INPUT_FILE VAR_NAME)
    get_filename_component(INPUT_FILENAME "${INPUT_FILE}" NAME)
    set(OUTPUT_FILE "${CMAKE_CURRENT_BINARY_DIR}/generated/${VAR_NAME}.hpp")
    
    # Ensure output directory exists
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")
    
    add_custom_command(
        OUTPUT "${OUTPUT_FILE}"
        COMMAND ${CMAKE_COMMAND}
            "-DINPUT_FILE=${INPUT_FILE}"
            "-DOUTPUT_FILE=${OUTPUT_FILE}"
            "-DVAR_NAME=${VAR_NAME}"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/bin2c.cmake"
        DEPENDS "${INPUT_FILE}" "${CMAKE_CURRENT_SOURCE_DIR}/cmake/bin2c.cmake"
        COMMENT "Embedding ${INPUT_FILENAME} as ${VAR_NAME}"
        VERBATIM
    )
    
    # Add to target sources
    target_sources(${TARGET} PRIVATE "${OUTPUT_FILE}")
    target_include_directories(${TARGET} PUBLIC "${CMAKE_CURRENT_BINARY_DIR}/generated")
endfunction()

# Function to embed all presets in a directory
function(embed_all_presets TARGET PRESET_DIR)
    file(GLOB PRESET_FILES "${PRESET_DIR}/*.json")
    foreach(PRESET_FILE ${PRESET_FILES})
        get_filename_component(PRESET_NAME "${PRESET_FILE}" NAME_WE)
        string(MAKE_C_IDENTIFIER "${PRESET_NAME}" VAR_NAME)
        set(VAR_NAME "preset_${VAR_NAME}")
        embed_preset(${TARGET} "${PRESET_FILE}" "${VAR_NAME}")
    endforeach()
endfunction()
