#include "FlexibleMapper.hpp"

#include <algorithm>
#include <cmath>
#include <nlohmann/json.hpp>

#include "../Common/MidiTypes.hpp"

using json = nlohmann::json;

namespace GCMidi {

namespace {
// Button name to SDL constant mapping
int buttonNameToIndex(std::string_view name) {
    if (name == "a") {
        return SDL_CONTROLLER_BUTTON_A;
    }
    if (name == "b") {
        return SDL_CONTROLLER_BUTTON_B;
    }
    if (name == "x") {
        return SDL_CONTROLLER_BUTTON_X;
    }
    if (name == "y") {
        return SDL_CONTROLLER_BUTTON_Y;
    }
    if (name == "back") {
        return SDL_CONTROLLER_BUTTON_BACK;
    }
    if (name == "guide") {
        return SDL_CONTROLLER_BUTTON_GUIDE;
    }
    if (name == "start") {
        return SDL_CONTROLLER_BUTTON_START;
    }
    if (name == "leftstick") {
        return SDL_CONTROLLER_BUTTON_LEFTSTICK;
    }
    if (name == "rightstick") {
        return SDL_CONTROLLER_BUTTON_RIGHTSTICK;
    }
    if (name == "leftshoulder") {
        return SDL_CONTROLLER_BUTTON_LEFTSHOULDER;
    }
    if (name == "rightshoulder") {
        return SDL_CONTROLLER_BUTTON_RIGHTSHOULDER;
    }
    if (name == "dpad_up") {
        return SDL_CONTROLLER_BUTTON_DPAD_UP;
    }
    if (name == "dpad_down") {
        return SDL_CONTROLLER_BUTTON_DPAD_DOWN;
    }
    if (name == "dpad_left") {
        return SDL_CONTROLLER_BUTTON_DPAD_LEFT;
    }
    if (name == "dpad_right") {
        return SDL_CONTROLLER_BUTTON_DPAD_RIGHT;
    }
    return -1;
}

int axisNameToIndex(std::string_view name) {
    if (name == "leftx") {
        return SDL_CONTROLLER_AXIS_LEFTX;
    }
    if (name == "lefty") {
        return SDL_CONTROLLER_AXIS_LEFTY;
    }
    if (name == "rightx") {
        return SDL_CONTROLLER_AXIS_RIGHTX;
    }
    if (name == "righty") {
        return SDL_CONTROLLER_AXIS_RIGHTY;
    }
    if (name == "triggerleft") {
        return SDL_CONTROLLER_AXIS_TRIGGERLEFT;
    }
    if (name == "triggerright") {
        return SDL_CONTROLLER_AXIS_TRIGGERRIGHT;
    }
    return -1;
}

MapperConfig::ButtonMode parseButtonMode(std::string_view mode) {
    if (mode == "note") {
        return MapperConfig::ButtonMode::Note;
    }
    if (mode == "chord") {
        return MapperConfig::ButtonMode::Chord;
    }
    if (mode == "cc_momentary") {
        return MapperConfig::ButtonMode::CC_Momentary;
    }
    if (mode == "cc_toggle") {
        return MapperConfig::ButtonMode::CC_Toggle;
    }
    if (mode == "octave_up") {
        return MapperConfig::ButtonMode::OctaveUp;
    }
    if (mode == "octave_down") {
        return MapperConfig::ButtonMode::OctaveDown;
    }
    return MapperConfig::ButtonMode::None;
}

MapperConfig::AxisMode parseAxisMode(std::string_view mode) {
    if (mode == "cc") {
        return MapperConfig::AxisMode::CC;
    }
    if (mode == "pitchbend") {
        return MapperConfig::AxisMode::PitchBend;
    }
    if (mode == "aftertouch") {
        return MapperConfig::AxisMode::Aftertouch;
    }
    return MapperConfig::AxisMode::None;
}

void parseButtonConfig(const json& j, MapperConfig::ButtonConfig& config) {
    config.mode = parseButtonMode(j.value("mode", "none"));
    config.noteOrCC = j.value("note", j.value("cc", 60));
    config.velocity = j.value("velocity", 100);

    if (j.contains("intervals")) {
        const auto& intervals = j["intervals"];
        config.intervalCount = std::min(intervals.size(), MapperConfig::MAX_CHORD_INTERVALS);
        for (size_t i = 0; i < config.intervalCount; ++i) {
            config.chordIntervals[i] = intervals[i].get<int8_t>();
        }
    }
}

void parseAxisConfig(const json& j, MapperConfig::AxisConfig& config) {
    config.mode = parseAxisMode(j.value("mode", "none"));
    config.ccNumber = j.value("cc", 0);
    config.isBipolar = j.value("bipolar", true);
    config.isInverted = j.value("inverted", false);
    config.deadzone = j.value("deadzone", 0.1f);
}
}  // anonymous namespace

FlexibleMapper::FlexibleMapper() = default;

bool FlexibleMapper::loadPreset(const uint8_t* jsonData, size_t jsonSize) {
    try {
        json j = json::parse(jsonData, jsonData + jsonSize);

        // Parse name
        std::string name = j.value("name", "Unnamed");
        std::copy_n(name.begin(), std::min(name.size(), size_t{63}), fPreset.name.begin());

        fPreset.channel = j.value("channel", 0);
        fPreset.baseOctaveOffset = j.value("baseOctaveOffset", 0);
        fOctaveOffset = fPreset.baseOctaveOffset;

        fPreset.shiftButton = j.value("shiftButton", static_cast<uint8_t>(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER));

        // Also support string-based button name
        if (j.contains("shiftButtonName")) {
            int idx = buttonNameToIndex(j["shiftButtonName"].get<std::string>());
            if (idx >= 0) {
                fPreset.shiftButton = static_cast<uint8_t>(idx);
            }
        }

        // Parse buttons
        if (j.contains("buttons")) {
            for (const auto& [key, value] : j["buttons"].items()) {
                int idx = buttonNameToIndex(key);
                if (idx >= 0 && idx < SDL_CONTROLLER_BUTTON_MAX) {
                    parseButtonConfig(value, fPreset.buttons[idx]);
                }
            }
        }

        // Parse shift buttons
        if (j.contains("shiftButtons")) {
            for (const auto& [key, value] : j["shiftButtons"].items()) {
                int idx = buttonNameToIndex(key);
                if (idx >= 0 && idx < SDL_CONTROLLER_BUTTON_MAX) {
                    parseButtonConfig(value, fPreset.shiftButtons[idx]);
                }
            }
        }

        // Parse axes
        if (j.contains("axes")) {
            for (const auto& [key, value] : j["axes"].items()) {
                int idx = axisNameToIndex(key);
                if (idx >= 0 && idx < SDL_CONTROLLER_AXIS_MAX) {
                    parseAxisConfig(value, fPreset.axes[idx]);
                }
            }
        }

        // Parse shift axes
        if (j.contains("shiftAxes")) {
            for (const auto& [key, value] : j["shiftAxes"].items()) {
                int idx = axisNameToIndex(key);
                if (idx >= 0 && idx < SDL_CONTROLLER_AXIS_MAX) {
                    parseAxisConfig(value, fPreset.shiftAxes[idx]);
                }
            }
        }

        return true;
    }
    catch (const json::exception&) {
        return false;
    }
}

void FlexibleMapper::onButton(uint8_t button, bool pressed, bool shift, MidiQueue& queue) {
    if (button >= SDL_CONTROLLER_BUTTON_MAX) {
        return;
    }

    const auto& config = shift ? fPreset.shiftButtons[button] : fPreset.buttons[button];

    switch (config.mode) {
        case MapperConfig::ButtonMode::Note:
            handleNoteMode(config, pressed, button, queue);
            break;
        case MapperConfig::ButtonMode::Chord:
            handleChordMode(config, pressed, button, queue);
            break;
        case MapperConfig::ButtonMode::CC_Momentary:
            handleCCMomentary(config, pressed, queue);
            break;
        case MapperConfig::ButtonMode::CC_Toggle:
            handleCCToggle(config, pressed, queue);
            break;
        case MapperConfig::ButtonMode::OctaveUp:
            if (pressed && fOctaveOffset < 4) {
                ++fOctaveOffset;
            }
            break;
        case MapperConfig::ButtonMode::OctaveDown:
            if (pressed && fOctaveOffset > -4) {
                --fOctaveOffset;
            }
            break;
        case MapperConfig::ButtonMode::None:
        default:
            break;
    }
}

void FlexibleMapper::onAxis(uint8_t axis, int16_t value, bool shift, MidiQueue& queue) {
    if (axis >= SDL_CONTROLLER_AXIS_MAX) {
        return;
    }

    const auto& config = shift ? fPreset.shiftAxes[axis] : fPreset.axes[axis];

    switch (config.mode) {
        case MapperConfig::AxisMode::CC:
            handleAxisCC(config, value, axis, queue);
            break;
        case MapperConfig::AxisMode::PitchBend:
            handleAxisPitchBend(config, value, axis, queue);
            break;
        case MapperConfig::AxisMode::Aftertouch:
            // Similar to CC but uses aftertouch message
            {
                float normalized = normalizeAxis(value, config);
                uint8_t atValue = static_cast<uint8_t>(normalized * 127.0f);

                // Only send if value changed
                if (atValue == fLastAxisAftertouchValues[axis]) {
                    break;
                }
                fLastAxisAftertouchValues[axis] = atValue;

                RawMidi midi{};
                midi.data[0] = 0xD0 | (fPreset.channel & 0x0F);  // Channel Aftertouch
                midi.data[1] = atValue;
                midi.size = 2;
                queue.push(midi);
            }
            break;
        case MapperConfig::AxisMode::None:
        default:
            break;
    }
}

int FlexibleMapper::getOctaveOffset() const {
    return fOctaveOffset;
}

void FlexibleMapper::setOctaveOffset(int offset) {
    fOctaveOffset = static_cast<int8_t>(std::clamp(offset, -4, 4));
}

uint8_t FlexibleMapper::getShiftButton() const {
    return fPreset.shiftButton;
}

void FlexibleMapper::handleNoteMode(const MapperConfig::ButtonConfig& config, bool pressed, uint8_t button, MidiQueue& queue) {
    if (pressed) {
        // Calculate note and store it for later Note Off
        uint8_t note = clampNote(config.noteOrCC + fOctaveOffset * 12);

        fActiveNotes[button].count = 1;
        fActiveNotes[button].notes[0] = note;

        RawMidi midi{};
        midi.data[0] = 0x90 | (fPreset.channel & 0x0F);  // Note On
        midi.data[1] = note;
        midi.data[2] = config.velocity;
        midi.size = 3;
        queue.push(midi);
    }
    else {
        // Send Note Off for the stored note, not recalculated one
        if (fActiveNotes[button].count > 0) {
            RawMidi midi{};
            midi.data[0] = 0x80 | (fPreset.channel & 0x0F);  // Note Off
            midi.data[1] = fActiveNotes[button].notes[0];
            midi.data[2] = 0;
            midi.size = 3;
            queue.push(midi);

            fActiveNotes[button].count = 0;
        }
    }
}

void FlexibleMapper::handleChordMode(const MapperConfig::ButtonConfig& config, bool pressed, uint8_t button, MidiQueue& queue) {
    if (pressed) {
        // Calculate and store all chord notes
        fActiveNotes[button].count = 0;
        for (size_t i = 0; i < config.intervalCount && i < MapperConfig::MAX_ACTIVE_NOTES; ++i) {
            int note = config.noteOrCC + config.chordIntervals[i] + fOctaveOffset * 12;
            uint8_t clampedNote = clampNote(note);

            fActiveNotes[button].notes[fActiveNotes[button].count++] = clampedNote;

            RawMidi midi{};
            midi.data[0] = 0x90 | (fPreset.channel & 0x0F);  // Note On
            midi.data[1] = clampedNote;
            midi.data[2] = config.velocity;
            midi.size = 3;
            queue.push(midi);
        }
    }
    else {
        // Send Note Off for all active notes
        for (size_t i = 0; i < fActiveNotes[button].count; ++i) {
            RawMidi midi{};
            midi.data[0] = 0x80 | (fPreset.channel & 0x0F);  // Note Off
            midi.data[1] = fActiveNotes[button].notes[i];
            midi.data[2] = 0;
            midi.size = 3;
            queue.push(midi);
        }
        fActiveNotes[button].count = 0;
    }
}

void FlexibleMapper::handleCCMomentary(const MapperConfig::ButtonConfig& config, bool pressed, MidiQueue& queue) {
    RawMidi midi{};
    midi.data[0] = 0xB0 | (fPreset.channel & 0x0F);  // Control Change
    midi.data[1] = config.noteOrCC;
    midi.data[2] = pressed ? 127 : 0;
    midi.size = 3;
    queue.push(midi);
}

void FlexibleMapper::handleCCToggle(const MapperConfig::ButtonConfig& config, bool pressed, MidiQueue& queue) {
    if (!pressed) {
        return;  // Only toggle on press
    }

    bool& state = fCCToggleStates[config.noteOrCC];
    state = !state;

    RawMidi midi{};
    midi.data[0] = 0xB0 | (fPreset.channel & 0x0F);
    midi.data[1] = config.noteOrCC;
    midi.data[2] = state ? 127 : 0;
    midi.size = 3;
    queue.push(midi);
}

void FlexibleMapper::handleAxisCC(const MapperConfig::AxisConfig& config, int16_t value, uint8_t axis, MidiQueue& queue) {
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

void FlexibleMapper::handleAxisPitchBend(const MapperConfig::AxisConfig& config, int16_t value, uint8_t axis, MidiQueue& queue) {
    float normalized = normalizeAxis(value, config);
    uint16_t bendValue = static_cast<uint16_t>(normalized * 16383.0f);

    // Only send if value changed
    if (bendValue == fLastAxisPitchBendValues[axis]) {
        return;
    }
    fLastAxisPitchBendValues[axis] = bendValue;

    RawMidi midi{};
    midi.data[0] = 0xE0 | (fPreset.channel & 0x0F);  // Pitch Bend
    midi.data[1] = bendValue & 0x7F;                 // LSB
    midi.data[2] = (bendValue >> 7) & 0x7F;          // MSB
    midi.size = 3;
    queue.push(midi);
}

uint8_t FlexibleMapper::clampNote(int note) const {
    return static_cast<uint8_t>(std::clamp(note, 0, 127));
}

float FlexibleMapper::normalizeAxis(int16_t value, const MapperConfig::AxisConfig& config) const {
    float normalized;

    if (config.isBipolar) {
        // Bipolar: -32768 to 32767 -> 0.0 to 1.0
        normalized = (static_cast<float>(value) + 32768.0f) / 65535.0f;
    }
    else {
        // Unipolar (triggers): 0 to 32767 -> 0.0 to 1.0
        normalized = static_cast<float>(std::max(value, int16_t{0})) / 32767.0f;
    }

    // Apply deadzone
    if (config.isBipolar) {
        float centered = normalized - 0.5f;
        if (std::abs(centered) < config.deadzone / 2.0f) {
            normalized = 0.5f;
        }
    }
    else {
        if (normalized < config.deadzone) {
            normalized = 0.0f;
        }
    }

    if (config.isInverted) {
        normalized = 1.0f - normalized;
    }

    return std::clamp(normalized, 0.0f, 1.0f);
}

}  // namespace GCMidi
