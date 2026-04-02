# FlexibleMapper Improvements Plan

This document outlines fixes and improvements for the FlexibleMapper implementation.

## Overview

Six improvements to be implemented:

| # | Improvement | Priority | Complexity |
|---|-------------|----------|------------|
| 1 | Thread safety documentation | Medium | Low |
| 2 | Missing SDL include | Low | Trivial |
| 3 | CMake path quoting | Low | Trivial |
| 4 | Preset load error logging | Medium | Low |
| 5 | Axis CC value caching | Medium | Medium |
| 6 | Configurable shift button | High | Medium |

---

## 1. Thread Safety Documentation

**Problem**: `EventDispatcher::setMapper()` uses a mutex, but `fMapper` is accessed without lock in `onControllerButton()` and `onControllerAxis()`. This works because the mapper is only set once at construction, but is fragile for future changes.

**Solution**: Add documentation comments to clarify the constraint.

**File**: `src/Core/EventDispatcher.hpp`

Add comment above `setMapper()`:

```cpp
/**
 * Set the MIDI mapper. 
 * 
 * IMPORTANT: This method is NOT fully thread-safe for dynamic switching.
 * It should only be called during initialization (before SDL events start)
 * or from the same thread that calls onControllerButton/onControllerAxis.
 * 
 * For dynamic mapper switching during playback, a lock-free mechanism
 * or atomic shared_ptr would be required.
 */
void setMapper(std::unique_ptr<IMidiMapper> mapper);
```

---

## 2. Missing SDL Include

**Problem**: `FlexibleMapper.hpp` uses `SDL_CONTROLLER_BUTTON_MAX` but relies on transitive include from `MapperConfig.hpp`.

**Solution**: Add explicit `#include <SDL.h>` to `FlexibleMapper.hpp`.

**File**: `src/Logic/FlexibleMapper.hpp`

```cpp
#pragma once

#include <SDL.h>  // ADD THIS LINE

#include <array>
#include <string_view>

#include "IMidiMapper.hpp"
#include "MapperConfig.hpp"
```

---

## 3. CMake Path Quoting

**Problem**: `EmbedPresets.cmake` doesn't quote paths, which will break with spaces.

**Solution**: Add quotes around path variables.

**File**: `cmake/EmbedPresets.cmake`

Change lines 13-17 from:
```cmake
    COMMAND ${CMAKE_COMMAND}
        -DINPUT_FILE=${INPUT_FILE}
        -DOUTPUT_FILE=${OUTPUT_FILE}
        -DVAR_NAME=${VAR_NAME}
        -P ${CMAKE_CURRENT_SOURCE_DIR}/cmake/bin2c.cmake
```

To:
```cmake
    COMMAND ${CMAKE_COMMAND}
        -DINPUT_FILE="${INPUT_FILE}"
        -DOUTPUT_FILE="${OUTPUT_FILE}"
        -DVAR_NAME="${VAR_NAME}"
        -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/bin2c.cmake"
```

---

## 4. Preset Load Error Logging

**Problem**: If `loadPreset()` fails, the plugin continues silently without a mapper.

**Solution**: Add error logging when preset fails to load.

**File**: `src/Plugin.cpp`

Change:
```cpp
auto mapper = std::make_unique<GCMidi::FlexibleMapper>();
if (mapper->loadPreset(preset_c_major_chords_data, preset_c_major_chords_size)) {
    fDispatcher->setMapper(std::move(mapper));
}
```

To:
```cpp
auto mapper = std::make_unique<GCMidi::FlexibleMapper>();
if (mapper->loadPreset(preset_c_major_chords_data, preset_c_major_chords_size)) {
    fDispatcher->setMapper(std::move(mapper));
} else {
    d_stderr2("ERROR: Failed to load preset 'c_major_chords'. Plugin will not produce MIDI output.");
}
```

---

## 5. Axis CC Value Caching

**Problem**: Every axis motion event sends a CC message, even if the quantized value (0-127) hasn't changed. This floods the MIDI output with redundant messages.

**Solution**: Track the last sent value per axis and only send when it changes.

### 5.1 Add State Tracking

**File**: `src/Logic/FlexibleMapper.hpp`

Add to private section:
```cpp
// Track last sent CC values per axis to avoid redundant messages
std::array<uint8_t, SDL_CONTROLLER_AXIS_MAX> fLastAxisCCValues{};
std::array<uint16_t, SDL_CONTROLLER_AXIS_MAX> fLastAxisPitchBendValues{};
std::array<uint8_t, SDL_CONTROLLER_AXIS_MAX> fLastAxisAftertouchValues{};
```

### 5.2 Update Axis Handlers

**File**: `src/Logic/FlexibleMapper.cpp`

Update `handleAxisCC()`:
```cpp
void FlexibleMapper::handleAxisCC(const MapperConfig::AxisConfig& config, int16_t value, 
                                   uint8_t axis, MidiQueue& queue) {
    float normalized = normalizeAxis(value, config);
    uint8_t ccValue = static_cast<uint8_t>(normalized * 127.0f);
    
    // Only send if value changed
    if (ccValue == fLastAxisCCValues[axis]) {
        return;
    }
    fLastAxisCCValues[axis] = ccValue;

    RawMidi midi{};
    midi.data[0] = 0xB0 | (fPreset.channel & 0x0F);
    midi.data[1] = config.ccNumber;
    midi.data[2] = ccValue;
    midi.size = 3;
    queue.push(midi);
}
```

Update `handleAxisPitchBend()`:
```cpp
void FlexibleMapper::handleAxisPitchBend(const MapperConfig::AxisConfig& config, int16_t value,
                                          uint8_t axis, MidiQueue& queue) {
    float normalized = normalizeAxis(value, config);
    uint16_t bendValue = static_cast<uint16_t>(normalized * 16383.0f);
    
    // Only send if value changed
    if (bendValue == fLastAxisPitchBendValues[axis]) {
        return;
    }
    fLastAxisPitchBendValues[axis] = bendValue;

    RawMidi midi{};
    midi.data[0] = 0xE0 | (fPreset.channel & 0x0F);
    midi.data[1] = bendValue & 0x7F;
    midi.data[2] = (bendValue >> 7) & 0x7F;
    midi.size = 3;
    queue.push(midi);
}
```

Update Aftertouch case in `onAxis()`:
```cpp
case MapperConfig::AxisMode::Aftertouch:
    {
        float normalized = normalizeAxis(value, config);
        uint8_t atValue = static_cast<uint8_t>(normalized * 127.0f);
        
        // Only send if value changed
        if (atValue == fLastAxisAftertouchValues[axis]) {
            break;
        }
        fLastAxisAftertouchValues[axis] = atValue;
        
        RawMidi midi{};
        midi.data[0] = 0xD0 | (fPreset.channel & 0x0F);
        midi.data[1] = atValue;
        midi.size = 2;
        queue.push(midi);
    }
    break;
```

### 5.3 Update Function Signatures

Update header and implementation to pass `axis` parameter to handlers:
```cpp
// In header:
void handleAxisCC(const MapperConfig::AxisConfig& config, int16_t value, uint8_t axis, MidiQueue& queue);
void handleAxisPitchBend(const MapperConfig::AxisConfig& config, int16_t value, uint8_t axis, MidiQueue& queue);

// In onAxis() calls:
handleAxisCC(config, value, axis, queue);
handleAxisPitchBend(config, value, axis, queue);
```

---

## 6. Configurable Shift Button

**Problem**: The shift button is hardcoded to `SDL_CONTROLLER_BUTTON_RIGHTSHOULDER` in `SdlManager.cpp`. Users should be able to configure which button acts as the shift modifier via the preset JSON.

**Solution**: Add shift button configuration to the preset and pass it through the architecture.

### 6.1 Update MapperConfig

**File**: `src/Logic/MapperConfig.hpp`

Add to `MapperPreset` struct:
```cpp
struct MapperPreset {
    std::array<char, 64> name{};
    uint8_t channel = 0;
    int8_t baseOctaveOffset = 0;
    uint8_t shiftButton = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;  // ADD THIS LINE
    
    // ... rest of struct
};
```

### 6.2 Update JSON Parsing

**File**: `src/Logic/FlexibleMapper.cpp`

In `loadPreset()`, add:
```cpp
fPreset.shiftButton = j.value("shiftButton", SDL_CONTROLLER_BUTTON_RIGHTSHOULDER);

// Also support string-based button name
if (j.contains("shiftButtonName")) {
    int idx = buttonNameToIndex(j["shiftButtonName"].get<std::string>());
    if (idx >= 0) {
        fPreset.shiftButton = static_cast<uint8_t>(idx);
    }
}
```

### 6.3 Add Getter to IMidiMapper Interface

**File**: `src/Logic/IMidiMapper.hpp`

Add method:
```cpp
// Get the button index used as shift modifier (default: right shoulder)
virtual uint8_t getShiftButton() const {
    return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
}
```

### 6.4 Implement in FlexibleMapper

**File**: `src/Logic/FlexibleMapper.hpp`

Add:
```cpp
uint8_t getShiftButton() const override;
```

**File**: `src/Logic/FlexibleMapper.cpp`

Add:
```cpp
uint8_t FlexibleMapper::getShiftButton() const {
    return fPreset.shiftButton;
}
```

### 6.5 Update SdlManager to Query Shift Button

**File**: `src/Core/IControllerEventHandler.hpp`

Add method:
```cpp
// Get the button index used as shift modifier
virtual uint8_t getShiftButton() const {
    return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;  // Default
}
```

**File**: `src/Core/EventDispatcher.hpp`

Add:
```cpp
uint8_t getShiftButton() const override;
```

**File**: `src/Core/EventDispatcher.cpp`

Add:
```cpp
uint8_t EventDispatcher::getShiftButton() const {
    if (fMapper) {
        return fMapper->getShiftButton();
    }
    return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
}
```

### 6.6 Update SdlManager to Use Dynamic Shift Button

**File**: `src/Core/SdlManager.cpp`

Change `handleControllerButton()`:
```cpp
void SdlManager::handleControllerButton(const SDL_ControllerButtonEvent& event) {
    if (!fHandler || !fController) {
        return;
    }

    bool down = (event.type == SDL_CONTROLLERBUTTONDOWN);
    uint8_t button = event.button;
    uint8_t shiftBtn = fHandler->getShiftButton();
    bool shift = SDL_GameControllerGetButton(fController, 
                     static_cast<SDL_GameControllerButton>(shiftBtn)) == 1;
    fHandler->onControllerButton(button, down, shift);
}
```

Change `handleControllerAxis()`:
```cpp
void SdlManager::handleControllerAxis(const SDL_ControllerAxisEvent& event) {
    if (!fHandler || !fController) {
        return;
    }

    uint8_t axis = event.axis;
    int16_t value = event.value;
    uint8_t shiftBtn = fHandler->getShiftButton();
    bool shift = SDL_GameControllerGetButton(fController,
                     static_cast<SDL_GameControllerButton>(shiftBtn)) == 1;
    fHandler->onControllerAxis(axis, value, shift);
}
```

### 6.7 Update JSON Schema

**File**: `presets/c_major_chords.json`

Add optional field (example):
```json
{
    "name": "C Major Chords",
    "channel": 0,
    "baseOctaveOffset": 0,
    "shiftButtonName": "rightshoulder",
    ...
}
```

---

## Implementation Order

Recommended order (dependencies considered):

1. **Missing SDL include** (trivial, no dependencies)
2. **CMake path quoting** (trivial, no dependencies)
3. **Preset load error logging** (trivial, no dependencies)
4. **Thread safety documentation** (trivial, no dependencies)
5. **Axis CC value caching** (medium, self-contained)
6. **Configurable shift button** (medium, touches multiple files)

---

## Verification Checklist

After implementation:

- [ ] Build succeeds with no warnings
- [ ] Plugin loads preset correctly (check for error message if preset invalid)
- [ ] Axis movements only send CC when quantized value changes
- [ ] Shift button can be changed via JSON preset
- [ ] Documentation comments are present on `setMapper()`
- [ ] Paths with spaces work in CMake (test with `presets/test preset.json` if desired)

---

## Future Considerations

Not in scope but worth noting:

- **Runtime preset switching**: Would require proper thread-safe mapper swapping
- **MIDI learn**: Allow users to assign mappings by pressing controller buttons
- **Multiple shift layers**: Support Shift1, Shift2 for more mapping layers
