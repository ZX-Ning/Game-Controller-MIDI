# Configuration Reference

Complete reference for configuration structures, enums, name mappings, and JSON preset schema.

## Button Modes

Defined in `src/Logic/MapperConfig.hpp` as `MapperConfig::ButtonMode`:

| Mode | Description | MIDI Output |
|------|-------------|-------------|
| `None` | No action | — |
| `Note` | Single MIDI note | Note On (press) / Note Off (release) |
| `Chord` | Multiple notes | Note On for root + intervals (press) / Note Off (release) |
| `CC_Momentary` | Control Change momentary | CC value 127 (press) / 0 (release) |
| `CC_Toggle` | Control Change toggle | CC toggles between 127 and 0 on each press |
| `OctaveUp` | Increment octave | No MIDI — adjusts internal offset +1 |
| `OctaveDown` | Decrement octave | No MIDI — adjusts internal offset -1 |

## Axis Modes

Defined in `src/Logic/MapperConfig.hpp` as `MapperConfig::AxisMode`:

| Mode | Description | MIDI Output | Range |
|------|-------------|-------------|-------|
| `None` | No action | — | — |
| `CC` | Control Change | CC message | 0-127 (7-bit) |
| `PitchBend` | Pitch Bend | Pitch Bend message | 0-16383 (14-bit) |
| `Aftertouch` | Channel Aftertouch | Aftertouch message | 0-127 (7-bit) |

## Button Name Mappings

Use these names in JSON presets under the `buttons` object:

| JSON Name | SDL Constant | Physical Button |
|-----------|--------------|-----------------|
| `a` | `SDL_CONTROLLER_BUTTON_A` | A / Cross |
| `b` | `SDL_CONTROLLER_BUTTON_B` | B / Circle |
| `x` | `SDL_CONTROLLER_BUTTON_X` | X / Square |
| `y` | `SDL_CONTROLLER_BUTTON_Y` | Y / Triangle |
| `back` | `SDL_CONTROLLER_BUTTON_BACK` | Back / Select |
| `guide` | `SDL_CONTROLLER_BUTTON_GUIDE` | Guide / Home |
| `start` | `SDL_CONTROLLER_BUTTON_START` | Start / Options |
| `leftstick` | `SDL_CONTROLLER_BUTTON_LEFTSTICK` | Left Stick Click |
| `rightstick` | `SDL_CONTROLLER_BUTTON_RIGHTSTICK` | Right Stick Click |
| `leftshoulder` | `SDL_CONTROLLER_BUTTON_LEFTSHOULDER` | LB / L1 |
| `rightshoulder` | `SDL_CONTROLLER_BUTTON_RIGHTSHOULDER` | RB / R1 |
| `dpad_up` | `SDL_CONTROLLER_BUTTON_DPAD_UP` | D-Pad Up |
| `dpad_down` | `SDL_CONTROLLER_BUTTON_DPAD_DOWN` | D-Pad Down |
| `dpad_left` | `SDL_CONTROLLER_BUTTON_DPAD_LEFT` | D-Pad Left |
| `dpad_right` | `SDL_CONTROLLER_BUTTON_DPAD_RIGHT` | D-Pad Right |

## Axis Name Mappings

Use these names in JSON presets under the `axes` object:

| JSON Name | SDL Constant | Physical Axis |
|-----------|--------------|---------------|
| `leftx` | `SDL_CONTROLLER_AXIS_LEFTX` | Left Stick Horizontal |
| `lefty` | `SDL_CONTROLLER_AXIS_LEFTY` | Left Stick Vertical |
| `rightx` | `SDL_CONTROLLER_AXIS_RIGHTX` | Right Stick Horizontal |
| `righty` | `SDL_CONTROLLER_AXIS_RIGHTY` | Right Stick Vertical |
| `triggerleft` | `SDL_CONTROLLER_AXIS_TRIGGERLEFT` | LT / L2 |
| `triggerright` | `SDL_CONTROLLER_AXIS_TRIGGERRIGHT` | RT / R2 |

**Note**: `triggerleft` and `triggerright` are reserved for trigger octave control and should not be mapped to MIDI axes.

## JSON Preset Schema

### Full Example

```json
{
  "name": "C Major Scale + Chords",
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
    "a": {
      "mode": "note",
      "note": 60,
      "velocity": 100
    },
    "b": {
      "mode": "chord",
      "note": 62,
      "velocity": 100,
      "intervals": [0, 3, 7],
      "chordOctaveOffset": -1
    },
    "dpad_up": {
      "mode": "octave_up"
    },
    "x": {
      "mode": "cc_momentary",
      "cc": 64,
      "velocity": 127
    },
    "y": {
      "mode": "cc_toggle",
      "cc": 65
    }
  },
  
  "axes": {
    "leftx": {
      "mode": "cc",
      "cc": 1,
      "bipolar": true,
      "deadzone": 0.15
    },
    "lefty": {
      "mode": "pitchbend",
      "bipolar": true,
      "deadzone": 0.1
    },
    "rightx": {
      "mode": "aftertouch",
      "bipolar": false,
      "deadzone": 0.05
    }
  }
}
```

### Top-Level Fields

| Field | Type | Required | Description |
|-------|------|----------|-------------|
| `name` | string | Yes | Preset display name |
| `channel` | int (0-15) | Yes | MIDI channel |
| `baseOctaveOffset` | int | No | Starting octave offset (default: 0) |
| `shiftButton` | int | No* | Shift button by SDL index |
| `shiftButtonName` | string | No* | Shift button by name (e.g., `"rightshoulder"`) |
| `buttonShiftKeys` | object | No | Per-button shift key overrides |
| `buttons` | object | Yes | Button mappings (key = button name) |
| `axes` | object | No | Axis mappings (key = axis name) |

*Use either `shiftButton` OR `shiftButtonName`, not both.

### Button Configuration Fields

| Field | Type | Modes | Description |
|-------|------|-------|-------------|
| `mode` | string | All | `"note"`, `"chord"`, `"cc_momentary"`, `"cc_toggle"`, `"octave_up"`, `"octave_down"` |
| `note` | int (0-127) | note, chord | MIDI note number (60 = C4) |
| `velocity` | int (0-127) | note, chord, cc_momentary | Velocity or CC value |
| `intervals` | int[] | chord | Semitone intervals from root (e.g., `[0, 4, 7]` = major triad) |
| `chordOctaveOffset` | int | chord | Octave offset for this chord only (does not include trigger octave) |
| `cc` | int (0-127) | cc_momentary, cc_toggle | CC number |

### Axis Configuration Fields

| Field | Type | Description |
|-------|------|-------------|
| `mode` | string | `"cc"`, `"pitchbend"`, `"aftertouch"` |
| `cc` | int (0-127) | CC number (required for `cc` mode) |
| `bipolar` | bool | If true, center = 0, extremes = ±max. If false, 0 to max. |
| `deadzone` | float (0-1) | Dead zone around center (default: 0.1) |

## Per-Button Shift Key Support

By default, all buttons use the global `shiftButton`. You can override this per-button using `buttonShiftKeys`:

```json
{
  "shiftButtonName": "rightshoulder",
  
  "buttonShiftKeys": {
    "dpad_up": "leftshoulder",
    "dpad_down": "leftshoulder",
    "dpad_left": "leftshoulder",
    "dpad_right": "leftshoulder"
  }
}
```

**Behavior**:
- Buttons listed in `buttonShiftKeys` use the specified shift key
- Buttons NOT listed use the global `shiftButton`
- Each button can have its own independent shift key
- Axes always use the global shift button

**Use Case**: Assign face buttons (ABXY) to right shoulder shift for melody, D-pad to left shoulder shift for chords — both active simultaneously.

## Octave System

The final MIDI note is calculated as:

```
finalNote = baseNote + (baseOctaveOffset * 12) + (octaveOffset * 12) + (triggerOctaveOffset * 12)
```

For chords specifically:

```
finalNote = baseNote + (baseOctaveOffset * 12) + (octaveOffset * 12) + (chordOctaveOffset * 12)
// Note: triggerOctaveOffset does NOT apply to chords
```

| Offset | Source | Affects |
|--------|--------|---------|
| `baseOctaveOffset` | Preset JSON | All notes and chords |
| `octaveOffset` | OctaveUp/OctaveDown buttons | All notes and chords |
| `triggerOctaveOffset` | LT/RT triggers | Melody notes only (NOT chords) |
| `chordOctaveOffset` | Per-button JSON | That specific chord only |
