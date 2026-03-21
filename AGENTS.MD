# AGENTS.MD - Project Structure and Development Guide

This document provides a technical overview of the `dpf_imgui` project, designed to assist both human engineers and AI agents in understanding and extending this minimal audio plugin framework.

## Project Overview

This project is a minimal audio plugin template built using the **DISTRHO Plugin Framework (DPF)** and **Dear ImGui**. It demonstrates how to integrate a modern, immediate-mode GUI into a cross-platform audio plugin, including parameter automation and state persistence.

## Directory Structure

```text
.
├── CMakeLists.txt              # Main build configuration
├── AGENTS.MD                   # This guide
├── src/                        # Plugin source code
│   ├── DistrhoPluginInfo.h     # Metadata (Name, URI, I/O count, Custom UI defines)
│   ├── Plugin.cpp              # Audio processing logic (DSP) and State management
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
- `DISTRHO_PLUGIN_HAS_UI`: Set to `1`.
- `DISTRHO_PLUGIN_WANT_STATE`: Set to `1` to enable custom state keys.
- `DISTRHO_PLUGIN_WANT_FULL_STATE`: Set to `1` to enable `getState()` serialization.
- `DISTRHO_UI_USE_CUSTOM`: Set to `1` to use DPF-Widgets integration.
- `DISTRHO_UI_CUSTOM_INCLUDE_PATH`: Set to `"DearImGui.hpp"`.
- `DISTRHO_UI_CUSTOM_WIDGET_TYPE`: Set to `DGL_NAMESPACE::ImGuiTopLevelWidget`.

### 2. DSP Logic and State (`src/Plugin.cpp`)

Inherits from `DISTRHO::Plugin`.

- `initParameter()`: Define parameters (ranges, defaults, symbols).
- `initState()`: Define state keys. **CRITICAL**: Must set `state.hints = kStateIsHostReadable | kStateIsHostWritable` for persistence.
- `run()`: The audio callback for signal processing.
- `getState()` / `setState()`: Handle serialization/deserialization of non-parameter data (e.g., strings).
- `setParameterValue()` / `getParameterValue()`: Syncs parameters with the host.

### 3. UI Logic (`src/UI.cpp`)

Inherits from `DISTRHO::UI`.

- **Inheritance**: Due to `DISTRHO_UI_USE_CUSTOM`, the `UI` class already inherits from `ImGuiTopLevelWidget`.
- `parameterChanged()`: Callback for parameter updates.
- `stateChanged()`: Callback for state updates from the host or DSP.
- `onImGuiDisplay()`: The immediate-mode draw loop. Use `ImGui::*` calls.
- **State Updates**: Use `setState("key", value)` to notify the host of changes.
- **UX Tip**: Use `ImGui::IsItemDeactivatedAfterEdit()` to commit state changes on Enter or Focus loss.

### 4. Build System (`CMakeLists.txt`)

Uses DPF's CMake macros (`dpf_add_plugin`).

- Links `third_party/DPF-Widgets/opengl/DearImGui.cpp`.
- Configures include paths for `src` and `third_party/DPF-Widgets/opengl`.

## Extension Guide for Agents

### Adding a New Parameter

1.  Update `kParamCount` enum in `Plugin.cpp`.
2.  Initialize in `initParameter()`.
3.  Add a member variable and handle logic in `run()`.
4.  Add a widget in `UI::onImGuiDisplay()` (e.g., `ImGui::SliderFloat`).

### Adding a New State Key

1.  Increment state count in `Plugin` constructor.
2.  Initialize key and hints in `initState()`.
3.  Update logic in `getState()` and `setState()`.
4.  Handle UI synchronization in `UI::stateChanged()`.

### Reference
Referece code: [dear-plugins](https://github.com/DISTRHO/dear-plugins)

