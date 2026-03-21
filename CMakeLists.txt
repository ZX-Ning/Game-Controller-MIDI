cmake_minimum_required(VERSION 3.10)
project(GameControllerMIDI)

set(DPF_ROOT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/DPF)
include(${DPF_ROOT_DIR}/cmake/DPF-plugin.cmake)

# Find SDL2
find_package(PkgConfig REQUIRED)
pkg_check_modules(SDL2 REQUIRED sdl2)

# Boost headers
set(BOOST_LOCKFREE_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/third_party/lockfree/include)

# ImGui implementation in DPF-Widgets
set(IMGUI_WIDGET_SRC
    ${CMAKE_CURRENT_SOURCE_DIR}/third_party/DPF-Widgets/opengl/DearImGui.cpp
)

dpf_add_plugin(GameControllerMIDI
    TARGETS lv2 vst3 jack
    UI_TYPE opengl
    FILES_DSP
        src/Plugin.cpp
        src/Core/SdlManager.cpp
        src/Logic/MajorScaleMapper.cpp
    FILES_UI
        src/UI.cpp
        ${IMGUI_WIDGET_SRC}
)

# Crucial: Add the current directory and src directory to include paths for all targets
# DPF creates several targets, we need to make sure they all find DistrhoPluginInfo.h
target_include_directories(GameControllerMIDI
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${SDL2_INCLUDE_DIRS}
        ${BOOST_LOCKFREE_INCLUDE_DIR}
)

target_include_directories(GameControllerMIDI-dsp
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${SDL2_INCLUDE_DIRS}
        ${BOOST_LOCKFREE_INCLUDE_DIR}
)

target_include_directories(GameControllerMIDI-ui
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}
        ${CMAKE_CURRENT_SOURCE_DIR}/src
        ${CMAKE_CURRENT_SOURCE_DIR}/third_party/DPF-Widgets/opengl
        ${SDL2_INCLUDE_DIRS}
        ${BOOST_LOCKFREE_INCLUDE_DIR}
)

target_link_libraries(GameControllerMIDI-dsp PRIVATE ${SDL2_LIBRARIES})
target_link_libraries(GameControllerMIDI-ui PRIVATE ${SDL2_LIBRARIES})
