cmake_minimum_required(VERSION 3.10)
project(GameControllerMIDI)

enable_testing()

set(DPF_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/DPF)
include(${DPF_ROOT_DIR}/cmake/DPF-plugin.cmake)

# Find SDL2, Boost and nlohmann-json via vcpkg
find_package(SDL2 REQUIRED)
find_package(Boost REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# ImGui implementation in DPF-Widgets
set(IMGUI_WIDGET_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/DPF-Widgets/opengl/DearImGui.cpp
)

# Include embedding functions
include(cmake/EmbedPresets.cmake)

dpf_add_plugin(GameControllerMIDI
    TARGETS vst2 vst3 clap 
    UI_TYPE opengl
    FILES_DSP
        src/Plugin.cpp
        src/Core/SdlManager.cpp
        src/Core/EventDispatcher.cpp
        src/Core/MidiEventQueue.cpp
        src/Logic/FlexibleMapper.cpp
        src/Logic/MapperSerialization.cpp
    FILES_UI
        src/UI.cpp
        ${IMGUI_WIDGET_SRC}
)

# Enable file browser support
target_compile_definitions(GameControllerMIDI PUBLIC DGL_USE_FILE_BROWSER=1)

# After target creation:
embed_all_presets(GameControllerMIDI "${CMAKE_CURRENT_SOURCE_DIR}/presets")

# Shared configuration (Propagates to both -dsp and -ui)
target_include_directories(GameControllerMIDI
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link SDL2 and Boost to the base target
target_link_libraries(GameControllerMIDI
    PUBLIC
        SDL2::SDL2-static
        Boost::boost
        nlohmann_json::nlohmann_json
)

# UI-specific include paths
target_include_directories(GameControllerMIDI-ui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/DPF-Widgets/opengl
)

add_executable(GameControllerMIDI_core_tests
    tests/core_tests.cpp
    src/Core/EventDispatcher.cpp
    src/Core/MidiEventQueue.cpp
)

target_compile_features(GameControllerMIDI_core_tests PRIVATE cxx_std_23)

target_include_directories(GameControllerMIDI_core_tests
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_link_libraries(GameControllerMIDI_core_tests
    PRIVATE
        SDL2::SDL2-static
        Boost::boost
)

add_test(NAME GameControllerMIDI_core_tests COMMAND GameControllerMIDI_core_tests)
