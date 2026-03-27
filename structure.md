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
├── vcpkg.json                  # Dependency manifest (SDL2, Boost)
├── .vscode/settings.json       # VS Code CMake/vcpkg integration
├── AGENTS.MD                   # This guide
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
│   │   ├── IMidiMapper.hpp     # Interface for Note Mapping (Strategy Pattern)
│   │   ├── MajorScaleMapper.hpp# C-Major X/Y/B/A logic implementation
│   │   └── MajorScaleMapper.cpp
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

The **DISTRHO Plugin Framework (DPF)** and its ImGui widgets are managed as Git submodules in `third_party/` because they are deeply integrated into the build process.

## Build Requirements

- **CMake** 3.10+
- **vcpkg** (pointed to by `CMAKE_TOOLCHAIN_FILE`)
- **Git** (for submodules)

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
Pure business logic. Takes raw controller button and axis states as input and outputs `RawMidi` events. It also provides metadata like `getOctaveOffset()`. To change how the controller plays music, implement a new `IMidiMapper`.

### 4. `GameControllerMIDIPlugin` (The Orchestrator)
The DPF Plugin entry point. It owns an `EventDispatcher` and handles the DPF lifecycle. In its `run()` loop, it pops MIDI events from the dispatcher and passes them to the host.

### 5. `GameControllerMIDIUI` (The Observer)
Builds the UI using Dear ImGui. It safely reads state (like connection status and button states) via the Plugin's dispatcher to render visual feedback.

## Extension Guide for Agents

### Adding a New Mapping Mode
1. Create `src/Logic/YourNewMapper.hpp` and `.cpp` implementing `IMidiMapper`.
2. Update `EventDispatcher.cpp` to instantiate your new mapper (or add logic to switch them).
3. (Optional) Expose a DPF parameter in `initParameter()` to let the user switch between mappers dynamically.

### Adding a New State Key
> **Note**: State is currently disabled in `src/DistrhoPluginInfo.h`. Re-enable `DISTRHO_PLUGIN_WANT_STATE` and `DISTRHO_PLUGIN_WANT_FULL_STATE` first.

1. Increment state count in `Plugin` constructor.
2. Initialize key and hints in `initState()`.
3. Update logic in `getState()` and `setState()`.
4. Handle UI synchronization in `UI::stateChanged()`.

## Architecture Best Practices

- **Real-time Safety**: Never use blocking calls, mutexes (unless very short and in non-audio threads), or memory allocation in the `run()` loop or the SDL event callbacks that push to the queue.
- **Decoupling**: Keep UI logic in `UI.cpp`, hardware logic in `SdlManager`, and MIDI logic in `IMidiMapper` implementations.
- **Modern C++**: Use C++23 features where beneficial, but ensure compatibility with the DPF build system. Prefer `std::atomic` for thread-safe UI flags.

## Reference
Reference code: [dear-plugins](https://github.com/DISTRHO/dear-plugins)
