#include "FlexibleMapper.hpp"

#include <algorithm>
#include <cmath>
#include <nlohmann/json.hpp>

#include "../Common/MidiTypes.hpp"

using json = nlohmann::json;

namespace GCMidi {

FlexibleMapper::FlexibleMapper() = default;

bool FlexibleMapper::loadPreset(const uint8_t* jsonData, size_t jsonSize) {
    std::string jsonStr(reinterpret_cast<const char*>(jsonData), jsonSize);
    if (MapperConfig::deserializePreset(jsonStr, fPreset)) {
        this->setPreset(fPreset);
        return true;
    }
    return false;
}

void FlexibleMapper::setPreset(const MapperConfig::MapperPreset& preset) {
    fPreset = preset;

    // Reset runtime state
    fOctaveOffset = fPreset.baseOctaveOffset;

    for (auto& notes : fActiveNotes) {
        notes.count = 0;
    }

    fCCToggleStates.fill(false);
    fLastAxisCCValues.fill(255);  // Force initial send
    fLastAxisPitchBendValues.fill(0xFFFF);
    fLastAxisAftertouchValues.fill(255);
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

int8_t FlexibleMapper::getTriggerOctaveOffset() const {
    return fTriggerOctaveOffset;
}

void FlexibleMapper::setTriggerOctaveOffset(int8_t offset) {
    fTriggerOctaveOffset = std::clamp(offset, int8_t(-4), int8_t(4));
}

uint8_t FlexibleMapper::applyMelodyOctave(uint8_t baseNote, int8_t triggerOctave) const {
    int total = baseNote + (fOctaveOffset * 12) + (triggerOctave * 12);
    return clampNote(total);
}

uint8_t FlexibleMapper::applyChordOctave(uint8_t baseNote, int8_t chordOctaveOffset) const {
    // Chords ignore trigger octave, only use base + chord-specific offset
    int total = baseNote + (fOctaveOffset * 12) + (chordOctaveOffset * 12);
    return clampNote(total);
}

uint8_t FlexibleMapper::getShiftButton() const {
    return fPreset.shiftButton;
}

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

void FlexibleMapper::handleNoteMode(const MapperConfig::ButtonConfig& config, bool pressed, uint8_t button, MidiQueue& queue) {
    if (pressed) {
        // Calculate note and store it for later Note Off
        uint8_t note = applyMelodyOctave(config.noteOrCC, fTriggerOctaveOffset);

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
            uint8_t baseNote = config.noteOrCC + config.chordIntervals[i];
            uint8_t clampedNote = applyChordOctave(baseNote, config.chordOctaveOffset);

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
