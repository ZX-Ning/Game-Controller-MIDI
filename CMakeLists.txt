cmake_minimum_required(VERSION 3.10)
project(GameControllerMIDI)

set(DPF_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/DPF)
include(${DPF_ROOT_DIR}/cmake/DPF-plugin.cmake)

# Find SDL2, Boost and nlohmann-json via vcpkg
find_package(SDL2 REQUIRED)
find_package(Boost REQUIRED)
find_package(nlohmann_json CONFIG REQUIRED)

set(CMAKE_CXX_STANDARD 23)

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
        src/Logic/FlexibleMapper.cpp
    FILES_UI
        src/UI.cpp
        ${IMGUI_WIDGET_SRC}
)

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
