#include "Plugin.hpp"

#include <cstring>
#include <string_view>
#include <unordered_map>

#include "Core/SdlManager.hpp"
#include "Logic/FlexibleMapper.hpp"

// Include the generated preset header
#include "preset_c_major_scale_chords.hpp"

START_NAMESPACE_DISTRHO

namespace {
// Helper to convert state key string to enum
int stringToStateEnum(std::string_view key) {
    static const std::unordered_map<std::string_view, int> stateMap = {
        {"config", GameControllerMIDIPlugin::kStateConfig},
        {"triggerOctave", GameControllerMIDIPlugin::kStateTriggerOctave},
        {"editMode", GameControllerMIDIPlugin::kStateEditMode},
        {"width", GameControllerMIDIPlugin::kStateWidth},
        {"height", GameControllerMIDIPlugin::kStateHeight}
    };

    auto it = stateMap.find(key);
    return (it != stateMap.end()) ? it->second : -1;
}
}  // namespace

std::atomic<int> GameControllerMIDIPlugin::sInstanceCount(0);

GameControllerMIDIPlugin::GameControllerMIDIPlugin()
    : Plugin(kParamCount, 0, kStateCount) {
    int count = ++sInstanceCount;
    if (count > 1) {
        d_stderr2("WARNING: Multiple Plugin instances detected (%d)! This plugin is designed for a single instance only.", count);
    }
    // Create the dispatcher (owned by this plugin)
    fDispatcher = std::make_unique<GCMidi::EventDispatcher>();

    // Initial mapper configuration
    auto mapper = std::make_unique<GCMidi::FlexibleMapper>();
    if (mapper->loadPreset(preset_c_major_scale_chords_data, preset_c_major_scale_chords_size)) {
        fActiveConfig = mapper->getPreset();
        fDispatcher->setMapper(std::move(mapper));
    }
    else {
        d_stderr2("ERROR: Failed to load preset 'c_major_scale_chords'. Plugin will not produce MIDI output.");
    }

    // Register with the global SDL pump
    GCMidi::SdlManager::getInstance().setEventHandler(fDispatcher.get());
}

GameControllerMIDIPlugin::~GameControllerMIDIPlugin() {
    // Unregister before destruction
    GCMidi::SdlManager::getInstance().setEventHandler(nullptr);
    sInstanceCount--;
}

void GameControllerMIDIPlugin::initParameter(uint32_t index, Parameter& parameter) {
    if (index == kParamOctave) {
        parameter.hints = kParameterIsAutomatable | kParameterIsInteger;
        parameter.name = "Octave";
        parameter.symbol = "octave";
        parameter.ranges.def = 0.0f;
        parameter.ranges.min = -4.0f;
        parameter.ranges.max = 4.0f;
    }
}

float GameControllerMIDIPlugin::getParameterValue(uint32_t index) const {
    if (index == kParamOctave && fDispatcher) {
        if (auto mapper = fDispatcher->getMapper()) {
            return static_cast<float>(mapper->getOctaveOffset());
        }
    }
    return 0.0f;
}

void GameControllerMIDIPlugin::setParameterValue(uint32_t index, float value) {
    if (index == kParamOctave && fDispatcher) {
        if (auto mapper = fDispatcher->getMapper()) {
            mapper->setOctaveOffset(static_cast<int>(value));
        }
    }
}

void GameControllerMIDIPlugin::initState(uint32_t index, State& state) {
    switch (index) {
        case kStateConfig:
            state.key = "config";
            state.defaultValue = "";  // Empty = use embedded preset
            state.label = "Mapper Configuration";
            state.hints = kStateIsBase64Blob;  // Must be visible to both UI and DSP
            break;
        case kStateTriggerOctave:
            state.key = "triggerOctave";
            state.defaultValue = "0";
            state.label = "Trigger Octave Offset";
            break;
        case kStateEditMode:
            state.key = "editMode";
            state.defaultValue = "false";
            state.label = "Edit Mode Active";
            break;
        case kStateWidth:
            state.key = "width";
            state.defaultValue = "1000";
            state.label = "Window Width";
            break;
        case kStateHeight:
            state.key = "height";
            state.defaultValue = "500";
            state.label = "Window Height";
            break;
    }
}

void GameControllerMIDIPlugin::setState(const char* key, const char* value) {
    int stateEnum = stringToStateEnum(key);
    if (stateEnum < 0) {
        return;
    }

    switch (stateEnum) {
        case kStateConfig:
            if (value && std::strlen(value) > 0) {
                MapperConfig::MapperPreset newConfig;
                if (MapperConfig::deserializePreset(value, newConfig)) {
                    {
                        std::lock_guard<std::mutex> lock(fConfigMutex);
                        fActiveConfig = newConfig;
                    }
                    // Reload outside lock to avoid deadlock with UI direct access
                    reloadMapper();
                }
            }
            break;
        case kStateTriggerOctave: {
            int8_t offset = static_cast<int8_t>(std::atoi(value));
            fTriggerOctaveOffset = offset;
            if (fDispatcher) {
                fDispatcher->setTriggerOctaveOffset(offset);
            }
            break;
        }
        case kStateEditMode:
            fPlayMode = (std::strcmp(value, "true") != 0);  // editMode=true -> fPlayMode=false
            break;
        case kStateWidth:
            fWidth = static_cast<uint32_t>(std::atoi(value));
            break;
        case kStateHeight:
            fHeight = static_cast<uint32_t>(std::atoi(value));
            break;
        case kStateCount:
            // Sentinel value, not an actual state
            break;
    }
}

String GameControllerMIDIPlugin::getState(const char* key) const {
    int stateEnum = stringToStateEnum(key);
    if (stateEnum < 0) {
        return String("");
    }

    switch (stateEnum) {
        case kStateConfig: {
            std::lock_guard<std::mutex> lock(fConfigMutex);
            return String(MapperConfig::serializePreset(fActiveConfig).c_str());
        }
        case kStateTriggerOctave:
            return String(std::to_string(fTriggerOctaveOffset.load()).c_str());
        case kStateEditMode:
            return String(!fPlayMode.load() ? "true" : "false");
        case kStateWidth:
            return String(std::to_string(fWidth.load()).c_str());
        case kStateHeight:
            return String(std::to_string(fHeight.load()).c_str());
        case kStateCount:
            // Sentinel value, not an actual state
            break;
    }
    return String("");
}

void GameControllerMIDIPlugin::reloadMapper() {
    auto mapper = std::make_unique<GCMidi::FlexibleMapper>();
    mapper->setPreset(fActiveConfig);
    fDispatcher->setMapper(std::move(mapper));
}

void GameControllerMIDIPlugin::run(
    const float**,
    float**,
    uint32_t,
    const MidiEvent* midiEvents,
    uint32_t midiEventCount
) {
    // 0. Check for parameter changes from within the dispatcher (e.g. D-Pad octave change)
    if (fDispatcher && fDispatcher->getAndResetOctaveDirty()) {
        if (auto mapper = fDispatcher->getMapper()) {
            requestParameterValueChange(
                kParamOctave,
                static_cast<float>(mapper->getOctaveOffset())
            );
        }
    }

    // 1. Process host MIDI
    for (uint32_t i = 0; i < midiEventCount; ++i) {
        writeMidiEvent(midiEvents[i]);
    }

    // 2. Poll MIDI from dispatcher (generated by SDL thread)
    if (fDispatcher) {
        GCMidi::RawMidi rawEv;
        while (fDispatcher->popMidi(rawEv)) {
            MidiEvent ev;
            ev.frame = 0;
            ev.size = rawEv.size;
            ev.dataExt = nullptr;
            std::memcpy(ev.data, rawEv.data, ev.size);

            writeMidiEvent(ev);
        }
    }
}

Plugin* createPlugin() {
    return new GameControllerMIDIPlugin();
}

END_NAMESPACE_DISTRHO
