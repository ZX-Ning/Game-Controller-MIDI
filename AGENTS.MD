# AGENTS.MD - Project Structure and Development Guide

This document provides a technical overview of the `GameControllerMIDI` project, designed to assist both human engineers and AI agents in understanding and extending this audio plugin framework.

## Project Overview

This project is an audio plugin built using the **DISTRHO Plugin Framework (DPF)**, **Dear ImGui**, and **SDL2**. It translates game controller inputs into MIDI messages using a decoupled, highly extendable architecture.

## Directory Structure

```text
.
├── CMakeLists.txt              # Main build configuration
├── AGENTS.MD                   # This guide
├── src/                        # Plugin source code
│   ├── Common/                 
│   │   └── MidiTypes.hpp       # Shared types (RawMidi)
│   ├── Core/                   
│   │   ├── IControllerEventHandler.hpp # Interface connecting SDL events to the plugin
│   │   ├── SdlManager.hpp      # SDL Lifecycle & Hardware Polling (Singleton)
│   │   └── SdlManager.cpp
│   ├── Logic/                  
│   │   ├── IMidiMapper.hpp     # Interface for Note Mapping (Strategy Pattern)
│   │   ├── MajorScaleMapper.hpp# C-Major X/Y/B/A logic implementation
│   │   └── MajorScaleMapper.cpp
│   ├── DistrhoPluginInfo.h     # Metadata (Name, URI, I/O count, Custom UI defines)
│   ├── Plugin.hpp              # DPF Plugin Orchestration
│   ├── Plugin.cpp
│   └── UI.cpp                  # Graphical User Interface logic (ImGui)
└── third_party/                # Dependencies (Git Submodules)
    ├── DPF/                    # Core DISTRHO Plugin Framework
    ├── DPF-Widgets/            # DPF UI extensions including ImGui integration
    └── lockfree/               # Boost.Lockfree for thread-safe communication
```

## Architectural Components

The project strictly follows the Single-Responsibility Principle with the following ownership and execution model:

### 1. `SdlManager` (The Hardware Poller)
A singleton that handles global `SDL_Init` and `SDL_Quit`, and runs the background thread for `SDL_PollEvent`. It holds a reference to an `IControllerEventHandler` (the Plugin) and blindly forwards hardware inputs. It knows nothing about MIDI.

### 2. `IMidiMapper` (The Translator)
Pure business logic. Takes raw controller button states as input and outputs `RawMidi` events. To change how the controller plays music (e.g., adding a drum pad mode), implement a new `IMidiMapper`.

### 3. `GameControllerMIDIPlugin` (The Orchestrator)
Acts as the central nervous system. 
- Implements `IControllerEventHandler` to receive SDL events from `SdlManager`.
- Owns the active `IMidiMapper` strategy.
- Owns a `boost::lockfree::queue<RawMidi>` to ferry data from the background SDL thread into the high-priority audio `run()` loop.

### 4. `GameControllerMIDIUI` (The Observer)
Builds the UI using Dear ImGui. It safely reads state (like `fControllerConnected`, `fButtonStates`, and `fMidiHistory`) directly from the Plugin instance to render visual feedback. It does not modify audio or controller state directly.

## Extension Guide for Agents

### Adding a New Mapping Mode
1. Create `src/Logic/YourNewMapper.hpp` and `.cpp` implementing `IMidiMapper`.
2. Update `Plugin.cpp` to instantiate and use your new mapper.
3. (Optional) Expose a DPF parameter in `initParameter()` to let the user switch between `MajorScaleMapper` and `YourNewMapper` dynamically.

### Adding a New State Key
> **Note**: State is currently disabled in `src/DistrhoPluginInfo.h`. Re-enable `DISTRHO_PLUGIN_WANT_STATE` and `DISTRHO_PLUGIN_WANT_FULL_STATE` first.

1. Increment state count in `Plugin` constructor.
2. Initialize key and hints in `initState()`.
3. Update logic in `getState()` and `setState()`.
4. Handle UI synchronization in `UI::stateChanged()`.

### Reference
Referece code: [dear-plugins](https://github.com/DISTRHO/dear-plugins)
