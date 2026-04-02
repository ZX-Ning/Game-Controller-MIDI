# Project Structure and Development Guide

This document provides a technical overview of the `GameControllerMIDI` project, designed to assist both human engineers and AI agents in understanding and extending this audio plugin framework.

## NOTES FOR AI AGENT
**Follow the best practice of modern C++ (C++ 23) and real time audio application, and ensure high readability and maintainability of the codebase.**

## Project Overview

This project is an audio plugin built using the **DISTRHO Plugin Framework (DPF)**, **Dear ImGui**, and **SDL2**. It translates game controller inputs into MIDI messages using a decoupled, highly extendable architecture.

## Directory Structure

```text
.
├── CMakeLists.txt              # Main build configuration (uses vcpkg)
├── vcpkg.json                  # Dependency manifest (SDL2, Boost, nlohmann-json)
├── .vscode/settings.json       # VS Code CMake/vcpkg integration
├── AGENTS.md                   # This guide
├── LICENSE.md                  # MIT License
├── cmake/                      # CMake modules
│   ├── bin2c.cmake             # Binary-to-C array converter script
│   └── EmbedPresets.cmake      # Functions to embed JSON presets
├── presets/                    # JSON preset files
│   └── c_major_chords.json     # Example: C Major chord mappings
├── src/                        # Plugin source code
│   ├── Common/
│   │   └── MidiTypes.hpp       # Shared types (RawMidi)
│   ├── Core/
│   │   ├── IControllerEventHandler.hpp # Interface connecting SDL events to the dispatcher
│   │   ├── SdlManager.hpp      # SDL Lifecycle & Hardware Polling (Singleton)
│   │   ├── SdlManager.cpp
│   │   ├── EventDispatcher.hpp # Routes events through Mappers and manages the MIDI queue
│   │   └── EventDispatcher.cpp
│   ├── Logic/
│   │   ├── IMidiMapper.hpp     # Interface for MIDI Mapping (Strategy Pattern)
│   │   ├── MapperConfig.hpp    # Configuration structures (ButtonMode, AxisMode, MapperPreset)
│   │   ├── FlexibleMapper.hpp  # Configurable mapper with JSON preset support
│   │   └── FlexibleMapper.cpp
│   ├── DistrhoPluginInfo.h     # Metadata (Name, URI, I/O count, Custom UI defines)
│   ├── Plugin.hpp              # DPF Plugin Orchestration
│   ├── Plugin.cpp
│   └── UI.cpp                  # Graphical User Interface logic (ImGui)
└── third_party/                # Local dependencies (Git Submodules)
    ├── DPF/                    # Core DISTRHO Plugin Framework
    └── DPF-Widgets/            # DPF UI extensions including ImGui integration
```

## Dependency Management

The project uses **vcpkg** in manifest mode to manage external libraries:
- **SDL2**: For game controller abstraction.
- **Boost.Lockfree**: For high-performance, thread-safe MIDI event queuing between the hardware thread and the audio thread.
- **nlohmann-json**: For parsing JSON preset files at initialization.

The **DISTRHO Plugin Framework (DPF)** and its ImGui widgets are managed as Git submodules in `third_party/` because they are deeply integrated into the build process.

### Preset Embedding System

JSON preset files in `presets/` are embedded into the binary at compile time using a **bin2c** mechanism:
1. `cmake/bin2c.cmake` converts JSON files to C byte arrays
2. `cmake/EmbedPresets.cmake` provides CMake functions to automate embedding
3. Generated headers are placed in `build/generated/preset_*.hpp`
4. No runtime file I/O required - all presets compiled into the plugin

## Build Requirements

- **CMake** 3.10+
- **vcpkg** (pointed to by `CMAKE_TOOLCHAIN_FILE`)
- **Git** (for submodules)
- **Ninja** (build system)
- **C++23 compatible compiler** (GCC 11+, Clang 14+, or MSVC 2022+)

## Building the Project

### Initial Setup

1. **Initialize Git submodules** (DPF and DPF-Widgets):
   ```bash
   git submodule update --init --recursive
   ```

2. **Install vcpkg** if not already installed:
   ```bash
   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   ./bootstrap-vcpkg.sh  # or bootstrap-vcpkg.bat on Windows
   ```

### Build Configuration

Configure the project using CMake with **Ninja generator** and **compile_commands.json** generation:

```bash
cmake -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_BUILD_TYPE=Release
```

**Important:**
- The build folder **must** be `build/` in the project root (not a sibling directory)
- Use `-G Ninja` to generate Ninja build files
- Use `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` to generate `compile_commands.json` for IDE/LSP support
- Set `CMAKE_TOOLCHAIN_FILE` to your vcpkg installation path

If you have vcpkg configured in `.vscode/settings.json`, the toolchain file path will be automatically detected by VS Code.

### Build

Build the plugin:

```bash
cmake --build build
```

Or use Ninja directly:

```bash
ninja -C build
```

### Build Output

Plugin binaries will be generated in `build/bin/`:
- `GameControllerMIDI.vst3` - VST3 plugin
- `GameControllerMIDI.clap` - CLAP plugin
- `GameControllerMIDI-vst2.so` - VST2 plugin (Linux)

Generated preset headers will be in `build/generated/`:
- `preset_c_major_chords.hpp` - Embedded C Major preset

### Development Builds

For debug builds with symbols:

```bash
cmake -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_BUILD_TYPE=Debug

cmake --build build
```

### Troubleshooting

- If vcpkg dependencies fail to install, ensure you have all system dependencies:
  - **Linux**: `libx11-dev`, `libasound2-dev`, `libgl-dev`
  - **macOS**: Xcode Command Line Tools
  - **Windows**: Visual Studio 2022 with C++ workload

- If `compile_commands.json` is not generated, ensure `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` is set during configuration (not build)

- If preset embedding fails, check that paths in `cmake/EmbedPresets.cmake` are correctly quoted

## Core Components

### 1. `SdlManager` (The Hardware Poller)
A singleton that handles global `SDL_Init` and `SDL_Quit`, and runs the background thread for `SDL_PollEvent`. It holds a reference to an `IControllerEventHandler` (usually the `EventDispatcher`) and forwards hardware inputs (Buttons and Axes). It knows nothing about MIDI.

### 2. `EventDispatcher` (The Router)
Implements `IControllerEventHandler`. It is the central hub for:
- Tracking controller connection state and name.
- Tracking button states for UI feedback.
- Owning and delegating to the active `IMidiMapper`.
- Managing the `boost::lockfree::queue<RawMidi>` that ferries data from the SDL thread to the audio thread.

### 3. `IMidiMapper` (The Translator)
Pure business logic interface. Takes raw controller button and axis states as input and outputs `RawMidi` events. It provides metadata like `getOctaveOffset()` and `getShiftButton()`. To change how the controller plays music, implement a new `IMidiMapper`.

**Current Implementation: `FlexibleMapper`**
- Loads configuration from embedded JSON presets
- Supports multiple button modes: single notes, chords, CC (momentary/toggle), octave shifts
- Supports multiple axis modes: CC, pitch bend, aftertouch
- Configurable shift button (defaults to right shoulder)
- **Per-button shift key support**: Assign different shift modifiers to different buttons via `buttonShiftKeys` in JSON
- Real-time safe: uses fixed-size `std::array` for all collections
- Axis value caching to reduce redundant MIDI traffic

### 4. `GameControllerMIDIPlugin` (The Orchestrator)
The DPF Plugin entry point. It owns an `EventDispatcher` and handles the DPF lifecycle. In its `run()` loop, it pops MIDI events from the dispatcher and passes them to the host.

### 5. `GameControllerMIDIUI` (The Observer)
Builds the UI using Dear ImGui. It safely reads state (like connection status and button states) via the Plugin's dispatcher to render visual feedback.

## Extension Guide for Agents

### Creating a New JSON Preset
1. Create a new JSON file in `presets/` (e.g., `presets/my_preset.json`)
2. Define button mappings, axis mappings, and metadata following the schema:
   ```json
   {
     "name": "My Preset",
     "channel": 0,
     "baseOctaveOffset": 0,
     "shiftButton": 10,  // or "shiftButtonName": "rightshoulder"
     "buttons": {
       "a": { "mode": "chord", "note": 60, "velocity": 100, "intervals": [0, 4, 7] },
       "dpad_up": { "mode": "octave_up" }
     },
     "axes": {
       "leftx": { "mode": "cc", "cc": 1, "bipolar": true, "deadzone": 0.15 }
     }
   }
   ```
3. Rebuild - preset will be automatically embedded via CMake
4. Reference in Plugin.cpp: `#include "preset_my_preset.hpp"`

### Adding a New Mapping Mode
1. Create `src/Logic/YourNewMapper.hpp` and `.cpp` implementing `IMidiMapper`.
2. In `Plugin.cpp`, instantiate your mapper and call `fDispatcher->setMapper(std::move(mapper))`.
3. (Optional) Expose a DPF parameter in `initParameter()` to let the user switch between mappers dynamically.

### Adding a New State Key
> **Note**: State is currently disabled in `src/DistrhoPluginInfo.h`. Re-enable `DISTRHO_PLUGIN_WANT_STATE` and `DISTRHO_PLUGIN_WANT_FULL_STATE` first.

1. Increment state count in `Plugin` constructor.
2. Initialize key and hints in `initState()`.
3. Update logic in `getState()` and `setState()`.
4. Handle UI synchronization in `UI::stateChanged()`.

## Architecture Best Practices

- **Real-time Safety**: Never use blocking calls, mutexes (unless very short and in non-audio threads), or memory allocation in the `run()` loop or the SDL event callbacks that push to the queue. Use fixed-size `std::array` instead of `std::vector` for real-time data structures.
- **Decoupling**: Keep UI logic in `UI.cpp`, hardware logic in `SdlManager`, and MIDI logic in `IMidiMapper` implementations.
- **Modern C++**: Use C++23 features where beneficial, but ensure compatibility with the DPF build system. Prefer `std::atomic` for thread-safe UI flags.
- **JSON Parsing**: Only parse JSON during initialization (in `loadPreset()`), never in the audio thread. All configuration is loaded into fixed-size structures before real-time processing begins.
- **Thread Safety**: The `setMapper()` method is NOT fully thread-safe for dynamic switching. Only call during initialization or ensure proper synchronization.

## Key Configuration Structures

### `MapperConfig::ButtonMode`
- `None`: No action
- `Note`: Single MIDI note
- `Chord`: Multiple notes (root + intervals)
- `CC_Momentary`: CC 127 on press, 0 on release
- `CC_Toggle`: Toggle CC between 127 and 0
- `OctaveUp` / `OctaveDown`: Adjust octave offset

### `MapperConfig::AxisMode`
- `None`: No action
- `CC`: Control Change (7-bit, 0-127)
- `PitchBend`: Pitch Bend (14-bit, 0-16383)
- `Aftertouch`: Channel Aftertouch (7-bit, 0-127)

### Button Name Mappings
```
a, b, x, y                      -> SDL_CONTROLLER_BUTTON_A/B/X/Y
back, guide, start              -> SDL_CONTROLLER_BUTTON_BACK/GUIDE/START
leftstick, rightstick           -> SDL_CONTROLLER_BUTTON_LEFTSTICK/RIGHTSTICK
leftshoulder, rightshoulder     -> SDL_CONTROLLER_BUTTON_LEFTSHOULDER/RIGHTSHOULDER
dpad_up, dpad_down, dpad_left, dpad_right -> SDL_CONTROLLER_BUTTON_DPAD_*
```

### Axis Name Mappings
```
leftx, lefty                    -> SDL_CONTROLLER_AXIS_LEFTX/LEFTY
rightx, righty                  -> SDL_CONTROLLER_AXIS_RIGHTX/RIGHTY
triggerleft, triggerright       -> SDL_CONTROLLER_AXIS_TRIGGERLEFT/TRIGGERRIGHT
```

## Reference
- DPF Framework: [dear-plugins](https://github.com/DISTRHO/dear-plugins)
- Implementation Plans: `.opencode/plans/flexible_mapper_implementation.md`, `flexiblemapper_improvements.md` and `multi_shift_key_support.md`

### Per-Button Shift Key Support

The `FlexibleMapper` supports assigning different shift keys to different buttons or button groups, enabling more ergonomic and expressive controller mappings.

#### Configuration

In your JSON preset, use the `buttonShiftKeys` object to specify which shift key each button should use:

```json
{
    "shiftButtonName": "rightshoulder",  // Global default
    
    "buttonShiftKeys": {
        "dpad_up": "leftshoulder",
        "dpad_down": "leftshoulder",
        "dpad_left": "leftshoulder",
        "dpad_right": "leftshoulder"
    }
}
```

#### Behavior

- Buttons **not listed** in `buttonShiftKeys` use the global `shiftButton`
- Each button can have its own independent shift key
- Axes currently use the global shift button only (per-axis shift support can be added if needed)

#### Example Use Case

This feature enables configurations like:
- **Right shoulder + ABXY** → Scale notes (C, D, E, F, G, A, B, C+8ve)
- **Left shoulder + D-pad** → Diatonic chords (I, IV, V, vi, iii, ii, vii°, III)

Both shift keys are active simultaneously, allowing seamless transitions between melody and harmony.
