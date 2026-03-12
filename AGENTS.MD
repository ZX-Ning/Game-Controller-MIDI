# AGENTS.MD - Project Structure and Development Guide

This document provides a technical overview of the `dpf_imgui` project, designed to assist both human engineers and AI agents in understanding and extending this minimal audio plugin framework.

## Project Overview

This project is a minimal audio plugin template built using the **DISTRHO Plugin Framework (DPF)** and **Dear ImGui**. It demonstrates how to integrate a modern, immediate-mode GUI into a cross-platform audio plugin.

## Directory Structure

```text
.
├── CMakeLists.txt              # Main build configuration
├── AGENTS.MD                   # This guide
├── src/                        # Plugin source code
│   ├── DistrhoPluginInfo.h     # Metadata (Name, URI, I/O count, etc.)
│   ├── Plugin.cpp              # Audio processing logic (DSP)
│   └── UI.cpp                  # Graphical User Interface logic (ImGui)
└── third_party/                # Dependencies (Git Submodules)
    ├── DPF/                    # Core DISTRHO Plugin Framework
    └── DPF-Widgets/            # DPF UI extensions including ImGui integration
```

## Key Components

### 1. Metadata (`src/DistrhoPluginInfo.h`)
Defines the plugin's identity and capabilities.
- `DISTRHO_PLUGIN_NAME`: Human-readable name.
- `DISTRHO_PLUGIN_URI`: Unique identifier for the plugin.
- `DISTRHO_PLUGIN_HAS_UI`: Set to `1` for ImGui support.
- `DISTRHO_UI_USE_OPENGL`: Set to `1` as ImGui requires an OpenGL backend in this setup.

### 2. DSP Logic (`src/Plugin.cpp`)
Inherits from `DISTRHO::Plugin`.
- `initParameter()`: Define parameters (ranges, defaults, symbols).
- `run()`: The audio callback. This is where the signal processing happens.
- `setParameterValue()` / `getParameterValue()`: Syncs internal state with the host/UI.

### 3. UI Logic (`src/UI.cpp`)
Inherits from `DISTRHO::UI` and `DGL_NAMESPACE::ImGuiTopLevelWidget`.
- **Constructor**: Sets initial size and geometry constraints.
- `parameterChanged()`: Callback from the DSP side (e.g., automation).
- `onImGuiDisplay()`: **CRITICAL.** This is the immediate-mode draw loop. Use `ImGui::*` calls here.
- `setParameterValue()`: Call this to send data from UI back to DSP.

### 4. Build System (`CMakeLists.txt`)
Uses DPF's CMake macros (`dpf_add_plugin`).
- It compiles `third_party/DPF-Widgets/opengl/DearImGui.cpp` into the UI target.
- Important: It maps include paths for `src` and `DPF-Widgets/opengl` to ensure headers are found by all sub-targets.

## Extension Guide for Agents

### Adding a New Parameter
1.  **Info**: Update `kParamCount` in `Plugin.cpp` (or an enum).
2.  **Plugin**: Initialize it in `initParameter()`.
3.  **Plugin**: Add a member variable to store the value and handle it in `run()`.
4.  **UI**: Add a corresponding `ImGui` widget in `onImGuiDisplay()` (e.g., `ImGui::SliderFloat`).
5.  **UI**: Ensure `parameterChanged()` handles the new index to keep the UI in sync with automation.

### Advanced ImGui Features
- DPF-Widgets includes `DearImGuiKnobs` and `DearImGuiToggle`.
- Check `third_party/DPF-Widgets/opengl/DearImGui.hpp` for available extensions.

## Building the Project

```bash
mkdir build && cd build
cmake ..
make -j$(nproc)
```

Binaries are generated in `build/bin/` in various formats (LV2, VST3, JACK).

## References
- [DPF Documentation](https://distrho.github.io/DPF/)
- [Dear ImGui Manual](https://pthom.github.io/imgui_manual_online/v1.89/imgui_manual.html)
- [Dear Plugins Samples](/home/zxning/code/audio/dear-plugins/)
