# GameControllerMIDI - Developer Guide

## AI Agent Instructions

**Follow the best practices of modern C++ (C++23) and real-time audio applications. Ensure high readability and maintainability.**

Key constraints:
- **Real-time safety**: No allocations, locks, or blocking in audio thread
- **Fixed-size arrays**: Use `std::array`, never `std::vector` in real-time paths
- **Build directory**: Must be `build/` in project root

## Project Overview

Audio plugin (VST3/CLAP/VST2) built with **DPF**, **Dear ImGui**, and **SDL2**. Translates game controller inputs into MIDI messages.

## Directory Structure

```
.
├── CMakeLists.txt              # Build configuration (vcpkg)
├── vcpkg.json                  # Dependencies: SDL2, Boost, nlohmann-json
├── cmake/                      # bin2c preset embedding
├── presets/                    # JSON preset files
├── docs/                       # Detailed documentation
│   ├── build.md                # Full build guide
│   ├── architecture.md         # Component details
│   ├── configuration.md        # Enums, mappings, JSON schema
│   └── extension-guide.md      # How-to recipes
├── src/
│   ├── Common/MidiTypes.hpp    # RawMidi struct
│   ├── Core/
│   │   ├── SdlManager.*        # Hardware polling (singleton)
│   │   ├── EventDispatcher.*   # MIDI queue, trigger octave
│   │   └── IControllerEventHandler.hpp
│   ├── Logic/
│   │   ├── IMidiMapper.hpp     # Mapper interface
│   │   ├── MapperConfig.hpp    # Config structs
│   │   ├── MapperSerialization.cpp
│   │   └── FlexibleMapper.*    # JSON-configurable mapper
│   ├── DistrhoPluginInfo.h     # DPF metadata
│   ├── Plugin.*                # DPF plugin + state system
│   └── UI.cpp                  # ImGui dual-mode UI
└── third_party/                # DPF, DPF-Widgets (submodules)
```

## Quick Start

```bash
git submodule update --init --recursive
cmake -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
cmake --build build
```

Output: `build/bin/GameControllerMIDI.{vst3,clap}`

## Core Components

| Component | Role | File |
|-----------|------|------|
| **SdlManager** | Polls controller hardware via SDL | `src/Core/SdlManager.*` |
| **EventDispatcher** | Routes events, manages MIDI queue, trigger octave | `src/Core/EventDispatcher.*` |
| **FlexibleMapper** | Translates buttons/axes to MIDI via JSON config | `src/Logic/FlexibleMapper.*` |
| **Plugin** | DPF entry point, state persistence | `src/Plugin.*` |
| **UI** | ImGui interface (Play/Edit modes) | `src/UI.cpp` |

## Architecture Best Practices

- **Real-time safety**: No blocking, no alloc in `run()` or SDL callbacks
- **Decoupling**: UI in `UI.cpp`, hardware in `SdlManager`, MIDI logic in mappers
- **Thread safety**: Use `boost::lockfree::queue` for cross-thread MIDI
- **JSON parsing**: Only at init, never in audio thread
- **State system**: Don't use `kStateIsOnlyForDSP` for UI-modifiable states

## Documentation Index

| Document | Contents | Read when... |
|----------|----------|--------------|
| [docs/build.md](docs/build.md) | Full build setup, flags, troubleshooting | Setting up project or fixing build errors |
| [docs/architecture.md](docs/architecture.md) | Component internals, threading, state system | Understanding or modifying core components |
| [docs/configuration.md](docs/configuration.md) | Enums, button/axis names, JSON schema | Writing or editing presets |
| [docs/extension-guide.md](docs/extension-guide.md) | Step-by-step extension recipes | Adding presets, mappers, or state keys |

## External References

- [DPF Framework](https://github.com/DISTRHO/DPF)
- [dear-plugins examples](https://github.com/DISTRHO/dear-plugins)
