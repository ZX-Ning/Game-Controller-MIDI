# Architecture

Detailed documentation of core components, threading model, and state management.

## Component Overview

```
┌─────────────────────────────────────────────────────────────────┐
│                        Host (DAW)                               │
└─────────────────────────────────────────────────────────────────┘
                              │
                              ▼
┌─────────────────────────────────────────────────────────────────┐
│                  GameControllerMIDIPlugin                       │
│                     (DPF Entry Point)                           │
│  ┌──────────────┐  ┌──────────────┐  ┌───────────────────────┐ │
│  │ State System │  │   run()      │  │   EventDispatcher     │ │
│  │ (Persistence)│  │ (MIDI out)   │  │   (MIDI Queue)        │ │
│  └──────────────┘  └──────────────┘  └───────────────────────┘ │
└─────────────────────────────────────────────────────────────────┘
                              │
          ┌───────────────────┴───────────────────┐
          ▼                                       ▼
┌──────────────────┐                   ┌──────────────────┐
│   SdlManager     │                   │ GameControllerUI │
│ (Hardware Thread)│                   │   (ImGui/DGL)    │
└──────────────────┘                   └──────────────────┘
          │
          ▼
┌──────────────────┐
│  FlexibleMapper  │
│ (MIDI Translator)│
└──────────────────┘
```

## Threading Model

| Thread | Responsibility | Real-time Safe? |
|--------|----------------|-----------------|
| **Audio Thread** | `Plugin::run()` — pops from MIDI queue, writes to host | Yes (must be) |
| **SDL Thread** | `SdlManager` — polls controller, pushes to queue | No (background) |
| **UI Thread** | `GameControllerMIDIUI` — renders ImGui | No |

**Critical**: The audio thread must never block. All cross-thread communication uses `boost::lockfree::queue`.

---

## Components

### SdlManager (Hardware Poller)

**File**: `src/Core/SdlManager.hpp`, `SdlManager.cpp`

A singleton managing the SDL lifecycle and hardware polling thread.

**Responsibilities**:
- Global `SDL_Init()` / `SDL_Quit()` lifecycle
- Background thread running `SDL_PollEvent()` loop
- Forwards button/axis events to `IControllerEventHandler`
- Knows nothing about MIDI — pure hardware abstraction

**Key Methods**:
- `getInstance()` — singleton access
- `setEventHandler(IControllerEventHandler*)` — register event receiver

---

### EventDispatcher (Router)

**File**: `src/Core/EventDispatcher.hpp`, `EventDispatcher.cpp`

Implements `IControllerEventHandler`. Central hub connecting hardware to MIDI output.

**Responsibilities**:
- Tracks controller connection state and name
- Maintains button states array for UI feedback
- Owns and delegates to the active `IMidiMapper`
- Manages `boost::lockfree::queue<RawMidi>` (SDL thread → audio thread)
- **Trigger octave control**: LT/RT edge detection for octave shifting

**Trigger Octave Logic**:
```cpp
constexpr float TRIGGER_THRESHOLD = 0.5f;

// LT (axis 4): increment octave when crossing threshold upward
// RT (axis 5): decrement octave when crossing threshold upward
// Range: -4 to +4 (clamped)
```

**Key Methods**:
- `popMidi(RawMidi&)` — called by audio thread
- `setMapper(std::unique_ptr<IMidiMapper>)` — install mapper
- `getMapper()` — access current mapper
- `isConnected()`, `getControllerName()` — UI queries
- `getButtonStates()` — UI feedback array

---

### IMidiMapper (Interface)

**File**: `src/Logic/IMidiMapper.hpp`

Pure interface for MIDI translation. Implement this to create custom mapping logic.

**Required Methods**:
```cpp
virtual void onButtonEvent(int button, bool pressed, MidiCallback cb) = 0;
virtual void onAxisEvent(int axis, float value, MidiCallback cb) = 0;
virtual int getOctaveOffset() const = 0;
virtual int getShiftButton() const = 0;
virtual int getTriggerOctaveOffset() const = 0;
virtual void setTriggerOctaveOffset(int offset) = 0;
```

---

### FlexibleMapper (Implementation)

**File**: `src/Logic/FlexibleMapper.hpp`, `FlexibleMapper.cpp`

Configurable mapper loaded from JSON presets.

**Features**:
- Multiple button modes: Note, Chord, CC (momentary/toggle), Octave shifts
- Multiple axis modes: CC, Pitch Bend, Aftertouch
- Per-button shift key support via `buttonShiftKeys` map
- **Real-time safe**: Fixed-size `std::array` for all collections
- Axis value caching to reduce redundant MIDI traffic

**Octave Handling**:
- `baseOctaveOffset` — set in preset JSON
- `octaveOffset_` — runtime adjustment via OctaveUp/OctaveDown buttons
- `triggerOctaveOffset_` — cumulative LT/RT offset (melody notes only)
- `chordOctaveOffset` — per-chord offset (does NOT include trigger octave)

**Key Methods**:
- `loadPreset(const char* json)` — parse embedded JSON
- `setPreset(const MapperPreset&)` — apply config directly
- `getPreset()` — retrieve current config (for serialization)

---

### GameControllerMIDIPlugin (Orchestrator)

**File**: `src/Plugin.hpp`, `Plugin.cpp`

DPF plugin entry point. Manages lifecycle and state persistence.

**Responsibilities**:
- Owns `EventDispatcher` instance
- Audio thread: `run()` pops MIDI events and writes to host
- State system: persists configuration across sessions

#### State Management

The plugin uses DPF's state system for persistence:

| State Key | Type | Description |
|-----------|------|-------------|
| `config` | JSON string | Full mapper preset (buttons, axes, metadata) |
| `triggerOctave` | Integer (-4 to +4) | Current trigger octave offset |
| `editMode` | Boolean | UI mode (true = Edit, false = Play) |
| `width` | Integer | Window width in pixels |
| `height` | Integer | Window height in pixels |

**State Flow**:
```
UI setState() → Plugin::setState() → reloadMapper() / update atomic
                                   ↓
Host saves state                   UI reads via getPluginInstancePointer()
                                   ↓
Host loads state → Plugin::setState() → restore config
```

**Important**: Do NOT use `kStateIsOnlyForDSP` for states the UI modifies — it blocks UI→DSP communication.

---

### GameControllerMIDIUI (Observer)

**File**: `src/UI.cpp`

ImGui-based user interface with dual-mode design.

**Modes**:

| Mode | Purpose | Contents |
|------|---------|----------|
| **Play** | Live performance | Controller status, active notes display, trigger octave indicator |
| **Edit** | Configuration | Full button/axis mapping tables, scrollable content, pinned action buttons |

**Window Behavior**:
- User-resizable (`DISTRHO_UI_USER_RESIZABLE = 1`)
- Size persisted via `width`/`height` states
- ImGui window fills plugin area (WindowPadding = 0)

**Thread Safety**:
- Reads dispatcher state via `getPluginInstancePointer()`
- Uses `std::atomic` for edit mode flag
- State changes via DPF `setState()` callback

---

## Data Flow

### Button Press → MIDI Note

```
1. SDL Thread: SDL_PollEvent() detects button press
2. SDL Thread: SdlManager calls handler->onButtonDown(button)
3. SDL Thread: EventDispatcher calls mapper->onButtonEvent(button, true, callback)
4. SDL Thread: FlexibleMapper generates Note On, calls callback(RawMidi)
5. SDL Thread: EventDispatcher pushes to lockfree queue
6. Audio Thread: Plugin::run() pops from queue
7. Audio Thread: Plugin writes MidiEvent to host output
```

### LT/RT → Trigger Octave

```
1. SDL Thread: Axis event for LT (axis 4) or RT (axis 5)
2. SDL Thread: EventDispatcher detects threshold crossing (0.5)
3. SDL Thread: Calls mapper->setTriggerOctaveOffset(newValue)
4. SDL Thread: Calls plugin setState("triggerOctave", value)
5. Future notes use updated offset (melody only, not chords)
```

---

## Dependency Management

### External (vcpkg)

- **SDL2**: Controller abstraction
- **Boost.Lockfree**: Thread-safe queue
- **nlohmann-json**: Preset parsing

### Submodules (third_party/)

- **DPF**: DISTRHO Plugin Framework
- **DPF-Widgets**: ImGui integration for DPF

### Preset Embedding

JSON presets are embedded at compile time via `cmake/bin2c.cmake`:
1. JSON file → C byte array header
2. `#include "preset_*.hpp"` in Plugin.cpp
3. Call `mapper->loadPreset(preset_data)` at init
