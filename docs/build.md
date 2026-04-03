# Build Guide

Complete instructions for building the GameControllerMIDI plugin.

## Requirements

- **CMake** 3.10+
- **vcpkg** (pointed to by `CMAKE_TOOLCHAIN_FILE`)
- **Git** (for submodules)
- **Ninja** (build system)
- **C++23 compatible compiler** (GCC 11+, Clang 14+, or MSVC 2022+)

## Initial Setup

### 1. Initialize Git Submodules

DPF and DPF-Widgets are managed as submodules:

```bash
git submodule update --init --recursive
```

### 2. Install vcpkg

If not already installed:

```bash
git clone https://github.com/Microsoft/vcpkg.git
cd vcpkg
./bootstrap-vcpkg.sh  # or bootstrap-vcpkg.bat on Windows
```

## Build Configuration

Configure with CMake using **Ninja generator**:

```bash
cmake -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_BUILD_TYPE=Release
```

### Configuration Flags

| Flag | Purpose |
|------|---------|
| `-B build` | Output to `build/` directory (required location) |
| `-G Ninja` | Use Ninja build system |
| `-DCMAKE_TOOLCHAIN_FILE=...` | Point to vcpkg toolchain |
| `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON` | Generate `compile_commands.json` for LSP |
| `-DCMAKE_BUILD_TYPE=Release` | Release build (or `Debug` for symbols) |

**Note:** If vcpkg is configured in `.vscode/settings.json`, VS Code will auto-detect the toolchain path.

## Building

```bash
cmake --build build
```

Or directly with Ninja:

```bash
ninja -C build
```

## Build Output

Plugin binaries are generated in `build/bin/`:

| File | Format |
|------|--------|
| `GameControllerMIDI.vst3` | VST3 plugin |
| `GameControllerMIDI.clap` | CLAP plugin |
| `GameControllerMIDI-vst2.so` | VST2 plugin (Linux) |

Generated preset headers are in `build/generated/`:
- `preset_c_major_chords.hpp` - Embedded preset as C byte array

## Debug Builds

For debugging with symbols:

```bash
cmake -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake \
  -DCMAKE_EXPORT_COMPILE_COMMANDS=ON \
  -DCMAKE_BUILD_TYPE=Debug

cmake --build build
```

## Troubleshooting

### vcpkg Dependencies Fail

Ensure system dependencies are installed:

| Platform | Packages |
|----------|----------|
| **Linux** | `libx11-dev`, `libasound2-dev`, `libgl-dev` |
| **macOS** | Xcode Command Line Tools |
| **Windows** | Visual Studio 2022 with C++ workload |

### `compile_commands.json` Not Generated

The flag must be set during **configuration**, not build:

```bash
cmake -B build -DCMAKE_EXPORT_COMPILE_COMMANDS=ON ...
```

### Preset Embedding Fails

Check that paths in `cmake/EmbedPresets.cmake` are correctly quoted. The bin2c mechanism converts JSON files to C byte arrays at build time.

### Build Directory Location

The build folder **must** be `build/` in the project root, not a sibling directory. Many paths assume this location.
