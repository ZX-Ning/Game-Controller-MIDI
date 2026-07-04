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
| **SDL Thread** | `SdlManager` — polls controller and dispatches events to mapper/dispatcher | No (background) |
| **UI Thread** | `GameControllerMIDIUI` — renders ImGui | No |

**Critical**: The audio thread must never block. MIDI events cross into the audio thread through `boost::lockfree::queue`; small UI/host-visible control values use atomics in dispatcher-owned shared state.

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
- Owns `SharedState`, the single source of truth for base and trigger octave offsets
- **Trigger octave control**: LT/RT edge detection for octave shifting

**Trigger Octave Logic**:
```cpp
constexpr float TRIGGER_THRESHOLD = 0.5f;

// LT (axis 4): decrement trigger octave when crossing threshold upward
// RT (axis 5): increment trigger octave when crossing threshold upward
// Range: -4 to +4 (clamped)
```

**Key Methods**:
- `popMidi(RawMidi&)` — called by audio thread
- `setMapper(std::unique_ptr<IMidiMapper>)` — install mapper
- `isConnected()`, `getControllerName()` — UI queries
- `getButtonState()` — UI feedback
- `sharedState()` — UI/host-visible shared mapper control state

---

### SharedState (Octave State)

**File**: `src/Common/SharedState.hpp`

Dispatcher-owned shared state for live octave offsets.

**Responsibilities**:
- Owns current base octave and trigger octave as atomics
- Clamps octave values to `-4..+4`
- Provides octave snapshots for mapper note/chord calculations
- Tracks whether mapper/controller changed base octave and the host parameter needs synchronization

`FlexibleMapper` receives a `SharedState&` during button mapping, so mapper modes such as `OctaveUp` and `OctaveDown` can synchronously update the single source of truth. The mapper must not store the reference.

---

### IMidiMapper (Interface)

**File**: `src/Logic/IMidiMapper.hpp`

Pure interface for MIDI translation. Implement this to create custom mapping logic.

**Required Methods**:
```cpp
virtual void onButton(uint8_t button, bool pressed, bool shiftState,
                      SharedState& state, IMidiOutputSink& out) = 0;
virtual void onAxis(uint8_t axis, int16_t value, bool shiftState,
                    const SharedState& state, IMidiOutputSink& out) = 0;
virtual bool flushActiveNotes(IMidiOutputSink& out) = 0;
```

---

### FlexibleMapper (Implementation)

**File**: `src/Logic/FlexibleMapper.hpp`, `FlexibleMapper.cpp`

Configurable mapper loaded from JSON presets.

**Features**:
- Multiple button modes: Note, Chord, CC (momentary/toggle), Octave shifts
- Multiple axis modes: CC and Pitch Bend
- Per-button shift key support via `buttonShiftKeys` map
- **Real-time safe**: Fixed-size `std::array` for all collections
- Axis value caching to reduce redundant MIDI traffic

**Octave Handling**:
- `baseOctaveOffset` — preset's initial base octave, copied into `SharedState` when the mapper is installed
- Runtime base octave — owned by `SharedState`; `OctaveUp/OctaveDown` modes update it synchronously
- Trigger octave — owned by `SharedState`; LT/RT edges update it in `EventDispatcher`
- `chordOctaveOffset` — per-chord offset (does NOT include trigger octave)
- Active notes store the exact pitches that were sent, so Note Off still matches after octave changes

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
UI setState() → Plugin::setState() → reloadMapper() / update SharedState
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
3. SDL Thread: EventDispatcher calls mapper->onButton(..., SharedState&, fMidiEvents)
4. SDL Thread: FlexibleMapper reads an octave snapshot and queues RawMidi
5. SDL Thread: MidiEventQueue stores RawMidi in lock-free queues
6. Audio Thread: Plugin::run() pops from queue
7. Audio Thread: Plugin writes MidiEvent to host output
```

### LT/RT → Trigger Octave

```
1. SDL Thread: Axis event for LT (axis 4) or RT (axis 5)
2. SDL Thread: EventDispatcher detects threshold crossing (0.5)
3. SDL Thread: Updates dispatcher-owned `SharedState`
4. UI/state reads the new trigger octave through EventDispatcher
5. Future melody notes use updated offset; chord notes ignore trigger octave
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
