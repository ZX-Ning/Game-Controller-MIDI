#pragma once

#include <SDL.h>

#include <array>
#include <string_view>

#include "IMidiMapper.hpp"
#include "MapperConfig.hpp"

namespace GCMidi {

/**
 * JSON-configurable mapper from SDL button/axis events to MIDI.
 *
 * The mapper owns preset and runtime note/CC/axis state, but does not lock
 * internally. `EventDispatcher` serializes all access with `fMapperMutex`.
 * Event paths use fixed-size arrays only and should remain allocation-free.
 */
class FlexibleMapper : public IMidiMapper {
public:
    /** Construct a mapper with default preset/runtime state. */
    FlexibleMapper();
    ~FlexibleMapper() override = default;

    /** Return the active preset name; pointer is owned by `fPreset`. */
    const char* getName() const override {
        return fPreset.name.data();
    }

    /** Load preset JSON and reset runtime state. Uses allocation; not real-time safe. */
    bool loadPreset(const uint8_t* jsonData, size_t jsonSize);

    /** Return the active preset; reference is stable until the next `setPreset()`. */
    const MapperConfig::MapperPreset& getPreset() const {
        return fPreset;
    }

    /** Replace the preset and reset active notes, toggles, and axis caches. */
    void setPreset(const MapperConfig::MapperPreset& preset);

    /** Map a button edge to note, chord, or CC MIDI. Serialized by owner. */
    void onButton(uint8_t button, bool pressed, bool shift, IMidiOutputSink& out) override;

    /** Map axis movement to CC, pitch bend, or aftertouch MIDI. */
    void onAxis(uint8_t axis, int16_t value, bool shift, IMidiOutputSink& out) override;

    /** Emit Note Off for tracked notes; failed sends remain tracked. */
    bool flushActiveNotes(IMidiOutputSink& out) override;

    /** Return the base octave offset. */
    int getOctaveOffset() const override;

    /** Set base octave offset, clamped to -4..+4. */
    void setOctaveOffset(int offset) override;

    /** Return transient LT/RT melody transposition offset. */
    int8_t getTriggerOctaveOffset() const override;

    /** Set transient LT/RT melody transposition offset, clamped to -4..+4. */
    void setTriggerOctaveOffset(int8_t offset) override;

    /** Apply base + trigger octave to a melody note and clamp to MIDI range. */
    uint8_t applyMelodyOctave(uint8_t baseNote, int8_t triggerOctave) const;

    /** Apply base + chord octave to a chord note and clamp to MIDI range. */
    uint8_t applyChordOctave(int baseNote, int8_t chordOctaveOffset) const;

    /** Return the preset's global shift button. */
    uint8_t getShiftButton() const override;

    /** Return a per-button shift override, or the global shift button. */
    uint8_t getShiftButtonForButton(uint8_t button) const override;

private:
    /** Active preset; all access is serialized by `EventDispatcher::fMapperMutex`. */
    MapperConfig::MapperPreset fPreset{};

    /** Base octave offset for melody and chord notes. */
    int8_t fOctaveOffset = 0;

    /** Transient LT/RT offset for melody notes; chord mode ignores it. */
    int8_t fTriggerOctaveOffset = 0;  // Cumulative LT/RT offset

    /**
     * Notes currently held by one button. Stores the exact sent pitches so
     * release events still match after octave changes.
     */
    struct ActiveNotes {
        uint8_t count = 0;
        std::array<uint8_t, MapperConfig::MAX_ACTIVE_NOTES> notes{};
    };

    /** Per-button active notes, serialized by the owner. */
    std::array<ActiveNotes, SDL_CONTROLLER_BUTTON_MAX> fActiveNotes{};

    /** Last toggle state for each MIDI CC number. */
    std::array<bool, 128> fCCToggleStates{};

    /** Last sent 7-bit CC value per axis, used to suppress duplicates. */
    std::array<uint8_t, SDL_CONTROLLER_AXIS_MAX> fLastAxisCCValues{};

    /** Last sent 14-bit pitch-bend value per axis. */
    std::array<uint16_t, SDL_CONTROLLER_AXIS_MAX> fLastAxisPitchBendValues{};

    /** Last sent 7-bit channel-aftertouch value per axis. */
    std::array<uint8_t, SDL_CONTROLLER_AXIS_MAX> fLastAxisAftertouchValues{};

    /** Emit Note On/Off for single-note button mode. */
    void handleNoteMode(const MapperConfig::ButtonConfig& config, bool pressed, uint8_t button, IMidiOutputSink& out);

    /** Emit Note On/Off messages for chord button mode. */
    void handleChordMode(const MapperConfig::ButtonConfig& config, bool pressed, uint8_t button, IMidiOutputSink& out);

    /** Emit 127 on press and 0 on release for momentary CC mode. */
    void handleCCMomentary(const MapperConfig::ButtonConfig& config, bool pressed, IMidiOutputSink& out);

    /** Toggle and emit CC state on button press. */
    void handleCCToggle(const MapperConfig::ButtonConfig& config, bool pressed, IMidiOutputSink& out);

    /** Normalize an axis and emit CC when the value changed. */
    void handleAxisCC(const MapperConfig::AxisConfig& config, int16_t value, uint8_t axis, IMidiOutputSink& out);

    /** Normalize an axis and emit 14-bit pitch bend when the value changed. */
    void handleAxisPitchBend(const MapperConfig::AxisConfig& config, int16_t value, uint8_t axis, IMidiOutputSink& out);

    /** Clamp an integer to legal MIDI note range. */
    uint8_t clampNote(int note) const;

    /** Convert SDL axis input to 0.0..1.0 with deadzone and inversion. */
    float normalizeAxis(int16_t value, const MapperConfig::AxisConfig& config) const;
};

}  // namespace GCMidi
