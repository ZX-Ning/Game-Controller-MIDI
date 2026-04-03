# GameControllerMIDI

Audio plugin (VST3/CLAP/VST2) that translates game controller inputs into MIDI messages. Play virtual instruments with your gamepad.

*BTW: This project is a experiment of reviving one of my old side projects with vibe coding, testing possibilities of today's LLM and agent tools on a modern C++ codebase.*

## Features

- **Button-to-MIDI mapping**: Notes, chords, CC (momentary/toggle), octave shifts
- **Axis-to-MIDI mapping**: CC, pitch bend, aftertouch with configurable deadzone
- **Trigger octave control**: LT/RT for real-time octave shifting
- **Per-button shift keys**: Independent shift modifiers for different button groups
- **Dual-mode UI**: Play mode for performance, Edit mode for configuration
- **JSON presets**: Fully configurable mappings via embedded presets
- **State persistence**: Configuration saved/restored by DAW

## Quick Start
Require cmake and vcpkg.
```bash
git submodule update --init --recursive
cmake -B build -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE=/path/to/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build
```

Output: `build/bin/GameControllerMIDI.{vst3,clap}`

## Documentation

See [AGENTS.md](AGENTS.md) for developer guide and [docs/](docs/) for detailed documentation.

## Dependencies

- [DPF](https://github.com/DISTRHO/DPF) - DISTRHO Plugin Framework
- [SDL2](https://www.libsdl.org/) - Game controller input
- [Dear ImGui](https://github.com/ocornut/imgui) - User interface
- [Boost.Lockfree](https://www.boost.org/) - Thread-safe MIDI queue
- [nlohmann/json](https://github.com/nlohmann/json) - Preset parsing

---

