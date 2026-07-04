# Extension Guide

Step-by-step instructions for extending the GameControllerMIDI plugin.

## Creating a New JSON Preset

### Step 1: Create the JSON File

Create a new file in `presets/` directory:

```bash
touch presets/my_preset.json
```

### Step 2: Define the Preset

Use the schema documented in [configuration.md](configuration.md):

```json
{
  "name": "My Custom Preset",
  "channel": 0,
  "baseOctaveOffset": 0,
  "shiftButtonName": "rightshoulder",
  
  "buttons": {
    "a": { "mode": "note", "note": 60, "velocity": 100 },
    "b": { "mode": "note", "note": 62, "velocity": 100 },
    "x": { "mode": "chord", "note": 64, "velocity": 100, "intervals": [0, 4, 7] },
    "y": { "mode": "cc_momentary", "cc": 64, "velocity": 127 },
    "dpad_up": { "mode": "octave_up" },
    "dpad_down": { "mode": "octave_down" }
  },
  
  "axes": {
    "leftx": { "mode": "cc", "cc": 1, "bipolar": true, "deadzone": 0.15 }
  }
}
```

### Step 3: Rebuild

The preset will be automatically embedded via CMake:

```bash
cmake --build build
```

The build system:
1. Runs `cmake/bin2c.cmake` on the JSON file
2. Generates `build/generated/preset_my_preset.hpp`
3. The header contains the JSON as a C byte array

### Step 4: Load in Plugin

Edit `src/Plugin.cpp` to include and use the new preset:

```cpp
#include "preset_my_preset.hpp"

// In constructor or initialization:
auto mapper = std::make_unique<FlexibleMapper>();
mapper->loadPreset(reinterpret_cast<const char*>(preset_my_preset_data));
fDispatcher->setMapper(std::move(mapper));
```

---

## Adding a New IMidiMapper Implementation

For custom mapping logic beyond what `FlexibleMapper` provides.

### Step 1: Create Header

Create `src/Logic/MyMapper.hpp`:

```cpp
#pragma once

#include "IMidiMapper.hpp"
#include <array>

class MyMapper : public IMidiMapper {
public:
    MyMapper();

    void onButton(uint8_t button, bool pressed, bool shiftState,
                  SharedState& state, IMidiOutputSink& out) override;
    void onAxis(uint8_t axis, int16_t value, bool shiftState,
                const SharedState& state, IMidiOutputSink& out) override;

    bool flushActiveNotes(IMidiOutputSink& out) override;
    int8_t getInitialBaseOctaveOffset() const override { return 0; }
    int getShiftButton() const override { return 10; } // rightshoulder

private:
    std::array<uint8_t, 16> activeNotes_{};
    std::array<bool, 16> hasActiveNote_{};
};
```

### Step 2: Create Implementation

Create `src/Logic/MyMapper.cpp`:

```cpp
#include "MyMapper.hpp"
#include "../Common/MidiTypes.hpp"

MyMapper::MyMapper() = default;

void MyMapper::onButton(uint8_t button, bool pressed, bool, SharedState& state, IMidiOutputSink& out) {
    // Example: Button 0 (A) plays C4
    if (button == 0) {
        const auto octave = state.octaveSnapshot();
        const uint8_t note = static_cast<uint8_t>(60 + octave.base * 12 + octave.trigger * 12);

        RawMidi midi;
        midi.data[0] = pressed ? 0x90 : 0x80; // Note On/Off, channel 0
        midi.data[1] = pressed ? note : activeNotes_[button];
        midi.data[2] = pressed ? 100 : 0;
        midi.size = 3;
        if (out.pushMidi(midi) && pressed) {
            activeNotes_[button] = note;
            hasActiveNote_[button] = true;
        }
        if (!pressed) {
            hasActiveNote_[button] = false;
        }
    }

    // Example: Button 11 (dpad_up) = octave up
    if (button == 11 && pressed) {
        state.adjustBaseOctaveFromMapper(1);
    }
}

void MyMapper::onAxis(uint8_t axis, int16_t value, bool, const SharedState&, IMidiOutputSink& out) {
    // Example: Left stick X = CC 1
    if (axis == 0) {
        RawMidi midi;
        midi.data[0] = 0xB0; // CC, channel 0
        midi.data[1] = 1;    // CC number
        midi.data[2] = static_cast<uint8_t>((static_cast<int>(value) + 32768) / 516); // approx 0..127
        midi.size = 3;
        out.pushMidi(midi);
    }
}

bool MyMapper::flushActiveNotes(IMidiOutputSink& out) {
    bool allQueued = true;
    for (size_t button = 0; button < activeNotes_.size(); ++button) {
        if (!hasActiveNote_[button]) {
            continue;
        }

        RawMidi midi{};
        midi.data[0] = 0x80;
        midi.data[1] = activeNotes_[button];
        midi.data[2] = 0;
        midi.size = 3;
        if (out.pushMidi(midi)) {
            hasActiveNote_[button] = false;
        }
        else {
            allQueued = false;
        }
    }
    return allQueued;
}
```

### Step 3: Add to CMakeLists.txt

```cmake
add_library(GameControllerMIDI_lib
    # ... existing files ...
    src/Logic/MyMapper.cpp
)
```

### Step 4: Use in Plugin

```cpp
#include "Logic/MyMapper.hpp"

// In Plugin constructor:
auto mapper = std::make_unique<MyMapper>();
fDispatcher->setMapper(std::move(mapper));
```

### Real-time Safety Checklist

Your mapper MUST be real-time safe:

- [ ] No dynamic memory allocation (`new`, `malloc`, `std::vector::push_back`)
- [ ] No blocking calls (mutex, file I/O, network)
- [ ] Use `std::array` instead of `std::vector`
- [ ] Pre-allocate everything in constructor
- [ ] Mapper callbacks may be called from SDL thread — keep them fast
- [ ] Do not store `SharedState&`; use it only during the current mapper callback

---

## Adding a New DPF State Key

For persisting new configuration values across sessions.

### Step 1: Add Enum Value

Edit `src/Plugin.hpp`:

```cpp
enum States {
    kStateConfig = 0,
    kStateTriggerOctave,
    kStateEditMode,
    kStateWidth,
    kStateHeight,
    kStateMyNewState,  // Add here
    kStateCount
};
```

### Step 2: Update Constructor

In `src/Plugin.cpp` constructor, verify state count matches:

```cpp
GameControllerMIDIPlugin()
    : Plugin(0, 0, kStateCount)  // 0 params, 0 programs, N states
{
    // ...
}
```

### Step 3: Initialize in initState()

```cpp
void GameControllerMIDIPlugin::initState(uint32_t index, State& state) {
    switch (index) {
        // ... existing cases ...
        
        case kStateMyNewState:
            state.key = "myNewState";
            state.label = "My New State";
            state.defaultValue = "default_value";
            state.hints = kStateIsHostReadable | kStateIsHostWritable;
            break;
    }
}
```

### Step 4: Implement getState()

```cpp
String GameControllerMIDIPlugin::getState(const char* key) const {
    if (std::strcmp(key, "myNewState") == 0) {
        return String(myNewStateValue_.c_str());
    }
    // ... existing keys ...
    return String();
}
```

### Step 5: Implement setState()

```cpp
void GameControllerMIDIPlugin::setState(const char* key, const char* value) {
    if (std::strcmp(key, "myNewState") == 0) {
        myNewStateValue_ = value;
        // Apply the value as needed
        return;
    }
    // ... existing keys ...
}
```

### Step 6: Handle in UI (if needed)

In `src/UI.cpp`, implement `stateChanged()`:

```cpp
void GameControllerMIDIUI::stateChanged(const char* key, const char* value) {
    if (std::strcmp(key, "myNewState") == 0) {
        // Update UI state
        myUiValue_ = value;
    }
    // ... existing keys ...
}
```

### Important Warning

**Do NOT use `kStateIsOnlyForDSP`** for states that the UI needs to modify. This flag blocks UI→DSP communication, causing `setState()` calls from the UI to be silently discarded.

Only use `kStateIsOnlyForDSP` for states that:
- Are only set by the DSP/audio thread
- The UI only reads (never writes)

---

## Adding a New Button/Axis Mode

To extend `FlexibleMapper` with new mapping modes.

### Step 1: Add Enum Value

Edit `src/Logic/MapperConfig.hpp`:

```cpp
enum class ButtonMode {
    None,
    Note,
    Chord,
    CC_Momentary,
    CC_Toggle,
    OctaveUp,
    OctaveDown,
    MyNewMode  // Add here
};
```

### Step 2: Update Serialization

Edit `src/Logic/MapperSerialization.cpp`:

```cpp
// In buttonModeFromString():
if (str == "my_new_mode") return ButtonMode::MyNewMode;

// In buttonModeToString():
case ButtonMode::MyNewMode: return "my_new_mode";
```

### Step 3: Implement in FlexibleMapper

Edit `src/Logic/FlexibleMapper.cpp` in `onButton()`:

```cpp
case ButtonMode::MyNewMode:
    if (pressed) {
        // Your custom logic here
        RawMidi midi;
        // ... fill midi ...
        out.pushMidi(midi);
    }
    break;
```

### Step 4: Update UI (if needed)

Add the new mode to dropdowns in `src/UI.cpp` if using the Edit mode UI.
