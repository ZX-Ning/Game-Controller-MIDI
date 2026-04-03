#include <algorithm>
#include <cstring>
#include <nlohmann/json.hpp>

#include "MapperConfig.hpp"

namespace MapperConfig {

using json = nlohmann::json;

// Helper: Button name to SDL constant mapping
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
    if (name == "misc1") {
        return SDL_CONTROLLER_BUTTON_MISC1;
    }
    if (name == "paddle1") {
        return SDL_CONTROLLER_BUTTON_PADDLE1;
    }
    if (name == "paddle2") {
        return SDL_CONTROLLER_BUTTON_PADDLE2;
    }
    if (name == "paddle3") {
        return SDL_CONTROLLER_BUTTON_PADDLE3;
    }
    if (name == "paddle4") {
        return SDL_CONTROLLER_BUTTON_PADDLE4;
    }
    if (name == "touchpad") {
        return SDL_CONTROLLER_BUTTON_TOUCHPAD;
    }
    return -1;
}

// Helper: Axis name to SDL constant mapping
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

// Helper: Button index to name
const char* buttonIndexToName(int index) {
    static const char* names[] = {
        "a", "b", "x", "y", "back", "guide", "start", "leftstick", "rightstick", "leftshoulder", "rightshoulder", "dpad_up", "dpad_down", "dpad_left", "dpad_right", "misc1", "paddle1", "paddle2", "paddle3", "paddle4", "touchpad"
    };
    static_assert(SDL_CONTROLLER_BUTTON_MAX == 21, "SDL button count changed - update names array");
    static_assert(sizeof(names) / sizeof(names[0]) == SDL_CONTROLLER_BUTTON_MAX, "Button names array size mismatch");
    if (index >= 0 && index < SDL_CONTROLLER_BUTTON_MAX) {
        return names[index];
    }
    return "unknown";
}

// Helper: Axis index to name
const char* axisIndexToName(int index) {
    static const char* names[] = {
        "leftx", "lefty", "rightx", "righty", "triggerleft", "triggerright"
    };
    static_assert(SDL_CONTROLLER_AXIS_MAX == 6, "SDL axis count changed - update names array");
    static_assert(sizeof(names) / sizeof(names[0]) == SDL_CONTROLLER_AXIS_MAX, "Axis names array size mismatch");
    if (index >= 0 && index < SDL_CONTROLLER_AXIS_MAX) {
        return names[index];
    }
    return "unknown";
}

// Helper: ButtonMode to string
const char* buttonModeToString(ButtonMode mode) {
    switch (mode) {
        case ButtonMode::Note:
            return "note";
        case ButtonMode::Chord:
            return "chord";
        case ButtonMode::CC_Momentary:
            return "cc_momentary";
        case ButtonMode::CC_Toggle:
            return "cc_toggle";
        case ButtonMode::OctaveUp:
            return "octave_up";
        case ButtonMode::OctaveDown:
            return "octave_down";
        default:
            return "none";
    }
}

// Helper: String to ButtonMode
ButtonMode parseButtonMode(std::string_view mode) {
    if (mode == "note") {
        return ButtonMode::Note;
    }
    if (mode == "chord") {
        return ButtonMode::Chord;
    }
    if (mode == "cc_momentary") {
        return ButtonMode::CC_Momentary;
    }
    if (mode == "cc_toggle") {
        return ButtonMode::CC_Toggle;
    }
    if (mode == "octave_up") {
        return ButtonMode::OctaveUp;
    }
    if (mode == "octave_down") {
        return ButtonMode::OctaveDown;
    }
    return ButtonMode::None;
}

// Helper: AxisMode to string
const char* axisModeToString(AxisMode mode) {
    switch (mode) {
        case AxisMode::CC:
            return "cc";
        case AxisMode::PitchBend:
            return "pitchbend";
        case AxisMode::Aftertouch:
            return "aftertouch";
        default:
            return "none";
    }
}

// Helper: String to AxisMode
AxisMode parseAxisMode(std::string_view mode) {
    if (mode == "cc") {
        return AxisMode::CC;
    }
    if (mode == "pitchbend") {
        return AxisMode::PitchBend;
    }
    if (mode == "aftertouch") {
        return AxisMode::Aftertouch;
    }
    return AxisMode::None;
}

// Serialize ButtonConfig to JSON
json serializeButtonConfig(const ButtonConfig& cfg) {
    json j;
    j["mode"] = buttonModeToString(cfg.mode);
    j["note"] = cfg.noteOrCC;
    j["velocity"] = cfg.velocity;

    if (cfg.mode == ButtonMode::Chord && cfg.intervalCount > 0) {
        j["intervals"] = json::array();
        for (size_t i = 0; i < cfg.intervalCount; ++i) {
            j["intervals"].push_back(cfg.chordIntervals[i]);
        }
        j["chordOctaveOffset"] = cfg.chordOctaveOffset;
    }

    if (cfg.shiftButton >= 0) {
        j["shiftButton"] = cfg.shiftButton;
    }

    return j;
}

// Deserialize ButtonConfig from JSON
void parseButtonConfig(const json& j, ButtonConfig& config) {
    config.mode = parseButtonMode(j.value("mode", "none"));
    config.noteOrCC = j.value("note", j.value("cc", 60));
    config.velocity = j.value("velocity", 100);

    if (j.contains("intervals")) {
        const auto& intervals = j["intervals"];
        config.intervalCount = std::min(intervals.size(), MAX_CHORD_INTERVALS);
        for (size_t i = 0; i < config.intervalCount; ++i) {
            config.chordIntervals[i] = intervals[i].get<int8_t>();
        }
    }

    if (j.contains("chordOctaveOffset")) {
        config.chordOctaveOffset = std::clamp(j["chordOctaveOffset"].get<int>(), -4, 4);
    }

    if (j.contains("shiftButton")) {
        config.shiftButton = j["shiftButton"].get<int8_t>();
    }
}

// Serialize AxisConfig to JSON
json serializeAxisConfig(const AxisConfig& cfg) {
    json j;
    j["mode"] = axisModeToString(cfg.mode);
    j["cc"] = cfg.ccNumber;
    j["bipolar"] = cfg.isBipolar;
    j["inverted"] = cfg.isInverted;
    j["deadzone"] = cfg.deadzone;
    return j;
}

// Deserialize AxisConfig from JSON
void parseAxisConfig(const json& j, AxisConfig& config) {
    config.mode = parseAxisMode(j.value("mode", "none"));
    config.ccNumber = j.value("cc", 0);
    config.isBipolar = j.value("bipolar", true);
    config.isInverted = j.value("inverted", false);
    config.deadzone = j.value("deadzone", 0.1f);
}

std::string serializePreset(const MapperPreset& preset) {
    json j;

    // Metadata
    j["name"] = preset.name.data();
    j["channel"] = preset.channel;
    j["baseOctaveOffset"] = preset.baseOctaveOffset;
    j["shiftButton"] = preset.shiftButton;
    j["shiftButtonName"] = buttonIndexToName(preset.shiftButton);

    // Buttons
    j["buttons"] = json::object();
    j["shiftButtons"] = json::object();
    j["buttonShiftKeys"] = json::object();

    for (int i = 0; i < SDL_CONTROLLER_BUTTON_MAX; ++i) {
        const char* name = buttonIndexToName(i);

        if (preset.buttons[i].mode != ButtonMode::None) {
            j["buttons"][name] = serializeButtonConfig(preset.buttons[i]);
        }

        if (preset.shiftButtons[i].mode != ButtonMode::None) {
            j["shiftButtons"][name] = serializeButtonConfig(preset.shiftButtons[i]);
        }

        if (preset.buttons[i].shiftButton >= 0) {
            j["buttonShiftKeys"][name] = buttonIndexToName(preset.buttons[i].shiftButton);
        }
    }

    // Axes
    j["axes"] = json::object();
    j["shiftAxes"] = json::object();

    for (int i = 0; i < SDL_CONTROLLER_AXIS_MAX; ++i) {
        const char* name = axisIndexToName(i);

        if (preset.axes[i].mode != AxisMode::None) {
            j["axes"][name] = serializeAxisConfig(preset.axes[i]);
        }

        if (preset.shiftAxes[i].mode != AxisMode::None) {
            j["shiftAxes"][name] = serializeAxisConfig(preset.shiftAxes[i]);
        }
    }

    return j.dump(2);
}

bool deserializePreset(const std::string& jsonStr, MapperPreset& preset) {
    try {
        json j = json::parse(jsonStr);

        // Parse name
        std::string name = j.value("name", "Unnamed");
        std::memset(preset.name.data(), 0, preset.name.size());
        std::copy_n(name.begin(), std::min(name.size(), preset.name.size() - 1), preset.name.begin());

        preset.channel = j.value("channel", 0);
        preset.baseOctaveOffset = j.value("baseOctaveOffset", 0);
        preset.shiftButton = j.value("shiftButton", static_cast<uint8_t>(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER));

        if (j.contains("shiftButtonName")) {
            int idx = buttonNameToIndex(j["shiftButtonName"].get<std::string>());
            if (idx >= 0) {
                preset.shiftButton = static_cast<uint8_t>(idx);
            }
        }

        // Reset arrays
        preset.buttons.fill({});
        preset.shiftButtons.fill({});
        preset.axes.fill({});
        preset.shiftAxes.fill({});

        // Parse buttons
        if (j.contains("buttons")) {
            for (const auto& [key, value] : j["buttons"].items()) {
                int idx = buttonNameToIndex(key);
                if (idx >= 0 && idx < SDL_CONTROLLER_BUTTON_MAX) {
                    parseButtonConfig(value, preset.buttons[idx]);
                }
            }
        }

        if (j.contains("shiftButtons")) {
            for (const auto& [key, value] : j["shiftButtons"].items()) {
                int idx = buttonNameToIndex(key);
                if (idx >= 0 && idx < SDL_CONTROLLER_BUTTON_MAX) {
                    parseButtonConfig(value, preset.shiftButtons[idx]);
                }
            }
        }

        // Parse axes
        if (j.contains("axes")) {
            for (const auto& [key, value] : j["axes"].items()) {
                int idx = axisNameToIndex(key);
                if (idx >= 0 && idx < SDL_CONTROLLER_AXIS_MAX) {
                    parseAxisConfig(value, preset.axes[idx]);
                }
            }
        }

        if (j.contains("shiftAxes")) {
            for (const auto& [key, value] : j["shiftAxes"].items()) {
                int idx = axisNameToIndex(key);
                if (idx >= 0 && idx < SDL_CONTROLLER_AXIS_MAX) {
                    parseAxisConfig(value, preset.shiftAxes[idx]);
                }
            }
        }

        // Parse per-button shift key overrides
        if (j.contains("buttonShiftKeys")) {
            for (const auto& [key, value] : j["buttonShiftKeys"].items()) {
                int btnIdx = buttonNameToIndex(key);
                if (btnIdx >= 0 && btnIdx < SDL_CONTROLLER_BUTTON_MAX) {
                    int shiftIdx = buttonNameToIndex(value.get<std::string>());
                    if (shiftIdx >= 0 && shiftIdx < SDL_CONTROLLER_BUTTON_MAX) {
                        preset.buttons[btnIdx].shiftButton = static_cast<int8_t>(shiftIdx);
                        preset.shiftButtons[btnIdx].shiftButton = static_cast<int8_t>(shiftIdx);
                    }
                }
            }
        }

        return true;
    }
    catch (...) {
        return false;
    }
}

}  // namespace MapperConfig
