#include "Plugin.hpp"

#include <cstring>

#include "Core/SdlManager.hpp"
#include "Logic/MajorScaleMapper.hpp"

START_NAMESPACE_DISTRHO

GameControllerMIDIPlugin::GameControllerMIDIPlugin()
    : Plugin(kParamCount, 0, 0),
      fMidiHistoryIndex(0),
      fControllerConnected(false),
      fMidiQueue(256) {
    std::memset(fControllerName, 0, sizeof(fControllerName));
    std::memset(fButtonStates, 0, sizeof(fButtonStates));

    fMidiHistory.resize(kMidiHistorySize);

    fMapper = std::make_unique<GCMidi::MajorScaleMapper>();

    GCMidi::SdlManager::getInstance().setEventHandler(this);
}

GameControllerMIDIPlugin::~GameControllerMIDIPlugin() {
    GCMidi::SdlManager::getInstance().setEventHandler(nullptr);
}

void GameControllerMIDIPlugin::onControllerConnected(const char* name) {
    fControllerConnected = true;
    if (name) {
        std::strncpy(fControllerName, name, sizeof(fControllerName) - 1);
        fControllerName[sizeof(fControllerName) - 1] = '\0';
    }
    else {
        std::strncpy(fControllerName, "Unknown Controller", sizeof(fControllerName) - 1);
    }
}

void GameControllerMIDIPlugin::onControllerDisconnected() {
    fControllerConnected = false;
    std::memset(fControllerName, 0, sizeof(fControllerName));
    std::memset(fButtonStates, 0, sizeof(fButtonStates));
}

void GameControllerMIDIPlugin::onControllerButton(uint8_t button, bool pressed, bool shiftState) {
    if (button < SDL_CONTROLLER_BUTTON_MAX) {
        fButtonStates[button] = pressed;
    }

    if (fMapper) {
        fMapper->onButton(button, pressed, shiftState, fMidiQueue);
    }
}

void GameControllerMIDIPlugin::run(const float**, float**, uint32_t, const MidiEvent* midiEvents, uint32_t midiEventCount) {
    for (uint32_t i = 0; i < midiEventCount; ++i) {
        if (midiEvents[i].size <= 4) {
            uint32_t historyIdx = fMidiHistoryIndex.load(std::memory_order_relaxed);
            historyIdx = (historyIdx + 1) % kMidiHistorySize;
            fMidiHistory[historyIdx].size = midiEvents[i].size;
            std::memcpy(fMidiHistory[historyIdx].data, midiEvents[i].data, midiEvents[i].size);
            fMidiHistoryIndex.store(historyIdx, std::memory_order_relaxed);
        }
        writeMidiEvent(midiEvents[i]);
    }

    // Drain our internal queue
    GCMidi::RawMidi rawEv;
    while (fMidiQueue.pop(rawEv)) {
        MidiEvent ev;
        ev.frame = 0;
        ev.size = rawEv.size;
        ev.dataExt = nullptr;
        std::memcpy(ev.data, rawEv.data, ev.size);

        uint32_t historyIdx = fMidiHistoryIndex.load(std::memory_order_relaxed);
        historyIdx = (historyIdx + 1) % kMidiHistorySize;
        fMidiHistory[historyIdx].size = ev.size;
        std::memcpy(fMidiHistory[historyIdx].data, ev.data, ev.size);
        fMidiHistoryIndex.store(historyIdx, std::memory_order_relaxed);

        writeMidiEvent(ev);
    }
}

Plugin* createPlugin() {
    return new GameControllerMIDIPlugin();
}

END_NAMESPACE_DISTRHO
