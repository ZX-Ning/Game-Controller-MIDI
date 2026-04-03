#pragma once

#include <SDL.h>

#include <array>
#include <cstdint>
#include <string>
#include <string_view>

namespace MapperConfig {

// Maximum chord size (root + 5 intervals covers most chords)
inline constexpr size_t MAX_CHORD_INTERVALS = 6;

// Maximum active notes per button (same as chord size)
inline constexpr size_t MAX_ACTIVE_NOTES = MAX_CHORD_INTERVALS;

enum class ButtonMode : uint8_t {
    None,
    Note,          // Single MIDI note
    Chord,         // Multiple MIDI notes (root + intervals)
    CC_Momentary,  // CC 127 on press, 0 on release
    CC_Toggle,     // CC toggles between 127 and 0
    OctaveUp,      // DEPRECATED: Use trigger octave control instead
    OctaveDown     // DEPRECATED: Use trigger octave control instead
};

enum class AxisMode : uint8_t {
    None,
    CC,         // Control Change (7-bit, 0-127)
    PitchBend,  // Pitch Bend (14-bit, 0-16383)
    Aftertouch  // Channel Aftertouch (7-bit, 0-127)
};

struct ButtonConfig {
    ButtonMode mode = ButtonMode::None;
    uint8_t noteOrCC = 60;                                     // Base note (MIDI number) or CC number
    uint8_t velocity = 100;                                    // Note velocity (0-127)
    uint8_t intervalCount = 0;                                 // Number of valid intervals in chordIntervals
    std::array<int8_t, MAX_CHORD_INTERVALS> chordIntervals{};  // Semitone offsets from base note

    // Per-button shift key override
    // -1 = use global shift from MapperPreset.shiftButton
    // 0-14 = specific SDL_GameControllerButton index
    int8_t shiftButton = -1;

    // Per-chord octave offset (added to base octave)
    // Only affects Chord mode, ignored for other modes
    int8_t chordOctaveOffset = 0;  // Range: -4 to +4
};

struct AxisConfig {
    AxisMode mode = AxisMode::None;
    uint8_t ccNumber = 0;     // CC number (for CC mode)
    bool isBipolar = true;    // true: stick (-32768 to 32767), false: trigger (0 to 32767)
    bool isInverted = false;  // Invert axis direction
    float deadzone = 0.1f;    // Deadzone as fraction (0.0 to 1.0)
};

struct MapperPreset {
    std::array<char, 64> name{};  // Preset name (null-terminated)
    uint8_t channel = 0;          // MIDI channel (0-15)
    int8_t baseOctaveOffset = 0;  // Starting octave offset
    uint8_t shiftButton = SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;

    std::array<ButtonConfig, SDL_CONTROLLER_BUTTON_MAX> buttons{};
    std::array<ButtonConfig, SDL_CONTROLLER_BUTTON_MAX> shiftButtons{};  // When shift is held
    std::array<AxisConfig, SDL_CONTROLLER_AXIS_MAX> axes{};
    std::array<AxisConfig, SDL_CONTROLLER_AXIS_MAX> shiftAxes{};  // When shift is held
};

// Serialization support
std::string serializePreset(const MapperPreset& preset);
bool deserializePreset(const std::string& json, MapperPreset& preset);

}  // namespace MapperConfig
