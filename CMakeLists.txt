cmake_minimum_required(VERSION 3.7)
project(DPFImGuiMinimal)

set(DPF_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/DPF)
include(${DPF_ROOT_DIR}/cmake/DPF-plugin.cmake)

# ImGui implementation in DPF-Widgets
set(IMGUI_WIDGET_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/DPF-Widgets/opengl/DearImGui.cpp
)

dpf_add_plugin(DPFImGuiMinimal
    TARGETS lv2 vst3 jack
    UI_TYPE opengl
    FILES_DSP
        src/Plugin.cpp
    FILES_UI
        src/UI.cpp
        ${IMGUI_WIDGET_SRC}
)

# Crucial: Add the current directory and src directory to include paths for all targets
# DPF creates several targets, we need to make sure they all find DistrhoPluginInfo.h
target_include_directories(DPFImGuiMinimal
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_include_directories(DPFImGuiMinimal-dsp
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_include_directories(DPFImGuiMinimal-ui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/DPF-Widgets/opengl
)
