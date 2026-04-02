# Multi-Shift Key Support Implementation Plan

## Overview

This document outlines the implementation plan for adding per-button shift key support to the GameControllerMIDI plugin. This feature enables different buttons to use different shift modifiers, allowing for more complex and ergonomic mappings.

**Status:** Planning Phase  
**Date:** 2026-04-02  
**Target:** FlexibleMapper architecture enhancement

---

## Executive Summary

### Problem Statement

Currently, the FlexibleMapper only supports a **single global shift button** (typically the right shoulder button). All buttons that have shift-modified behavior use the same shift key. This limits the expressiveness of controller mappings.

### User Requirements

The user requires the following button mapping configuration:
- **Right shoulder + ABXY buttons** → C Major scale notes (C, D, E, F, G, A, B, C+8ve)
- **Left shoulder + D-pad buttons** → C Major diatonic chords (I, IV, V, vi, iii, ii, vii°, III)

This requires **two independent shift keys** working with different button groups.

### Proposed Solution

Implement **per-button shift key configuration** where:
- Each button can optionally specify its own shift key
- Buttons without a specific shift key fall back to the global default
- The JSON preset format is extended to support `buttonShiftKeys` mapping
- Full backward compatibility with existing presets

---

## Current Architecture Analysis

### Shift Key Flow (Current Implementation)

```
1. Preset Definition
   └─> MapperPreset.shiftButton (uint8_t, global)

2. SdlManager Event Handling
   ├─> fHandler->getShiftButton()  // Query global shift
   ├─> SDL_GameControllerGetButton(shiftBtn)  // Read hardware state
   └─> fHandler->onControllerButton(button, pressed, shiftState)

3. FlexibleMapper Processing
   └─> config = shift ? fPreset.shiftButtons[button] : fPreset.buttons[button]
```

### Key Components

| Component | File | Responsibility |
|-----------|------|----------------|
| `MapperPreset` | `MapperConfig.hpp` | Data structure holding button configs and global shift button |
| `FlexibleMapper` | `FlexibleMapper.cpp` | Parses JSON, implements mapping logic |
| `SdlManager` | `SdlManager.cpp` | Queries shift state and dispatches events |
| `EventDispatcher` | `EventDispatcher.cpp` | Routes events from SDL thread to mapper |
| `IMidiMapper` | `IMidiMapper.hpp` | Interface defining mapper contract |

### Limitations

1. **Single Global Shift**: All buttons share one shift modifier
2. **Static Shift Query**: `getShiftButton()` returns a single value
3. **No Per-Button Override**: `ButtonConfig` has no shift key field

---

## Proposed Architecture

### Core Concept

Extend the architecture to support **per-button shift key overrides** while maintaining full backward compatibility.

### Data Flow (New)

```
1. Preset Definition
   ├─> MapperPreset.shiftButton (uint8_t, global default)
   └─> ButtonConfig.shiftButton (int8_t, per-button override, -1 = use global)

2. SdlManager Event Handling
   ├─> fHandler->getShiftButtonForButton(button)  // NEW: Dynamic query
   ├─> SDL_GameControllerGetButton(shiftBtn)
   └─> fHandler->onControllerButton(button, pressed, shiftState)

3. FlexibleMapper Processing
   └─> config = shift ? fPreset.shiftButtons[button] : fPreset.buttons[button]
       (Logic unchanged, shift state is correctly computed by SdlManager)
```

### Design Principles

1. **Backward Compatibility**: Existing presets work without modification
2. **Opt-In**: Per-button shift is optional, global shift remains default
3. **Real-Time Safety**: No memory allocation or locks in event handling path
4. **Interface Extension**: Use virtual methods with default implementations

---

## Implementation Plan

### Phase 1: Data Structure Modification

#### 1.1 Modify `MapperConfig.hpp`

**Location:** `src/Logic/MapperConfig.hpp:34-40`

**Change:** Add `shiftButton` field to `ButtonConfig`

```cpp
struct ButtonConfig {
    ButtonMode mode = ButtonMode::None;
    uint8_t noteOrCC = 60;
    uint8_t velocity = 100;
    uint8_t intervalCount = 0;
    std::array<int8_t, MAX_CHORD_INTERVALS> chordIntervals{};
    
    // Per-button shift key override
    // -1 = use global shift from MapperPreset.shiftButton
    // 0-14 = specific SDL_GameControllerButton index
    int8_t shiftButton = -1;
};
```

**Rationale:**
- `int8_t` allows -1 as sentinel value for "use global"
- Default value `-1` ensures backward compatibility
- Fixed-size POD type maintains real-time safety

---

### Phase 2: Interface Extension

#### 2.1 Extend `IMidiMapper` Interface

**Location:** `src/Logic/IMidiMapper.hpp:36-38`

**Change:** Add `getShiftButtonForButton()` virtual method

```cpp
// Get the button index used as shift modifier (default: right shoulder)
virtual uint8_t getShiftButton() const {
    return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
}

// Get the shift button for a specific button (per-button shift support)
// Default implementation: fall back to global shift
virtual uint8_t getShiftButtonForButton(uint8_t button) const {
    (void)button;  // Suppress unused parameter warning
    return getShiftButton();
}
```

**Rationale:**
- Default implementation provides backward compatibility
- Mappers not implementing per-button shift still work
- Unused parameter cast avoids compiler warnings

---

#### 2.2 Extend `IControllerEventHandler` Interface

**Location:** `src/Core/IControllerEventHandler.hpp:21-23`

**Change:** Add `getShiftButtonForButton()` virtual method

```cpp
// Get the button index used as shift modifier
virtual uint8_t getShiftButton() const {
    return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;  // Default
}

// Get the shift button for a specific button (per-button shift support)
virtual uint8_t getShiftButtonForButton(uint8_t button) const {
    (void)button;  // Suppress unused parameter warning
    return getShiftButton();  // Default: use global shift
}
```

**Rationale:**
- Mirrors `IMidiMapper` interface structure
- Allows `SdlManager` to query shift button dynamically
- Default implementation maintains compatibility

---

### Phase 3: FlexibleMapper Implementation

#### 3.1 Declare Method Override

**Location:** `src/Logic/FlexibleMapper.hpp:32`

**Change:** Add method declaration after `getShiftButton()`

```cpp
uint8_t getShiftButton() const override;
uint8_t getShiftButtonForButton(uint8_t button) const override;
```

---

#### 3.2 Implement `getShiftButtonForButton()`

**Location:** `src/Logic/FlexibleMapper.cpp` (after line 299, end of file)

**Change:** Add method implementation

```cpp
uint8_t FlexibleMapper::getShiftButtonForButton(uint8_t button) const {
    if (button >= SDL_CONTROLLER_BUTTON_MAX) {
        return fPreset.shiftButton;
    }
    
    // Check if this button has a specific shift key override
    if (fPreset.buttons[button].shiftButton >= 0) {
        return static_cast<uint8_t>(fPreset.buttons[button].shiftButton);
    }
    
    // Fallback to global shift button
    return fPreset.shiftButton;
}
```

**Logic:**
1. Validate button index
2. Check if button has override (`>= 0`)
3. Return override if present, otherwise global default

**Real-Time Safety:**
- No memory allocation
- Simple array lookup and comparison
- O(1) complexity

---

#### 3.3 Extend JSON Parsing

**Location:** `src/Logic/FlexibleMapper.cpp:206` (after shift axes parsing)

**Change:** Add `buttonShiftKeys` parsing logic

```cpp
// Parse per-button shift key overrides
if (j.contains("buttonShiftKeys")) {
    for (const auto& [key, value] : j["buttonShiftKeys"].items()) {
        int btnIdx = buttonNameToIndex(key);
        if (btnIdx < 0 || btnIdx >= SDL_CONTROLLER_BUTTON_MAX) {
            continue;  // Skip invalid button names
        }
        
        int shiftIdx = buttonNameToIndex(value.get<std::string>());
        if (shiftIdx >= 0 && shiftIdx < SDL_CONTROLLER_BUTTON_MAX) {
            // Set shift button override for both normal and shift configs
            // This ensures consistency regardless of shift state
            fPreset.buttons[btnIdx].shiftButton = static_cast<int8_t>(shiftIdx);
            fPreset.shiftButtons[btnIdx].shiftButton = static_cast<int8_t>(shiftIdx);
        }
    }
}
```

**Rationale:**
- Parses button name → shift button name mapping
- Validates both button and shift button indices
- Sets override for both `buttons` and `shiftButtons` arrays for consistency
- JSON parsing only happens during initialization (not real-time)

**Error Handling:**
- Invalid button names are silently skipped
- Invalid shift button names are silently skipped
- Maintains robustness against malformed presets

---

### Phase 4: Event Dispatcher Updates

#### 4.1 Declare Method in EventDispatcher

**Location:** `src/Core/EventDispatcher.hpp:30` (after `getShiftButton()`)

**Change:** Add method declaration

```cpp
uint8_t getShiftButton() const override;
uint8_t getShiftButtonForButton(uint8_t button) const override;
```

---

#### 4.2 Implement Method in EventDispatcher

**Location:** `src/Core/EventDispatcher.cpp:62` (after `getShiftButton()`)

**Change:** Add method implementation

```cpp
uint8_t EventDispatcher::getShiftButtonForButton(uint8_t button) const {
    if (fMapper) {
        return fMapper->getShiftButtonForButton(button);
    }
    return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
}
```

**Rationale:**
- Simple delegation to mapper
- Fallback to default if no mapper present
- Matches existing pattern for `getShiftButton()`

---

### Phase 5: SdlManager Shift Query Update

#### 5.1 Modify Button Event Handling

**Location:** `src/Core/SdlManager.cpp:155-156`

**Current Code:**
```cpp
uint8_t shiftBtn = fHandler->getShiftButton();
bool shift = SDL_GameControllerGetButton(fController, static_cast<SDL_GameControllerButton>(shiftBtn)) == 1;
```

**New Code:**
```cpp
uint8_t shiftBtn = fHandler->getShiftButtonForButton(button);
bool shift = SDL_GameControllerGetButton(fController, static_cast<SDL_GameControllerButton>(shiftBtn)) == 1;
```

**Change:** Replace `getShiftButton()` with `getShiftButtonForButton(button)`

---

#### 5.2 Keep Axis Event Handling Unchanged

**Location:** `src/Core/SdlManager.cpp:167-168`

**Keep Current Code:**
```cpp
uint8_t shiftBtn = fHandler->getShiftButton();
bool shift = SDL_GameControllerGetButton(fController, static_cast<SDL_GameControllerButton>(shiftBtn)) == 1;
```

**Rationale:**
- User does not require per-axis shift support yet
- Axes use global shift button for simplicity
- Can be extended later if needed

**Optional:** Add comment explaining this decision:
```cpp
// For axes, we use the global shift button (no per-axis shift support yet)
uint8_t shiftBtn = fHandler->getShiftButton();
```

---

### Phase 6: Preset Creation

#### 6.1 Create New Preset File

**File:** `presets/c_major_scale_chords.json`

**Content:**

```json
{
    "name": "C Major Scale & Chords",
    "channel": 0,
    "baseOctaveOffset": 0,
    "shiftButtonName": "rightshoulder",
    
    "buttonShiftKeys": {
        "dpad_up": "leftshoulder",
        "dpad_down": "leftshoulder",
        "dpad_left": "leftshoulder",
        "dpad_right": "leftshoulder"
    },
    
    "buttons": {
        "a": { "mode": "note", "note": 60, "velocity": 100 },
        "b": { "mode": "note", "note": 62, "velocity": 100 },
        "x": { "mode": "note", "note": 64, "velocity": 100 },
        "y": { "mode": "note", "note": 65, "velocity": 100 },
        
        "dpad_up": { "mode": "chord", "note": 60, "velocity": 100, "intervals": [0, 4, 7] },
        "dpad_down": { "mode": "chord", "note": 65, "velocity": 100, "intervals": [0, 4, 7] },
        "dpad_left": { "mode": "chord", "note": 67, "velocity": 100, "intervals": [0, 4, 7] },
        "dpad_right": { "mode": "chord", "note": 69, "velocity": 100, "intervals": [0, 3, 7] }
    },
    
    "shiftButtons": {
        "a": { "mode": "note", "note": 67, "velocity": 100 },
        "b": { "mode": "note", "note": 69, "velocity": 100 },
        "x": { "mode": "note", "note": 71, "velocity": 100 },
        "y": { "mode": "note", "note": 72, "velocity": 100 },
        
        "dpad_up": { "mode": "chord", "note": 64, "velocity": 100, "intervals": [0, 3, 7] },
        "dpad_down": { "mode": "chord", "note": 62, "velocity": 100, "intervals": [0, 3, 7] },
        "dpad_left": { "mode": "chord", "note": 71, "velocity": 100, "intervals": [0, 3, 6] },
        "dpad_right": { "mode": "chord", "note": 64, "velocity": 100, "intervals": [0, 4, 7] }
    }
}
```

---

#### 6.2 Button Mapping Reference

**ABXY Buttons (Right Shoulder as Shift):**

| Button | No Shift | With Shift (Right Shoulder) | Note |
|--------|----------|----------------------------|------|
| A | C4 (60) | G4 (67) | C Major scale |
| B | D4 (62) | A4 (69) | |
| X | E4 (64) | B4 (71) | |
| Y | F4 (65) | C5 (72) | Octave up |

**D-Pad Buttons (Left Shoulder as Shift):**

| Button | No Shift | Chord | With Shift (Left Shoulder) | Chord | Notes |
|--------|----------|-------|---------------------------|-------|-------|
| Up | I | C Major (C-E-G) | iii | E minor (E-G-B) | 60-64-67 / 64-67-71 |
| Down | IV | F Major (F-A-C) | ii | D minor (D-F-A) | 65-69-72 / 62-65-69 |
| Left | V | G Major (G-B-D) | vii° | B dim (B-D-F) | 67-71-74 / 71-74-77 |
| Right | vi | A minor (A-C-E) | III | E Major (E-G♯-B) | 69-72-76 / 64-68-71 |

**Chord Interval Notation:**
- `[0, 4, 7]` = Major chord (root + major 3rd + perfect 5th)
- `[0, 3, 7]` = Minor chord (root + minor 3rd + perfect 5th)
- `[0, 3, 6]` = Diminished chord (root + minor 3rd + diminished 5th)
- `[0, 4, 8]` = Augmented chord (not used in this preset)

**MIDI Note Numbers:**
- C4 = 60, D4 = 62, E4 = 64, F4 = 65, G4 = 67, A4 = 69, B4 = 71, C5 = 72
- G♯4 = 68 (used in III Major chord, non-diatonic)

---

#### 6.3 Update CMakeLists.txt

**Location:** Find existing `embed_preset()` calls in CMakeLists.txt

**Change:** Add new preset embedding

```cmake
embed_preset("c_major_scale_chords" "${CMAKE_CURRENT_SOURCE_DIR}/presets/c_major_scale_chords.json")
```

**Location in Build Output:**
- Generated header: `build/generated/preset_c_major_scale_chords.hpp`
- Contains: `const uint8_t preset_c_major_scale_chords_data[]` and `preset_c_major_scale_chords_size`

---

#### 6.4 Update Plugin.cpp to Load New Preset

**Location:** `src/Plugin.cpp` (find existing preset loading code)

**Current:**
```cpp
#include "preset_c_major_chords.hpp"
// ...
mapper->loadPreset(preset_c_major_chords_data, preset_c_major_chords_size);
```

**Change to:**
```cpp
#include "preset_c_major_scale_chords.hpp"
// ...
mapper->loadPreset(preset_c_major_scale_chords_data, preset_c_major_scale_chords_size);
```

**Alternative:** Keep both presets and add parameter to switch between them (future enhancement)

---

### Phase 7: Documentation Update

#### 7.1 Update AGENTS.md

**Location:** `AGENTS.md` (add new section or update existing section)

**New Section:**

```markdown
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

#### Best Practices

1. **Avoid Shift Key Conflicts:** Don't assign functionality to buttons that are used as shift keys to prevent ambiguous behavior
2. **Ergonomic Grouping:** Group related buttons under the same shift key (e.g., all face buttons, all d-pad buttons)
3. **Visual Consistency:** Consider physical layout when assigning shift keys for better muscle memory

#### JSON Schema Extension

| Field | Type | Description |
|-------|------|-------------|
| `buttonShiftKeys` | Object | Maps button names to their shift button names |
| `buttonShiftKeys.<button>` | String | Button name (e.g., "dpad_up", "a", "b") |
| Value | String | Shift button name (e.g., "leftshoulder", "rightshoulder") |

#### Backward Compatibility

All existing presets without `buttonShiftKeys` continue to work unchanged, using the global `shiftButton`.
```

---

### Phase 8: Testing Plan

#### 8.1 Build Verification

**Steps:**
1. Clean build directory: `rm -rf build`
2. Configure: `cmake -B build -G Ninja -DCMAKE_TOOLCHAIN_FILE=<vcpkg>/scripts/buildsystems/vcpkg.cmake -DCMAKE_BUILD_TYPE=Release`
3. Build: `cmake --build build`
4. Verify outputs:
   - `build/generated/preset_c_major_scale_chords.hpp` exists
   - `build/bin/GameControllerMIDI.vst3` builds successfully
   - No compilation errors or warnings

---

#### 8.2 Functional Testing

**Test Case 1: ABXY + Right Shoulder (Scale Notes)**

| Action | Expected MIDI | Note |
|--------|--------------|------|
| Press A | Note On, Ch 0, Note 60 (C4), Vel 100 | |
| Release A | Note Off, Ch 0, Note 60 | |
| Hold Right Shoulder, Press A | Note On, Ch 0, Note 67 (G4), Vel 100 | |
| Hold Right Shoulder + A, Release A | Note Off, Ch 0, Note 67 | |
| Press B | Note On, Ch 0, Note 62 (D4), Vel 100 | |
| Hold Right Shoulder, Press X | Note On, Ch 0, Note 71 (B4), Vel 100 | |
| Hold Right Shoulder, Press Y | Note On, Ch 0, Note 72 (C5), Vel 100 | Octave up |

**Test Case 2: D-Pad + Left Shoulder (Chords)**

| Action | Expected MIDI | Notes |
|--------|--------------|-------|
| Press Up | 3x Note On: 60, 64, 67 (C Major) | |
| Release Up | 3x Note Off: 60, 64, 67 | |
| Hold Left Shoulder, Press Up | 3x Note On: 64, 67, 71 (E minor) | |
| Hold Left Shoulder, Press Down | 3x Note On: 62, 65, 69 (D minor) | |
| Hold Left Shoulder, Press Left | 3x Note On: 71, 74, 77 (B dim) | vii° chord |
| Hold Left Shoulder, Press Right | 3x Note On: 64, 68, 71 (E Major) | Non-diatonic |

**Test Case 3: Cross-Shift Isolation**

| Action | Expected MIDI | Verification |
|--------|--------------|--------------|
| Hold Left Shoulder, Press A | Note On, Ch 0, Note 60 (C4) | Left shoulder does NOT shift ABXY |
| Hold Right Shoulder, Press Up | 3x Note On: 60, 64, 67 (C Major) | Right shoulder does NOT shift D-pad |

**Test Case 4: Backward Compatibility**

Load the old `c_major_chords.json` preset and verify:
- All buttons use right shoulder as global shift
- No errors or crashes
- Existing mappings work unchanged

---

#### 8.3 Edge Case Testing

**Test Case 5: Simultaneous Shift Keys**

| Action | Expected Behavior |
|--------|------------------|
| Hold both shoulders, press A | Should use right shoulder shift (global default for ABXY) |
| Hold both shoulders, press Up | Should use left shoulder shift (per-button override) |

**Test Case 6: Rapid Shift Transitions**

| Action | Expected Behavior |
|--------|------------------|
| Press A, quickly press Right Shoulder before release | Note Off for 60, then Note On for 67 (two separate notes) |
| Hold Right Shoulder, press A rapidly 10 times | 10 pairs of Note On/Off for note 67 |

**Test Case 7: Shift Key as Functional Button**

*Note: Shift keys should NOT have their own functionality in this preset, but verify graceful behavior if misconfigured.*

| Action | Expected Behavior |
|--------|------------------|
| Press Right Shoulder alone | No MIDI output (not mapped) |
| Press Left Shoulder alone | No MIDI output (not mapped) |

---

#### 8.4 Performance Testing

**Metrics to Verify:**
- No audio dropouts or glitches during rapid button presses
- SDL event loop maintains < 1ms latency
- MIDI queue never overflows (check log for warnings)
- CPU usage remains low (< 5% during typical use)

**Stress Test:**
- Mash all buttons randomly for 30 seconds
- Verify no crashes, hangs, or MIDI stuck notes

---

## Implementation Checklist

### Code Modifications

- [ ] `src/Logic/MapperConfig.hpp` - Add `ButtonConfig::shiftButton` field
- [ ] `src/Logic/IMidiMapper.hpp` - Add `getShiftButtonForButton()` virtual method
- [ ] `src/Logic/FlexibleMapper.hpp` - Declare `getShiftButtonForButton()` override
- [ ] `src/Logic/FlexibleMapper.cpp` - Implement method and JSON parsing
- [ ] `src/Core/IControllerEventHandler.hpp` - Add `getShiftButtonForButton()` virtual method
- [ ] `src/Core/EventDispatcher.hpp` - Declare `getShiftButtonForButton()`
- [ ] `src/Core/EventDispatcher.cpp` - Implement `getShiftButtonForButton()` delegation
- [ ] `src/Core/SdlManager.cpp` - Update button handler to use `getShiftButtonForButton()`

### Preset and Build

- [ ] `presets/c_major_scale_chords.json` - Create new preset file
- [ ] `CMakeLists.txt` - Add preset embedding
- [ ] `src/Plugin.cpp` - Update to load new preset

### Documentation

- [ ] `AGENTS.md` - Add per-button shift key documentation
- [ ] `.opencode/plans/multi_shift_key_support.md` - This document (completed)

### Testing

- [ ] Clean build verification
- [ ] Functional test: ABXY + right shoulder
- [ ] Functional test: D-pad + left shoulder
- [ ] Functional test: Cross-shift isolation
- [ ] Functional test: Backward compatibility with old preset
- [ ] Edge case: Simultaneous shift keys
- [ ] Edge case: Rapid shift transitions
- [ ] Performance: Stress test with rapid input

---

## Risk Assessment and Mitigation

### Risk 1: Backward Compatibility Break

**Likelihood:** Low  
**Impact:** High  
**Mitigation:**
- Default value `-1` for `ButtonConfig::shiftButton` ensures fallback to global
- Default implementations in base interfaces maintain existing behavior
- Explicit testing with old presets

---

### Risk 2: Shift Key Conflicts

**Scenario:** User assigns functionality to left shoulder button while also using it as a shift key

**Likelihood:** Medium  
**Impact:** Medium (confusing behavior)  
**Mitigation:**
- Document best practice: shift keys should not have other functions
- Future enhancement: Add validation in JSON parser to warn about conflicts

---

### Risk 3: Real-Time Safety Violation

**Concern:** New code introduces latency or memory allocation

**Likelihood:** Very Low  
**Impact:** High (audio glitches)  
**Mitigation:**
- All new fields are fixed-size POD types
- JSON parsing only during initialization
- `getShiftButtonForButton()` is O(1) array lookup
- No locks or allocations in event path

---

### Risk 4: UI Display Confusion

**Scenario:** UI shows only one shift key highlighted, but two are active

**Likelihood:** High  
**Impact:** Low (cosmetic only)  
**Mitigation:**
- Current implementation: Accept this limitation for now
- Future enhancement: Extend UI to show all active shift keys
- Document current UI limitation in AGENTS.md

---

## Future Enhancements

### Per-Axis Shift Keys

Extend the same pattern to axes:
- Add `AxisConfig::shiftButton` field
- Modify `handleControllerAxis()` to use `getShiftButtonForButton(axis)`

**Use Case:** Left stick + left shoulder for mod wheel, right stick + right shoulder for pitch bend

---

### Multi-Layer Shift

Support multiple shift layers (Shift1, Shift2, Shift1+Shift2):
- Extend `MapperPreset` to have `buttons`, `shift1Buttons`, `shift2Buttons`, `shift1And2Buttons`
- Modify `onButton()` to check multiple shift states
- JSON format: `"shift1Buttons"`, `"shift2Buttons"`, etc.

**Use Case:** Right shoulder = scale variations, left shoulder = chords, both = extended techniques

---

### Shift Key Chaining

Support combinations like "A + B = shift for C":
- More complex state tracking
- Requires refactoring shift state from bool to bitmask
- Higher implementation complexity

---

### Runtime Preset Switching

Add UI controls to switch between multiple embedded presets:
- Extend plugin parameters to include preset selector
- Implement thread-safe preset switching in EventDispatcher
- Preserve octave offset and other state during switch

---

### MIDI Learn Mode

Allow users to assign controller buttons to MIDI functions interactively:
- Enter learn mode via plugin parameter
- Press controller button, then trigger MIDI function
- Save learned mapping to user preset directory

---

## Conclusion

This implementation plan provides a **comprehensive, backward-compatible solution** for per-button shift key support in the GameControllerMIDI plugin. The design:

- ✅ Satisfies user requirements (ABXY + right shoulder, D-pad + left shoulder)
- ✅ Maintains real-time safety (no allocations, locks, or blocking calls)
- ✅ Preserves backward compatibility (existing presets work unchanged)
- ✅ Follows modern C++23 best practices
- ✅ Extends naturally to future enhancements (per-axis shift, multi-layer)

**Estimated Implementation Time:** 2-4 hours  
**Estimated Testing Time:** 1-2 hours  
**Total Effort:** 3-6 hours

**Next Step:** Review this plan with user, address any questions, then proceed to implementation phase.

---

## Appendix: JSON Schema Definition

```json
{
  "$schema": "http://json-schema.org/draft-07/schema#",
  "title": "FlexibleMapper Preset with Per-Button Shift Keys",
  "type": "object",
  "properties": {
    "name": { "type": "string" },
    "channel": { "type": "integer", "minimum": 0, "maximum": 15 },
    "baseOctaveOffset": { "type": "integer", "minimum": -4, "maximum": 4 },
    "shiftButton": { "type": "integer", "minimum": 0, "maximum": 14 },
    "shiftButtonName": { "type": "string", "enum": ["leftshoulder", "rightshoulder", "back", "guide", "start"] },
    
    "buttonShiftKeys": {
      "type": "object",
      "additionalProperties": { 
        "type": "string",
        "enum": ["a", "b", "x", "y", "back", "guide", "start", "leftstick", "rightstick", "leftshoulder", "rightshoulder", "dpad_up", "dpad_down", "dpad_left", "dpad_right"]
      }
    },
    
    "buttons": { "$ref": "#/definitions/buttonMap" },
    "shiftButtons": { "$ref": "#/definitions/buttonMap" },
    "axes": { "$ref": "#/definitions/axisMap" },
    "shiftAxes": { "$ref": "#/definitions/axisMap" }
  },
  
  "definitions": {
    "buttonMap": {
      "type": "object",
      "additionalProperties": {
        "type": "object",
        "properties": {
          "mode": { "type": "string", "enum": ["none", "note", "chord", "cc_momentary", "cc_toggle", "octave_up", "octave_down"] },
          "note": { "type": "integer", "minimum": 0, "maximum": 127 },
          "cc": { "type": "integer", "minimum": 0, "maximum": 127 },
          "velocity": { "type": "integer", "minimum": 0, "maximum": 127 },
          "intervals": { "type": "array", "items": { "type": "integer" } }
        }
      }
    },
    "axisMap": {
      "type": "object",
      "additionalProperties": {
        "type": "object",
        "properties": {
          "mode": { "type": "string", "enum": ["none", "cc", "pitchbend", "aftertouch"] },
          "cc": { "type": "integer", "minimum": 0, "maximum": 127 },
          "bipolar": { "type": "boolean" },
          "inverted": { "type": "boolean" },
          "deadzone": { "type": "number", "minimum": 0, "maximum": 1 }
        }
      }
    }
  }
}
```

---

**Document Version:** 1.0  
**Author:** OpenCode AI Assistant  
**Reviewed By:** (Pending user review)  
**Approval Status:** Pending
